#include "native_jpeg.h"
#include <vpx/vpx.h>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <climits>
#include <unistd.h>
#include <string>
#include <mutex>
#include <new>
namespace {
struct NativeAvi {FILE *file{};std::string target,temporary;bool failed{};};
std::mutex aviOwner;
int activeCodec=-1;bool vpxAudioNotice;
NativeAvi *AviState(const avi_s *avi){return static_cast<NativeAvi*>(avi->nativeState);}
bool AviWrite(NativeAvi *state,const void *bytes,std::size_t size){if(!state || !state->file || state->failed)return false;if(std::fwrite(bytes,1,size,state->file)!=size){state->failed=true;return false;}return true;}
void AviFailure(){Com_PrintError(16,"Native AVI recording failed: file/codec/capture resource operation did not complete\n");}
}
avi_s::~avi_s(){close();delete AviState(this);nativeState=nullptr;}
bool avi_s::native_good()const{auto *s=AviState(this);return s && !s->failed && s->file;}
void avi_s::create(const char *name,unsigned width,unsigned height,unsigned fps){
    if(nativeState || !name || !*name || !width || !height || (width&1) || (height&1) || !fps || fps>1000000 || width>65535 || height>65535 || std::uint64_t(width)*height>UINT32_MAX/3)return;
    NativeAvi *state=nullptr;
    try{state=new NativeAvi;state->target=name;state->temporary=state->target+".tmp.XXXXXX";}catch(const std::bad_alloc&){delete state;return;}
    int fd=mkstemp(state->temporary.data());if(fd<0){delete state;return;}
    state->file=fdopen(fd,"w+b");if(!state->file){::close(fd);unlink(state->temporary.c_str());delete state;return;}
    nativeState=state;hFile=state->file;audioBufferCount=0;fileSize=0;
    riff.size=riff.list.size=riff.list.avih.size=0;riff.list.strl_vid.strf.sizeBi=40;
    riff.list.strl_vid.size=riff.list.strl_vid.strh.size=riff.list.strl_vid.strf.size=0;
    riff.list.strl_aud.size=riff.list.strl_aud.strh.size=riff.list.strl_aud.strf.size=0;
    riff.Setup(width,height,fps);unsigned char *now=buffer;riff.Out(&now);
    output_data(buffer,static_cast<unsigned>(now-buffer));output_data(reinterpret_cast<unsigned char*>(const_cast<char*>("LISTXXXXmovi")),12);
}
unsigned avi_s::output_data(unsigned char *data,unsigned size){auto *s=AviState(this);const unsigned offset=fileSize;if(!s || (!data&&size) || size>UINT32_MAX-fileSize){if(s)s->failed=true;return offset;}if(AviWrite(s,data,size))fileSize+=size;return offset;}
void avi_s::add_frame(unsigned char *y,unsigned char *u,unsigned char *v){auto *s=AviState(this);if(!native_good())return;if(riff.idx1.count>=8191){s->failed=true;return;}if(std::uint64_t(riff.list.avih.TotalFrames)>std::uint64_t(riff.list.strl_vid.strh.Rate)*(std::uint64_t(audioBufferCount)+riff.list.strl_aud.strh.Length)/24000)return;
    unsigned char *jpeg=nullptr;std::size_t size=0;if(!MjpegEncodePlanes(y,u,v,riff.list.avih.Width,riff.list.avih.Height,&jpeg,&size)){s->failed=true;return;}
    const unsigned padded=static_cast<unsigned>((size+3)&~std::size_t(3));unsigned char header[8]={'0','0','d','c'};unsigned char *p=header+4;avi::Out32(&p,padded);const unsigned offset=output_data(header,8);output_data(jpeg,static_cast<unsigned>(size));unsigned char pad[3]{};output_data(pad,padded-static_cast<unsigned>(size));std::free(jpeg);if(!s->failed)riff.AddFrame(offset,padded);
}
void avi_s::flush_samples(){auto *s=AviState(this);if(!native_good() || !audioBufferCount)return;if(riff.idx1.count>=8192){s->failed=true;return;}unsigned char header[8]={'0','1','w','b'};unsigned char *p=header+4;const unsigned bytes=audioBufferCount*2;avi::Out32(&p,bytes);const unsigned offset=output_data(header,8);output_data(reinterpret_cast<unsigned char*>(audioBuffer),bytes);if(!s->failed){riff.idx1.offsets[riff.idx1.count]=offset;riff.idx1.sizes[riff.idx1.count++]=bytes;riff.list.strl_aud.strh.Length+=audioBufferCount;}audioBufferCount=0;}
void avi_s::add_samples(short *input,int count){auto *s=AviState(this);if(!native_good())return;if(!input || count<0 || (count&1)){s->failed=true;return;}
    if(std::uint64_t(riff.list.avih.TotalFrames)<std::uint64_t(riff.list.strl_vid.strh.Rate)*(std::uint64_t(count/2)+audioBufferCount+riff.list.strl_aud.strh.Length)/24000)return;
    for(int i=0;i<count;i+=2){const unsigned short sample=static_cast<unsigned short>(input[i]);auto *p=reinterpret_cast<unsigned char*>(audioBuffer)+audioBufferCount*2;p[0]=sample;p[1]=sample>>8;++audioBufferCount;if(audioBufferCount==24000)flush_samples();if(s->failed)return;}}
