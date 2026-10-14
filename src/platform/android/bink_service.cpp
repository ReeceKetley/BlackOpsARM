#include "bink_service.h"
#include "faudio_bridge.h"
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/intreadwrite.h>
#include <libavutil/error.h>
#include <libavutil/mathematics.h>
}
#include <vector>
#include <deque>
#include <memory>
#include <string>
#include <chrono>
#include <atomic>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <limits>
#include <cstdio>
namespace {
thread_local std::string error;
thread_local std::vector<unsigned> selected;
thread_local IXAudio2 *audioEngine{};
thread_local IXAudio2Voice *audioOutput{};
using Clock=std::chrono::steady_clock;
bool Fail(const char *message){error=message;return false;}
bool AvFail(const char *operation,int status){char text[AV_ERROR_MAX_STRING_SIZE]{};av_strerror(status,text,sizeof(text));error=std::string(operation)+": "+text;return false;}
struct MovieTrack final:IXAudio2VoiceCallback {
    struct Buffer {std::vector<float> pcm;std::atomic<bool> ended{};};
    unsigned id{},channels{},rate{};AVCodecContext *codec{};IXAudio2SourceVoice *voice{};
    IXAudio2 *engine{};IXAudio2Voice *destination{};
    std::deque<Buffer> buffers;std::atomic<bool> failed{};float volume{1};std::uint64_t submitted{};
    ~MovieTrack(){if(voice)voice->DestroyVoice();avcodec_free_context(&codec);}
    bool Create(){
        if(!engine || !destination || engine->Failure())return Fail("Bink audio requires the actual initialized game FAudio graph");
        WAVEFORMATEX format{};format.wFormatTag=FAUDIO_FORMAT_IEEE_FLOAT;format.nChannels=channels;format.nSamplesPerSec=rate;format.wBitsPerSample=32;format.nBlockAlign=channels*4;format.nAvgBytesPerSec=rate*format.nBlockAlign;
        XAUDIO2_SEND_DESCRIPTOR send{0,destination};XAUDIO2_VOICE_SENDS sends{1,&send};
        if(engine->CreateSourceVoice(&voice,&format,0,2,this,&sends,nullptr)!=S_OK)return Fail("actual Bink movie source creation failed");
        XAUDIO2_VOICE_DETAILS out{};destination->GetVoiceDetails(&out);if(!out.InputChannels || out.InputChannels>8)return Fail("invalid actual Bink destination topology");
        float matrix[16]{};for(unsigned i=0;i<channels;++i){if(channels==1){matrix[0]=1;if(out.InputChannels>1)matrix[1]=1;}else matrix[i*out.InputChannels+(i%out.InputChannels)]=1;}
        if(voice->SetOutputMatrix(destination,channels,out.InputChannels,matrix,0)!=S_OK || voice->SetVolume(volume,0)!=S_OK)return Fail("actual Bink output matrix/volume failed");return true;
    }
    bool Queue(AVFrame *frame,bool paused){
        if(failed || !voice || !engine || engine->Failure() || frame->format!=AV_SAMPLE_FMT_FLTP || frame->sample_rate!=static_cast<int>(rate) || frame->ch_layout.nb_channels!=static_cast<int>(channels) || frame->nb_samples<=0)return Fail("invalid actual Bink audio decoder output");
        while(!buffers.empty() && buffers.front().ended.load(std::memory_order_acquire))buffers.pop_front();
        XAUDIO2_VOICE_STATE state{};voice->GetState(&state);
        // Bounded actual SDK queue: failure is explicit, never lose decoded audio silently.
        if(state.BuffersQueued>=FAUDIO_MAX_QUEUED_BUFFERS || submitted<state.SamplesPlayed || submitted-state.SamplesPlayed>rate*4ULL)return Fail("actual Bink audio backlog exceeds four seconds");
        buffers.emplace_back();auto &b=buffers.back();b.pcm.resize(static_cast<std::size_t>(frame->nb_samples)*channels);
        for(int n=0;n<frame->nb_samples;++n)for(unsigned c=0;c<channels;++c){float v=reinterpret_cast<float *>(frame->extended_data[c])[n];if(!std::isfinite(v)){buffers.pop_back();return Fail("nonfinite actual Bink PCM");}b.pcm[n*channels+c]=v;}
        XAUDIO2_BUFFER input{};input.AudioBytes=b.pcm.size()*sizeof(float);input.pAudioData=reinterpret_cast<const std::uint8_t *>(b.pcm.data());input.pContext=&b;
        if(voice->SubmitSourceBuffer(&input,nullptr)!=S_OK){buffers.pop_back();return Fail("actual Bink audio submission failed");}
        submitted+=frame->nb_samples;if(!paused && voice->Start(0,0)!=S_OK)return Fail("actual Bink movie track start failed");return true;
    }
    void OnVoiceProcessingPassStart(UINT32) override{} void OnVoiceProcessingPassEnd() override{} void OnStreamEnd() override{} void OnBufferStart(void *) override{}
    void OnBufferEnd(void *p) override{static_cast<Buffer *>(p)->ended.store(true,std::memory_order_release);}void OnLoopEnd(void *) override{}void OnVoiceError(void *,HRESULT) override{failed=true;}
};
struct Movie {
    BINK publicView{};std::vector<std::uint8_t> input;std::size_t cursor{};AVIOContext *io{};AVFormatContext *format{};AVCodecContext *video{};AVFrame *frame{};AVPacket *packet{};
    BINKFRAMEBUFFERS *planes{};std::vector<std::unique_ptr<MovieTrack>> tracks;
    Clock::time_point origin{Clock::now()},pausedAt{};bool paused{},decoded{},bad{};unsigned ioControl{};std::uint64_t videoMicros{},audioMicros{},readMicros{};
    ~Movie(){tracks.clear();av_packet_free(&packet);av_frame_free(&frame);avcodec_free_context(&video);avformat_close_input(&format);if(io){av_freep(&io->buffer);avio_context_free(&io);}}
    static int Read(void *p,std::uint8_t *destination,int count){auto &m=*static_cast<Movie *>(p);if(count<=0)return AVERROR(EINVAL);const auto length=(std::min)(static_cast<std::size_t>(count),m.input.size()-m.cursor);if(!length)return AVERROR_EOF;std::memcpy(destination,m.input.data()+m.cursor,length);m.cursor+=length;return static_cast<int>(length);}
    static std::int64_t Seek(void *p,std::int64_t offset,int whence){auto &m=*static_cast<Movie *>(p);if(whence==AVSEEK_SIZE)return m.input.size();whence&=~AVSEEK_FORCE;std::int64_t base=whence==SEEK_SET?0:(whence==SEEK_CUR?m.cursor:(whence==SEEK_END?m.input.size():-1));if(base<0 || offset>static_cast<std::int64_t>(m.input.size())-base || offset<-base)return AVERROR(EINVAL);m.cursor=base+offset;return m.cursor;}
    bool Open(){
        auto *buffer=static_cast<std::uint8_t *>(av_malloc(32768));if(!buffer)return Fail("Bink AVIO allocation failed");
        io=avio_alloc_context(buffer,32768,0,this,Read,nullptr,Seek);if(!io){av_free(buffer);return Fail("Bink AVIO creation failed");}
        format=avformat_alloc_context();if(!format)return Fail("Bink demux allocation failed");format->pb=io;format->flags|=AVFMT_FLAG_CUSTOM_IO;
        const auto *demux=av_find_input_format("bink");if(!demux)return Fail("actual Bink demux is not built");int status=avformat_open_input(&format,nullptr,demux,nullptr);if(status<0)return AvFail("Bink demux open",status);
        if(!format->nb_streams || format->streams[0]->codecpar->codec_id!=AV_CODEC_ID_BINKVIDEO)return Fail("input is not supported Bink1 video");
        auto *st=format->streams[0];auto *decoder=avcodec_find_decoder(st->codecpar->codec_id);if(!decoder)return Fail("actual Bink video decoder is not built");video=avcodec_alloc_context3(decoder);if(!video)return Fail("Bink video allocation failed");
        if((status=avcodec_parameters_to_context(video,st->codecpar))<0 || (status=avcodec_open2(video,decoder,nullptr))<0)return AvFail("Bink video init",status);
        if(st->duration<=0 || st->duration>1000000 || video->width<=0 || video->height<=0 || video->width>7680 || video->height>4800 || st->time_base.num<=0 || st->time_base.den<=0)return Fail("invalid Bink video dimensions/frame clock");
        const auto durationMs=av_rescale_q(st->duration,st->time_base,AVRational{1,1000});if(durationMs<0 || durationMs>UINT_MAX)return Fail("Bink duration exceeds canonical millisecond clock");
        publicView.Width=video->width;publicView.Height=video->height;publicView.Frames=st->duration;publicView.FrameNum=1;publicView.FrameRate=st->time_base.den;publicView.FrameRateDiv=st->time_base.num;publicView.nativeOwner=this;
        for(unsigned i=1;i<format->nb_streams;++i){auto *a=format->streams[i];if(std::find(selected.begin(),selected.end(),static_cast<unsigned>(a->id))==selected.end())continue;
            if(a->codecpar->codec_id!=AV_CODEC_ID_BINKAUDIO_DCT && a->codecpar->codec_id!=AV_CODEC_ID_BINKAUDIO_RDFT)return Fail("unsupported Bink audio codec");
            auto *d=avcodec_find_decoder(a->codecpar->codec_id);if(!d)return Fail("actual Bink audio decoder is not built");auto t=std::make_unique<MovieTrack>();t->engine=audioEngine;t->destination=audioOutput;t->id=a->id;t->rate=a->codecpar->sample_rate;t->channels=a->codecpar->ch_layout.nb_channels;
            if(!t->rate || !t->channels || t->channels>2)return Fail("unsupported actual Bink audio topology");t->codec=avcodec_alloc_context3(d);if(!t->codec)return Fail("Bink audio allocation failed");
            if((status=avcodec_parameters_to_context(t->codec,a->codecpar))<0 || (status=avcodec_open2(t->codec,d,nullptr))<0)return AvFail("Bink audio init",status);
            if(!t->Create())return false;tracks.push_back(std::move(t));
        }
        frame=av_frame_alloc();packet=av_packet_alloc();if(!frame || !packet)return Fail("Bink packet/frame allocation failed");return true;
    }
    bool CopyPlane(const std::uint8_t *src,int stride,BINKPLANE &dst,unsigned width,unsigned height){if(!src || stride<static_cast<int>(width) || !dst.Buffer || dst.BufferPitch<width)return Fail("invalid real Bink plane buffer/pitch");for(unsigned y=0;y<height;++y)std::memcpy(static_cast<std::uint8_t *>(dst.Buffer)+static_cast<std::size_t>(y)*dst.BufferPitch,src+static_cast<std::size_t>(y)*stride,width);return true;}
    bool Decode(){
        if(bad)return false;if(decoded)return true;if(!planes)return Fail("Bink output buffers were not registered");
        for(;;){const auto readStart=Clock::now();int status=av_read_frame(format,packet);readMicros+=std::chrono::duration_cast<std::chrono::microseconds>(Clock::now()-readStart).count();if(status<0)return AvFail("Bink next packet",status);
            const int stream=packet->stream_index;AVCodecContext *target=stream==0?video:nullptr;MovieTrack *track=nullptr;
            if(stream>0)for(auto &t:tracks)if(t->id==static_cast<unsigned>(format->streams[stream]->id)){target=t->codec;track=t.get();break;}
            if(!target){av_packet_unref(packet);continue;}
            const auto decodeStart=Clock::now();status=avcodec_send_packet(target,packet);av_packet_unref(packet);if(status<0)return AvFail("Bink decode packet",status);
            bool gotVideo=false;
            while((status=avcodec_receive_frame(target,frame))>=0){
                if(track){if(!track->Queue(frame,paused)){av_frame_unref(frame);return false;}}
                else {if(frame->format!=AV_PIX_FMT_YUV420P && frame->format!=AV_PIX_FMT_YUVA420P){av_frame_unref(frame);return Fail("unsupported Bink video pixel format");}
                    const unsigned index=publicView.LastFrameNum%2;auto &p=planes->Frames[index];
                    const bool copied=CopyPlane(frame->data[0],frame->linesize[0],p.YPlane,publicView.Width,publicView.Height) && CopyPlane(frame->data[2],frame->linesize[2],p.cRPlane,(publicView.Width+1)/2,(publicView.Height+1)/2) && CopyPlane(frame->data[1],frame->linesize[1],p.cBPlane,(publicView.Width+1)/2,(publicView.Height+1)/2) && (frame->format!=AV_PIX_FMT_YUVA420P || CopyPlane(frame->data[3],frame->linesize[3],p.APlane,publicView.Width,publicView.Height));
                    if(!copied){av_frame_unref(frame);return false;}planes->FrameNum=index;gotVideo=true;
                }av_frame_unref(frame);
            }
            if(status!=AVERROR(EAGAIN) && status!=AVERROR_EOF)return AvFail("Bink decode frame",status);
            const auto elapsed=std::chrono::duration_cast<std::chrono::microseconds>(Clock::now()-decodeStart).count();if(track)audioMicros+=elapsed;else videoMicros+=elapsed;
            if(gotVideo){publicView.LastFrameNum=publicView.FrameNum;decoded=true;return true;}
        }
    }
    bool Loop(){int status=av_seek_frame(format,0,0,AVSEEK_FLAG_BACKWARD);if(status<0)return AvFail("Bink loop seek",status);avcodec_flush_buffers(video);
        for(auto &t:tracks){avcodec_flush_buffers(t->codec);if(t->voice){t->voice->DestroyVoice();t->voice=nullptr;}t->buffers.clear();t->submitted=0;if(!t->Create())return false;}
        publicView.FrameNum=1;publicView.LastFrameNum=0;origin=Clock::now();if(paused)pausedAt=origin;decoded=false;return true;}
};
Movie *Get(HBINK h){if(!h || !h->nativeOwner){Fail("Bink handle is uninitialized");return nullptr;}return static_cast<Movie *>(h->nativeOwner);}
std::int64_t Elapsed(Movie &m){return std::chrono::duration_cast<std::chrono::microseconds>((m.paused?m.pausedAt:Clock::now())-m.origin).count();}
}
namespace bo1::android {
bool BindBinkAudio(IXAudio2 *engine,IXAudio2Voice *output){error.clear();if(!engine || !output || engine->Failure())return Fail("actual Bink game sound graph unavailable");audioEngine=engine;audioOutput=output;return true;}
HBINK OpenBinkMemory(const void *data,std::size_t length,unsigned flags,std::size_t budget){error.clear();
    if(!data || length<44 || length>budget || length>static_cast<std::size_t>(INT_MAX)) {Fail("Bink input exceeds real configured budget or is truncated");return nullptr;}
    const auto *b=static_cast<const std::uint8_t *>(data);if(std::memcmp(b,"BIK",3) || b[3]!='i' || static_cast<std::uint64_t>(AV_RL32(b+4))+8!=length){Fail("Bink input requires exact supported BIKi file size");return nullptr;}
    try {auto m=std::make_unique<Movie>();m->input.assign(b,b+length);m->publicView.OpenFlags=flags;if(!m->Open())return nullptr;auto *h=&m->publicView;m.release();return h;}
    catch(const std::bad_alloc &){Fail("actual Bink owned input/decoder allocation failed");return nullptr;}
}
HBINK OpenBinkFile(const char *path,unsigned flags,std::size_t budget){
    error.clear();if(!path){Fail("Bink path is null");return nullptr;}
    using File=std::unique_ptr<FILE,decltype(&std::fclose)>;File file(std::fopen(path,"rb"),std::fclose);if(!file){Fail("actual Bink file open failed");return nullptr;}
    if(std::fseek(file.get(),0,SEEK_END)){Fail("actual Bink file size seek failed");return nullptr;}const auto length=std::ftell(file.get());
    if(length<44 || static_cast<std::uint64_t>(length)>budget || length>INT_MAX || std::fseek(file.get(),0,SEEK_SET)){Fail("actual Bink input size exceeds configured budget");return nullptr;}
    try {auto m=std::make_unique<Movie>();m->input.resize(length);
        if(std::fread(m->input.data(),1,m->input.size(),file.get())!=m->input.size()){Fail("actual Bink file read was incomplete");return nullptr;}
        file.reset();const auto *b=m->input.data();if(std::memcmp(b,"BIK",3) || b[3]!='i' || static_cast<std::uint64_t>(AV_RL32(b+4))+8!=m->input.size()){Fail("Bink file requires exact supported BIKi input");return nullptr;}
        m->publicView.OpenFlags=flags;if(!m->Open())return nullptr;auto *h=&m->publicView;m.release();return h;
    }catch(const std::bad_alloc &){Fail("actual Bink input allocation failed");return nullptr;}
}
std::size_t BinkOwnedBytes(HBINK h){auto *m=Get(h);return m?m->input.size():0;}
}
const char *BinkGetError(){return error.c_str();}
void BinkClose(HBINK h){if(!h)return;auto *m=Get(h);delete m;}
int BinkDoFrame(HBINK h){auto *m=Get(h);if(!m)return 0;try{if(m->Decode())return 1;}catch(const std::bad_alloc &){Fail("actual Bink decoded storage allocation failed");}m->bad=true;return 0;}
void BinkNextFrame(HBINK h){auto *m=Get(h);if(!m || m->bad)return;if(!m->decoded){Fail("Bink next frame requires actual decoded current frame");m->bad=true;return;}if(h->FrameNum==h->Frames){if(!m->Loop())m->bad=true;}else{++h->FrameNum;m->decoded=false;}}
int BinkPause(HBINK h,int pause){auto *m=Get(h);if(!m)return 0;const bool wanted=pause!=0;if(wanted!=m->paused){if(wanted)m->pausedAt=Clock::now();else m->origin+=Clock::now()-m->pausedAt;m->paused=wanted;for(auto &t:m->tracks){if((wanted?t->voice->Stop(0,0):t->voice->Start(0,0))!=S_OK){Fail("actual Bink track pause failed");m->bad=true;return 0;}}}return 1;}
int BinkWait(HBINK h){auto *m=Get(h);if(!m)return 0;return m->paused || Elapsed(*m)<av_rescale(h->FrameNum-1,1000000LL*h->FrameRateDiv,h->FrameRate);}
int BinkShouldSkip(HBINK h){auto *m=Get(h);return m && !m->paused && Elapsed(*m)>av_rescale(h->FrameNum,1000000LL*h->FrameRateDiv,h->FrameRate);}
void BinkGetFrameBuffersInfo(HBINK h,BINKFRAMEBUFFERS *out){auto *m=Get(h);if(!m || !out){Fail("Bink frame info requires valid output");return;}*out={};out->TotalFrames=2;out->YABufferWidth=h->Width;out->YABufferHeight=h->Height;out->cRcBBufferWidth=(h->Width+1)/2;out->cRcBBufferHeight=(h->Height+1)/2;for(auto &p:out->Frames){p.YPlane.Allocate=p.cRPlane.Allocate=p.cBPlane.Allocate=1;p.APlane.Allocate=(m->video->extradata_size>=4 && (AV_RL32(m->video->extradata)&0x00100000))?1:0;}}
void BinkRegisterFrameBuffers(HBINK h,BINKFRAMEBUFFERS *p){auto *m=Get(h);if(!m || !p || p->TotalFrames!=2 || p->YABufferWidth!=h->Width || p->YABufferHeight!=h->Height || p->cRcBBufferWidth!=(h->Width+1)/2 || p->cRcBBufferHeight!=(h->Height+1)/2){Fail("Bink registration requires both actual frame buffers");return;}m->planes=p;}
void BinkGetRealtime(HBINK h,BINKREALTIME *out,unsigned){auto *m=Get(h);if(!m || !out){Fail("Bink realtime requires valid output");return;}*out={};out->FrameNum=h->FrameNum;out->FrameRate=h->FrameRate;out->FrameRateDiv=h->FrameRateDiv;out->Frames=h->LastFrameNum;out->FramesTime=Elapsed(*m)/1000;out->FramesVideoDecompTime=m->videoMicros/1000;out->FramesAudioDecompTime=m->audioMicros/1000;out->FramesReadTime=m->readMicros/1000;out->ReadBufferSize=out->ReadBufferUsed=m->input.size();}
int BinkControlBackgroundIO(HBINK h,unsigned control){auto *m=Get(h);if(!m || (control!=1 && control!=2)){Fail("unsupported Bink input control");return 0;}m->ioControl=control;return 1; /* Actual complete owned input: no outstanding disk reads to suspend. */}
void BinkSetSoundTrack(unsigned count,unsigned *ids){error.clear();if(count>256 || (count && !ids)){Fail("invalid actual Bink selected tracks");return;}try{selected.clear();if(count)selected.assign(ids,ids+count);}catch(const std::bad_alloc &){Fail("Bink track selection allocation failed");}}
void BinkSetVolume(HBINK h,unsigned id,int volume){auto *m=Get(h);if(!m || volume<0 || volume>32768){Fail("invalid actual Bink track volume");return;}for(auto &t:m->tracks)if(t->id==id){t->volume=volume/32768.0f;if(t->voice->SetVolume(t->volume,0)!=S_OK){Fail("actual Bink track volume failed");m->bad=true;}}}
