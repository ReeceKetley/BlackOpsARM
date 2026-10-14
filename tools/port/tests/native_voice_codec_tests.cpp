#define SDL_MAIN_HANDLED
#include <platform/android/voice_audio.h>
#include <platform/android/window_lifecycle.h>
#include <groupvoice/encode.h>
#include <groupvoice/decode.h>
#include <groupvoice/record.h>
#include <universal/dvar.h>
#include <win32/win_voice.h>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <array>
#include <vector>
#include <cmath>
#include <cstring>
static void Check(bool value,int line){if(!value){fprintf(stderr,"FAIL voice fixture line%d\n",line);std::abort();}}
#define CHECK(x) Check((x),__LINE__)
static dvar_t quality{};
const dvar_t *sv_voiceQuality=&quality;

static std::vector<std::vector<char>> packets;
// Isolated downstream consumer and explicitly enabled test transmit gate;
// these are not network/player/input/voice-initialization readiness substitutes.
char Voice_SendVoiceData(){return 1;}
bool IN_IsTalkKeyHeld(){return true;}
void CL_VoiceTransmit(int){}
int Client_SendVoiceData(int bytes,char *data){CHECK(bytes>0);packets.emplace_back(data,data+bytes);return bytes;}
void Sys_SetQuitRequested(){std::abort();}
void Com_Printf(int,const char *format,...){va_list args;va_start(args,format);vfprintf(stderr,format,args);va_end(args);}
void Com_PrintError(int,const char *format,...){va_list args;va_start(args,format);vfprintf(stderr,format,args);va_end(args);}
bool Assert_MyHandler(const char *file,int line,int,const char *,...){fprintf(stderr,"engine assert %s:%d\n",file,line);std::abort();}
static void Input(const SDL_Event &,void *){}
static void Codec(){
    quality.current.integer=1;CHECK(!Voice_HasOwnedResources());CHECK(Encode_Init(0));CHECK(Decode_Init(0));CHECK(Voice_HasOwnedResources());CHECK(Encode_GetFrameSize()==160 && g_decode_frame_size==160);
    CHECK(!Encode_Init(0) && !Decode_Init(0));
    std::array<short,321> input{};for(unsigned i=0;i<input.size();++i)input[i]=static_cast<short>(9000*std::sin(i*.19));
    audioSample_t capture{};capture.bytesPerSample=2;capture.channels=1;
    capture.buffer=reinterpret_cast<unsigned char *>(input.data());capture.lengthInSamples=159;capture.lengthInBytes=318;
    CHECK(Record_QueueAudioDataForEncoding(&capture)==0 && packets.empty());
    capture.buffer=reinterpret_cast<unsigned char *>(input.data()+159);capture.lengthInSamples=162;capture.lengthInBytes=324;
    CHECK(Record_QueueAudioDataForEncoding(&capture)>0 && packets.size()==2);
    std::array<short,168> decoded{};bool nonzero=false;
    for(auto &packet:packets){decoded.fill(12345);CHECK(Decode_Sample(packet.data(),packet.size(),decoded.data(),160)==320);for(unsigned i=160;i<decoded.size();++i)CHECK(decoded[i]==12345);for(unsigned i=0;i<160;++i)nonzero|=decoded[i]!=0;}
    CHECK(nonzero);CHECK(Decode_Sample(nullptr,1,decoded.data(),160)==0);CHECK(Decode_Sample(packets[0].data(),packets[0].size(),decoded.data(),159)==0);
    char tooSmall{};CHECK(Encode_Sample(input.data(),&tooSmall,1)<0);
    quality.current.integer=11;std::array<char,256> encoded{};CHECK(Encode_Sample(input.data(),encoded.data(),encoded.size())<0);CHECK(Encode_Sample(input.data(),encoded.data(),encoded.size())<0);quality.current.integer=1;CHECK(Encode_Sample(input.data(),encoded.data(),encoded.size())>0);
    Voice_Shutdown();CHECK(!Voice_HasOwnedResources());Voice_Shutdown();
    CHECK(Encode_Init(0)&&Decode_Init(0));packets.clear();capture.buffer=reinterpret_cast<unsigned char *>(input.data());capture.lengthInSamples=159;capture.lengthInBytes=318;
    CHECK(Record_QueueAudioDataForEncoding(&capture)==0 && packets.empty());Voice_Shutdown();CHECK(!Voice_HasOwnedResources());
    puts("PASS actual bundled Speex NB160 partial159+162 assembly/two canonical packets/320-byte decode/canaries/output bounds/actual partial-owner Voice_Shutdown/restart");
}
static void Ring(){
    bo1::android::VoiceCaptureRing ring;std::vector<short> input(30000);for(unsigned i=0;i<input.size();++i)input[i]=i;
    ring.Push(input);CHECK(ring.Overruns()==4400);std::vector<short> output(25600);CHECK(ring.Read({output.data(),12345})==12345);CHECK(ring.Read({output.data()+12345,output.size()-12345})==13255);
    for(unsigned i=0;i<output.size();++i)CHECK(output[i]==input[4400+i]);CHECK(ring.Read(output)==0);ring.Push(input);ring.Clear();CHECK(ring.Read(output)==0);
    puts("PASS actual capture ring wrap/25600-frame bound/overrun accounting/order/clear");
}
static void Conversion(){
    SDL_AudioStream *stream=SDL_NewAudioStream(AUDIO_S16SYS,1,48000,AUDIO_S16SYS,1,8192);CHECK(stream);
    std::vector<short> input(48000);for(unsigned i=0;i<input.size();++i)input[i]=static_cast<short>(9000*std::sin(i*.035));
    CHECK(SDL_AudioStreamPut(stream,input.data(),input.size()*2)==0);
    const int bytes=SDL_AudioStreamAvailable(stream);printf("Actual SDL converted bytes=%d\n",bytes);CHECK(bytes>0 && !(bytes%2));
    std::vector<short> output(bytes/2+8,12345);CHECK(SDL_AudioStreamGet(stream,output.data(),bytes)==bytes);
    bool nonzero=false;for(int i=0;i<bytes/2;++i)nonzero|=output[i]!=0;CHECK(nonzero);for(unsigned i=bytes/2;i<output.size();++i)CHECK(output[i]==12345);
    CHECK(SDL_AudioStreamPut(stream,input.data(),input.size()*2)==0);
    const int next=SDL_AudioStreamAvailable(stream);printf("Actual SDL continuous converted bytes=%d\n",next);CHECK(next>=15360 && next<=16400 && !(next%2));
    SDL_AudioStreamClear(stream);CHECK(SDL_AudioStreamAvailable(stream)==0);SDL_FreeAudioStream(stream);
    puts("PASS actual SDL48000-to-canonical8192 PCM conversion/nonzero/output canaries/clear; synthetic codec input only");
}
int main(){SDL_SetMainReady();Codec();Ring();Conversion();bo1::android::VoiceCapture capture;CHECK(!capture.Open(8192,1));CHECK(capture.Read({})==-1);
    bo1::android::InstallSDLEventOwner(Input,nullptr);
#if defined(__ANDROID__)
    CHECK(!capture.Open(8192,1));CHECK(strstr(SDL_GetError(),"actual SDL Java setup"));
    puts("PASS actual missing-Java voice capture refusal before device access; no Android microphone opened");
#endif
    bo1::android::RetireSDLEventOwner();}
