#define SDL_MAIN_HANDLED
#include <platform/android/bink_service.h>
#if !defined(__ANDROID__)
#include <platform/android/faudio_bridge.h>
#include <platform/android/window_lifecycle.h>
#include <SDL.h>
#endif
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <array>
#include <cstring>
#include <thread>
#include <chrono>
#include <cmath>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"FAIL Bink line%d (%s)\n",__LINE__,BinkGetError());std::abort();}}while(0)
static void Video(const char *path,bool whole){
    BinkSetSoundTrack(0,nullptr);CHECK(!bo1::android::OpenBinkFile(path,0,1));
    auto *rejected=bo1::android::OpenBinkFile(path,0,256*1024*1024);CHECK(rejected);BINKFRAMEBUFFERS wrong{};BinkGetFrameBuffersInfo(rejected,&wrong);--wrong.YABufferWidth;BinkRegisterFrameBuffers(rejected,&wrong);CHECK(!BinkDoFrame(rejected) && rejected->LastFrameNum==0);BinkClose(rejected);
    auto *h=bo1::android::OpenBinkFile(path,0,256*1024*1024);CHECK(h);BINKFRAMEBUFFERS fb{};BinkGetFrameBuffersInfo(h,&fb);CHECK(fb.TotalFrames==2);
    std::array<std::array<std::vector<unsigned char>,4>,2> storage;
    for(unsigned i=0;i<2;++i){std::array<BINKPLANE *,4> planes{&fb.Frames[i].YPlane,&fb.Frames[i].cRPlane,&fb.Frames[i].cBPlane,&fb.Frames[i].APlane};for(unsigned p=0;p<4;++p){if(!planes[p]->Allocate)continue;const unsigned width=(p==0||p==3)?fb.YABufferWidth:fb.cRcBBufferWidth,height=(p==0||p==3)?fb.YABufferHeight:fb.cRcBBufferHeight;storage[i][p].resize(static_cast<std::size_t>(width+16)*height+32,0xA5);planes[p]->Buffer=storage[i][p].data();planes[p]->BufferPitch=width+16;}}
    BinkRegisterFrameBuffers(h,&fb);CHECK(BinkDoFrame(h));CHECK(h->LastFrameNum==1);const auto first=storage[0][0];
    CHECK(BinkPause(h,1) && BinkWait(h));std::this_thread::sleep_for(std::chrono::milliseconds(20));CHECK(BinkWait(h));CHECK(BinkPause(h,0));
    const unsigned frames=whole?h->Frames:(std::min)(h->Frames,3u);
    for(unsigned n=1;n<frames;++n){BinkNextFrame(h);CHECK(BinkDoFrame(h));CHECK(h->LastFrameNum==n+1);}
    for(unsigned i=0;i<2;++i)for(unsigned p=0;p<4;++p){const auto &data=storage[i][p];if(data.empty())continue;const unsigned width=(p==0||p==3)?fb.YABufferWidth:fb.cRcBBufferWidth,height=(p==0||p==3)?fb.YABufferHeight:fb.cRcBBufferHeight;for(unsigned y=0;y<height;++y)for(unsigned x=width;x<width+16;++x)CHECK(data[y*(width+16)+x]==0xA5);for(std::size_t n=static_cast<std::size_t>(width+16)*height;n<data.size();++n)CHECK(data[n]==0xA5);}
    if(whole){BinkNextFrame(h);CHECK(h->FrameNum==1);CHECK(BinkDoFrame(h));CHECK(storage[0][0]==first);}
    BINKREALTIME stats{};BinkGetRealtime(h,&stats,0);CHECK(stats.ReadBufferSize==bo1::android::BinkOwnedBytes(h) && stats.ReadBufferUsed==stats.ReadBufferSize);CHECK(BinkControlBackgroundIO(h,1));CHECK(BinkControlBackgroundIO(h,2));
    printf("PASS actual retail BIKi %ux%u video%u frames/pitched YCrCbA canaries/pause/owned%u input%s\n",h->Width,h->Height,frames,stats.ReadBufferSize,whole?"/exact loop-first-frame":"");BinkClose(h);BinkClose(nullptr);
}
static void AudioCodec(const char *path){
    AVFormatContext *format{};CHECK(avformat_open_input(&format,path,av_find_input_format("bink"),nullptr)>=0);
    // The fixture uses a real file protocol only in its standalone test recipe.
    std::vector<AVCodecContext *> codecs(format->nb_streams);unsigned tracks=0;std::uint64_t samples=0;double energy=0;
    for(unsigned i=1;i<format->nb_streams;++i){auto *d=avcodec_find_decoder(format->streams[i]->codecpar->codec_id);CHECK(d);codecs[i]=avcodec_alloc_context3(d);CHECK(codecs[i] && avcodec_parameters_to_context(codecs[i],format->streams[i]->codecpar)>=0 && avcodec_open2(codecs[i],d,nullptr)>=0);++tracks;}
    auto *packet=av_packet_alloc();auto *frame=av_frame_alloc();CHECK(packet && frame);
    unsigned count=0;while(count++<30 && av_read_frame(format,packet)>=0){auto *codec=codecs[packet->stream_index];if(codec){CHECK(avcodec_send_packet(codec,packet)>=0);int status;while((status=avcodec_receive_frame(codec,frame))>=0){CHECK(frame->format==AV_SAMPLE_FMT_FLTP);for(int c=0;c<frame->ch_layout.nb_channels;++c)for(int n=0;n<frame->nb_samples;++n){float value=reinterpret_cast<float *>(frame->extended_data[c])[n];CHECK(std::isfinite(value));energy+=value*value;++samples;}av_frame_unref(frame);}CHECK(status==AVERROR(EAGAIN)||status==AVERROR_EOF);}av_packet_unref(packet);}
    CHECK(tracks && samples && energy>0);printf("PASS actual retail Bink DCT%u tracks/%llu decoded float samples/nonzero energy; codec-only no audio device\n",tracks,(unsigned long long)samples);
    av_frame_free(&frame);av_packet_free(&packet);for(auto *c:codecs)avcodec_free_context(&c);avformat_close_input(&format);
}
#if !defined(__ANDROID__)
void Sys_SetQuitRequested(){std::abort();}
static void Input(const SDL_Event &,void *){}
static void MovieAudio(const char *path){
    SDL_SetMainReady();bo1::android::InstallSDLEventOwner(Input,nullptr);CHECK(SDL_SetHint(SDL_HINT_AUDIODRIVER,"dummy"));
    IXAudio2 *engine{};CHECK(XAudio2Create(&engine,0,XAUDIO2_ANY_PROCESSOR)==S_OK);IXAudio2MasteringVoice *master{};CHECK(engine->CreateMasteringVoice(&master,2,48000,0,0,nullptr)==S_OK);
    IXAudio2SubmixVoice *bus{};XAUDIO2_SEND_DESCRIPTOR send{0,master};XAUDIO2_VOICE_SENDS sends{1,&send};CHECK(engine->CreateSubmixVoice(&bus,4,48000,0,0,&sends,nullptr)==S_OK);
    CHECK(bo1::android::BindBinkAudio(engine,bus));unsigned ids[]{0,1,2,3,4};BinkSetSoundTrack(5,ids);auto *h=bo1::android::OpenBinkFile(path,0,256*1024*1024);CHECK(h);
    BINKFRAMEBUFFERS fb{};BinkGetFrameBuffersInfo(h,&fb);std::array<std::array<std::vector<unsigned char>,4>,2> storage;
    for(unsigned i=0;i<2;++i){std::array<BINKPLANE *,4> p{&fb.Frames[i].YPlane,&fb.Frames[i].cRPlane,&fb.Frames[i].cBPlane,&fb.Frames[i].APlane};for(unsigned c=0;c<4;++c)if(p[c]->Allocate){const unsigned w=(c==0||c==3)?fb.YABufferWidth:fb.cRcBBufferWidth,y=(c==0||c==3)?fb.YABufferHeight:fb.cRcBBufferHeight;storage[i][c].resize(w*y);p[c]->Buffer=storage[i][c].data();p[c]->BufferPitch=w;}}
    BinkRegisterFrameBuffers(h,&fb);CHECK(engine->StartEngine()==S_OK);CHECK(BinkDoFrame(h));BinkSetVolume(h,0,23167);BinkSetVolume(h,1,23167);CHECK(!*BinkGetError());CHECK(BinkPause(h,1));std::this_thread::sleep_for(std::chrono::milliseconds(20));CHECK(BinkPause(h,0));
    for(unsigned n=1;n<h->Frames;++n){BinkNextFrame(h);while(BinkWait(h))std::this_thread::sleep_for(std::chrono::milliseconds(1));CHECK(BinkDoFrame(h));}
    BinkNextFrame(h);CHECK(h->FrameNum==1);CHECK(BinkDoFrame(h));
    // Close with newly submitted loop audio still active. DestroyVoice must quiesce
    // callbacks before packet owners free, and engine's live-voice count must retire.
    BinkClose(h);storage={};std::this_thread::sleep_for(std::chrono::milliseconds(25));CHECK(!engine->Failure());bus->DestroyVoice();master->DestroyVoice();engine->Release();bo1::android::RetireSDLEventOwner();
    puts("PASS actual FAudio Bink movie tracks/volume/pause/paced full300frame playback/loop/active callback retirement; SDL dummy output no physical speaker proof");
}
#endif
int main(int argc,char **argv){CHECK(argc>=3);std::array<unsigned char,44> bad{};CHECK(!bo1::android::OpenBinkMemory(bad.data(),bad.size(),0,1024));CHECK(!bo1::android::BindBinkAudio(nullptr,nullptr));Video(argv[1],true);Video(argv[2],false);AudioCodec(argv[2]);
#if !defined(__ANDROID__)
    CHECK(argc==4);MovieAudio(argv[3]);
#endif
}
