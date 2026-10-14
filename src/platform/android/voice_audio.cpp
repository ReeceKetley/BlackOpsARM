#include "voice_audio.h"
#include "controller_service.h"
#include "window_lifecycle.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <new>
#include <limits>
namespace bo1::android {
void VoiceCaptureRing::Push(std::span<const std::int16_t> input) {
    std::lock_guard lock(gate);
    for(auto frame:input) {if(count==frames.size()){first=(first+1)%frames.size();--count;++overruns;}frames[(first+count)%frames.size()]=frame;++count;}
}
std::size_t VoiceCaptureRing::Read(std::span<std::int16_t> output) {
    std::lock_guard lock(gate);const auto length=(std::min)(output.size(),count);
    for(std::size_t i=0;i<length;++i)output[i]=frames[(first+i)%frames.size()];
    first=(first+length)%frames.size();count-=length;return length;
}
void VoiceCaptureRing::Clear(){std::lock_guard lock(gate);first=count=0;}
std::uint64_t VoiceCaptureRing::Overruns(){std::lock_guard lock(gate);return overruns;}
namespace {
bool Owner() {if(IsSDLEventOwner())return true;SDL_SetError("voice device control requires actual SDL event owner");return false;}
bool JavaAndPermission() {
#if defined(__ANDROID__)
    if(!SDLJavaSetupComplete()){SDL_SetError("voice capture requires completed actual SDL Java setup");return false;}
    auto *env=static_cast<JNIEnv *>(SDL_AndroidGetJNIEnv());
    if(!env){SDL_SetError("voice capture JNI owner is unavailable");return false;}
    jobject activity=static_cast<jobject>(SDL_AndroidGetActivity());
    if(!activity){SDL_SetError("voice capture requires actual SDL Activity");return false;}
    jclass type=env->GetObjectClass(activity);
    auto check=type?env->GetMethodID(type,"checkSelfPermission","(Ljava/lang/String;)I"):nullptr;
    jstring permission=check?env->NewStringUTF("android.permission.RECORD_AUDIO"):nullptr;
    const bool granted=permission && env->CallIntMethod(activity,check,permission)==0 && !env->ExceptionCheck();
    if(env->ExceptionCheck())env->ExceptionClear();
    if(permission)env->DeleteLocalRef(permission);if(type)env->DeleteLocalRef(type);env->DeleteLocalRef(activity);
    if(!granted)SDL_SetError("actual RECORD_AUDIO permission is required before voice capture");
    return granted;
#else
    return true; // Host fixture opens actual explicitly selected SDL backend, never an Android permission claim.
#endif
}
}
void VoiceCapture::Callback(void *opaque,std::uint8_t *data,int bytes) {
    auto &owner=*static_cast<VoiceCapture *>(opaque);
    if(bytes<0 || bytes%2 || (bytes>0 && (!data || reinterpret_cast<std::uintptr_t>(data)%alignof(std::int16_t)))){owner.failed=true;return;}
    if(!bytes || owner.muted.load(std::memory_order_relaxed))return;
    if(!owner.conversion || SDL_AudioStreamPut(owner.conversion,data,bytes)<0){owner.failed=true;return;}
    std::array<std::int16_t,1024> converted{};
    for(;;) {
        const int available=SDL_AudioStreamAvailable(owner.conversion);
        if(available<0){owner.failed=true;return;}if(!available)break;
        const int length=SDL_AudioStreamGet(owner.conversion,converted.data(),sizeof(converted));
        if(length<=0 || length%2){owner.failed=true;return;}
        const auto scale=owner.gain.load(std::memory_order_relaxed)/65535.0f;
        for(int i=0;i<length/2;++i)converted[i]=static_cast<std::int16_t>(std::clamp(converted[i]*scale,-32768.0f,32767.0f));
        owner.ring.Push({converted.data(),static_cast<std::size_t>(length/2)});
    }
}
bool VoiceCapture::Open(unsigned rate,unsigned channelCount) {
    if(device || !Owner() || !JavaAndPermission())return false;
    if(!rate || rate>static_cast<unsigned>(INT_MAX) || channelCount!=1){SDL_SetError("canonical voice capture requires mono PCM16");return false;}
    if(!(SDL_WasInit(SDL_INIT_AUDIO)&SDL_INIT_AUDIO)){SDL_SetError("voice capture requires actual audio subsystem initialization");return false;}
    SDL_AudioSpec wanted{},obtained{};wanted.freq=48000;wanted.channels=channelCount;wanted.format=AUDIO_S16SYS;wanted.samples=512;wanted.callback=Callback;wanted.userdata=this;
    // Freeze the actually enumerated logical route and open that exact name.
    // Default unnamed routes remain usable but are not reported as a guessed device.
    const char *name=SDL_GetNumAudioDevices(1)>0?SDL_GetAudioDeviceName(0,1):nullptr;
    try {route=name?name:"";}catch(const std::bad_alloc &){SDL_SetError("voice route storage allocation failed");return false;}
    device=SDL_OpenAudioDevice(route.empty()?nullptr:route.c_str(),1,&wanted,&obtained,0);
    if(!device)return false;
    if(obtained.freq!=48000 || obtained.channels!=channelCount || obtained.format!=AUDIO_S16SYS) {
        SDL_CloseAudioDevice(device);device=0;SDL_SetError("voice capture app format differs from canonical PCM");return false;
    }
    conversion=SDL_NewAudioStream(obtained.format,obtained.channels,obtained.freq,AUDIO_S16SYS,1,static_cast<int>(rate));
    if(!conversion){SDL_CloseAudioDevice(device);device=0;return false;}
    return true;
}
VoiceCapture::~VoiceCapture(){if(device)SDL_CloseAudioDevice(device);if(conversion)SDL_FreeAudioStream(conversion);}
bool VoiceCapture::Live() const{return device && conversion && !failed.load(std::memory_order_acquire) && SDL_GetAudioDeviceStatus(device)!=SDL_AUDIO_STOPPED;}
bool VoiceCapture::Start(){if(!Owner() || !Live())return false;SDL_LockAudioDevice(device);SDL_AudioStreamClear(conversion);ring.Clear();SDL_UnlockAudioDevice(device);SDL_PauseAudioDevice(device,0);return SDL_GetAudioDeviceStatus(device)==SDL_AUDIO_PLAYING;}
bool VoiceCapture::Stop(){if(!Owner() || !Live())return false;SDL_PauseAudioDevice(device,1);SDL_LockAudioDevice(device);SDL_AudioStreamClear(conversion);ring.Clear();SDL_UnlockAudioDevice(device);return true;}
std::ptrdiff_t VoiceCapture::Read(std::span<std::int16_t> output){return Owner() && Live()?static_cast<std::ptrdiff_t>(ring.Read(output)):-1;}
bool VoiceCapture::Controls(unsigned level,bool mute){if(!Owner() || !Live() || level>65535)return false;const bool changed=muted.load(std::memory_order_relaxed)!=mute;SDL_LockAudioDevice(device);gain=level;muted=mute;if(changed){SDL_AudioStreamClear(conversion);ring.Clear();}SDL_UnlockAudioDevice(device);return true;}
bool VoiceCapture::Route(char *out,std::size_t capacity){if(!Owner() || !Live() || !out || !capacity)return false;if(route.empty() || route.size()>=capacity){SDL_SetError("Actual SDL capture route is unnamed or does not fit the requested metadata buffer");return false;}SDL_strlcpy(out,route.c_str(),capacity);return true;}
VoicePlayback::VoicePlayback(IXAudio2 *owner,IXAudio2Voice *destination,unsigned rate,unsigned channelCount):engine(owner),output(destination),frequency(rate),channels(channelCount){Create();}
bool VoicePlayback::Create(){
    if(!Owner() || !engine || !output || engine->Failure() || channels!=1 || !frequency)return false;
    WAVEFORMATEX format{};format.wFormatTag=FAUDIO_FORMAT_PCM;format.nChannels=channels;format.nSamplesPerSec=frequency;format.wBitsPerSample=16;format.nBlockAlign=channels*2;format.nAvgBytesPerSec=frequency*format.nBlockAlign;
    XAUDIO2_SEND_DESCRIPTOR send{0,output};XAUDIO2_VOICE_SENDS sends{1,&send};
    if(engine->CreateSourceVoice(&voice,&format,0,2,this,&sends,nullptr)!=S_OK)return false;
    XAUDIO2_VOICE_DETAILS details{};output->GetVoiceDetails(&details);
    if(!details.InputChannels || details.InputChannels>8){voice->DestroyVoice();voice=nullptr;return false;}
    std::array<float,8> matrix{};matrix[0]=1;if(details.InputChannels>1)matrix[1]=1;
    if(voice->SetOutputMatrix(output,1,details.InputChannels,matrix.data(),0)!=S_OK){voice->DestroyVoice();voice=nullptr;return false;}
    return true;
}
VoicePlayback::~VoicePlayback(){if(voice)voice->DestroyVoice();}
bool VoicePlayback::Live() const{return voice && !failed.load(std::memory_order_acquire) && engine && !engine->Failure();}
void VoicePlayback::OnBufferEnd(void *context){static_cast<Packet *>(context)->ended.store(true,std::memory_order_release);}
int VoicePlayback::BufferedBytes(){
    if(!Live())return -1;XAUDIO2_VOICE_STATE state{};voice->GetState(&state);
    if(state.SamplesPlayed>queuedFrames){failed=true;return -1;}
    return static_cast<int>((queuedFrames-state.SamplesPlayed)*channels*2);
}
int VoicePlayback::Queue(const std::int16_t *data,unsigned bytes,unsigned maximumBytes){
    if(!Owner() || !Live() || !data || reinterpret_cast<std::uintptr_t>(data)%alignof(std::int16_t) ||
        bytes%(channels*2) || maximumBytes%(channels*2) || maximumBytes>static_cast<unsigned>(INT_MAX))return -1;
    const int buffered=BufferedBytes();if(buffered<0 || static_cast<unsigned>(buffered)>maximumBytes)return -1;
    const unsigned accepted=(std::min)(bytes,maximumBytes-static_cast<unsigned>(buffered));if(!accepted)return 0;
    try {pending.insert(pending.end(),data,data+accepted/2);}
    catch(const std::bad_alloc &){return -1;}
    queuedFrames+=accepted/(channels*2);return accepted;
}
bool VoicePlayback::Frame(unsigned start,unsigned low,unsigned high,unsigned comfortable,float up,float down){
    if(!Owner() || !Live() || !start || low>comfortable || comfortable>high ||
        !std::isfinite(up) || !std::isfinite(down) || up<0 || up>=1 || down<0 || down>=1)return false;
    int bytes=BufferedBytes();if(bytes<0)return false;
    while(!packets.empty() && packets.front().ended.load(std::memory_order_acquire))packets.pop_front();
    XAUDIO2_VOICE_STATE state{};voice->GetState(&state);
    if(!pending.empty() && state.BuffersQueued<FAUDIO_MAX_QUEUED_BUFFERS) {
        try {packets.emplace_back();}catch(const std::bad_alloc &){return false;}
        auto &packet=packets.back();packet.frames.swap(pending);
        XAUDIO2_BUFFER buffer{};buffer.AudioBytes=packet.frames.size()*sizeof(std::int16_t);
        buffer.pAudioData=reinterpret_cast<const std::uint8_t *>(packet.frames.data());buffer.pContext=&packet;
        if(voice->SubmitSourceBuffer(&buffer,nullptr)!=S_OK){packet.frames.swap(pending);packets.pop_back();return false;}
    }
    if(!playing && static_cast<unsigned>(bytes)>=start){if(voice->Start(0,0)!=S_OK)return false;playing=true;}
    if(playing){if(!bytes)return Stop();
        if(static_cast<unsigned>(bytes)<low)jitterMode=1;
        else if(static_cast<unsigned>(bytes)>high)jitterMode=0;
        else if((jitterMode==0 && static_cast<unsigned>(bytes)<=comfortable) || (jitterMode==1 && static_cast<unsigned>(bytes)>=comfortable))jitterMode=2;
        const double changed=frequency*(jitterMode==0?1.0+up:(jitterMode==1?1.0-down:1.0));
        const float ratio=static_cast<unsigned>(changed)/static_cast<float>(frequency);
        if(voice->SetFrequencyRatio(ratio,0)!=S_OK)return false;}
    return true;
}
bool VoicePlayback::Stop(){
    if(!Owner())return false;if(voice){voice->DestroyVoice();voice=nullptr;}packets.clear();pending.clear();queuedFrames=0;playing=false;jitterMode=2;failed=false;
    return Create(); // A genuinely new stopped voice replaces its retired queue/counter.
}
}