void avi_s::close(){auto *s=AviState(this);if(!s||!s->file)return;flush_samples();const unsigned dataSize=fileSize;unsigned char *now=encodeBuff;riff.idx1.Out(&now);output_data(encodeBuff,static_cast<unsigned>(now-encodeBuff));now=encodeBuff;riff.Out(&now);const unsigned headerSize=static_cast<unsigned>(now-encodeBuff);unsigned char *size=encodeBuff+4;avi::Out32(&size,fileSize-8);
    if(!s->failed && fseeko(s->file,0,SEEK_SET)!=0)s->failed=true;
    AviWrite(s,encodeBuff,headerSize);std::memcpy(encodeBuff,"LISTXXXXmovi",12);size=encodeBuff+4;avi::Out32(&size,dataSize-headerSize-8);AviWrite(s,encodeBuff,12);
    if(fflush(s->file)!=0 || fsync(fileno(s->file))!=0)s->failed=true;
    if(fclose(s->file)!=0)s->failed=true;s->file=nullptr;hFile=nullptr;
    if(!s->failed && std::rename(s->temporary.c_str(),s->target.c_str())!=0)s->failed=true;
    if(s->failed)unlink(s->temporary.c_str());
}
void avi_s::write_thread(){Com_Error(ERR_FATAL,"Native AVI writes are owned by the canonical MJPEG encoder worker, not a second write-thread entry");std::abort();}
unsigned __stdcall mjpeg_write_thread(void*){Com_Error(ERR_FATAL,"Invalid independent native AVI write-thread entry");std::abort();}
void mjpeg_create(const char *name,int width,int height,unsigned fps){
    mjpeg_close();
    if(!r_clipCodec || (r_clipCodec->current.integer!=0&&r_clipCodec->current.integer!=1)){Com_PrintError(16,"Invalid recording codec selection; recording not started\n");return;}
    const int codec=r_clipCodec->current.integer;
    if(!name || !*name || width<=0 || height<=0 || width>(codec?16383:65500) || height>(codec?16383:65500) || (width&1) || (height&1) || !fps || fps>1000000){AviFailure();return;}
    yuv_init(width,height,true);int actualWidth=0,actualHeight=0;yuv_dimensions(&actualWidth,&actualHeight);
    if(!yuv_is_ready() || actualWidth!=width || actualHeight!=height){AviFailure();return;}
    bool okay=false;
    {std::lock_guard<std::mutex> lock(aviOwner);activeCodec=codec;vpxAudioNotice=false;
        if(codec==1)okay=vpx_init_native(name,width,height,fps);
        else{AVI=new(std::nothrow)avi_s{};if(AVI){AVI->create(name,width,height,fps);okay=AVI->native_good();}}}
    if(!okay){mjpeg_close_int();AviFailure();return;}
    mjpeg_start_encoding(width,height,fps);
    if(!mjpeg_is_encoding()){mjpeg_close_int();AviFailure();}
}
void mjpeg_add_frame(unsigned char *y,unsigned char *u,unsigned char *v){bool okay=false;
    {std::lock_guard<std::mutex> lock(aviOwner);if(activeCodec==1)okay=vpx_encode_native(y,u,v);else if(AVI){AVI->add_frame(y,u,v);okay=AVI->native_good();}}
    if(!okay){mjpeg_stop_encoding();AviFailure();}
}
void mjpeg_add_samples(short *samples,int count){if(!mjpeg_is_encoding())return;bool okay=true,notice=false;
    {std::lock_guard<std::mutex> lock(aviOwner);
        if(activeCodec==1){notice=!vpxAudioNotice;vpxAudioNotice=true;} // IVF is the canonical video-only VP8 format.
        else if(AVI){AVI->add_samples(samples,count);okay=AVI->native_good();}}
    if(notice)Com_PrintWarning(16,"VP8 IVF recording has no audio stream\n");
    if(!okay){mjpeg_stop_encoding();AviFailure();}
}
void mjpeg_close_int(){bool failed=false;
    {std::lock_guard<std::mutex> lock(aviOwner);if(activeCodec==1)failed=!vpx_shutdown_native();
        if(AVI){AVI->close();auto *s=AviState(AVI);failed=failed||(s&&s->failed);delete AVI;AVI=nullptr;}activeCodec=-1;}
    yuv_shutdown();if(failed)AviFailure();
}
bool mjpeg_writer_ready(int width,int height,unsigned fps){std::lock_guard<std::mutex> lock(aviOwner);
    if(activeCodec==1)return vpx_ready_for(width,height,fps);
    return AVI&&AVI->native_good()&&AVI->riff.list.avih.Width==unsigned(width)&&AVI->riff.list.avih.Height==unsigned(height)&&AVI->riff.list.strl_vid.strh.Rate==fps;
}
bool mjpeg_writer_uses_vpx(){std::lock_guard<std::mutex> lock(aviOwner);return activeCodec==1;}
