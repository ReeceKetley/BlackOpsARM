#include "faudio_bridge.h"
#include "window_lifecycle.h"
#include "controller_service.h"
#include <SDL.h>
#include <FAudio_internal.h>
#include <array>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <new>

namespace {
#if defined(FAUDIO_DISABLE_DEBUGCONFIGURATION)
#error Native device observation requires the pinned FORCE_ENABLE_DEBUGCONFIGURATION build contract.
#endif
static_assert(FAUDIO_MAJOR_VERSION==25 && FAUDIO_MINOR_VERSION==2 && FAUDIO_PATCH_VERSION==0);
HRESULT Result(std::uint32_t result) { return static_cast<HRESULT>(static_cast<std::int32_t>(result)); }
constexpr HRESULT invalid=static_cast<HRESULT>(static_cast<std::int32_t>(FAUDIO_E_INVALID_CALL));
[[noreturn]] void Fatal(const char *message) { std::fprintf(stderr,"Native FAudio lifetime: %s\n",message); std::abort(); }
struct Lists {
    std::array<FAudioSendDescriptor,8> sends{};
    std::array<FAudioEffectDescriptor,8> effects{};
    FAudioVoiceSends sendList{};
    FAudioEffectChain effectList{};
    bool Convert(const XAUDIO2_VOICE_SENDS *send,const XAUDIO2_EFFECT_CHAIN *effect) {
        if(send) {
            if(send->SendCount>sends.size() || (!send->pSends && send->SendCount)) return false;
            sendList.SendCount=send->SendCount; sendList.pSends=sends.data();
            for(unsigned i=0;i<send->SendCount;++i) {
                if(!send->pSends[i].pOutputVoice) return false;
                sends[i]={send->pSends[i].Flags,send->pSends[i].pOutputVoice->NativeVoice()};
            }
        }
        if(effect) {
            if(effect->EffectCount>effects.size() || (!effect->pEffectDescriptors && effect->EffectCount)) return false;
            effectList.EffectCount=effect->EffectCount; effectList.pEffectDescriptors=effects.data();
            for(unsigned i=0;i<effect->EffectCount;++i) {
                const auto &source=effect->pEffectDescriptors[i];
                if(!source.pEffect) return false;
                effects[i]={source.pEffect->NativeEffect(),source.InitialState,source.OutputChannels};
            }
        }
        return true;
    }
};
}
CXAPOBase &CXAPOBase::Owner(void *opaque) { return *static_cast<Adapter *>(opaque)->owner; }
void FAPOCALL CXAPOBase::Dispose(void *opaque) {
    auto &base=static_cast<Adapter *>(opaque)->api;
    if(base.m_pfl32MatrixCoefficients) base.pFree(base.m_pfl32MatrixCoefficients);
    base.m_pfl32MatrixCoefficients=nullptr;
    // The engine owns fixed DSP pool storage; the C API does not delete its C++ owner.
}
uint32_t FAPOCALL CXAPOBase::Lock(void *self,uint32_t a,const FAPOLockForProcessBufferParameters *b,uint32_t c,const FAPOLockForProcessBufferParameters *d) {
    return static_cast<std::uint32_t>(Owner(self).LockForProcess(a,b,c,d));
}
void FAPOCALL CXAPOBase::Unlock(void *self) { Owner(self).UnlockForProcess(); }
void FAPOCALL CXAPOBase::ResetEffect(void *self) { Owner(self).Reset(); }
void FAPOCALL CXAPOBase::ProcessEffect(void *self,uint32_t a,const FAPOProcessBufferParameters *b,uint32_t c,FAPOProcessBufferParameters *d,int32_t enabled) {
    auto &owner=Owner(self); std::lock_guard guard(owner.parameterGate);
    owner.Process(a,b,c,d,enabled);
}
void FAPOCALL CXAPOBase::SetEffect(void *self,const void *data,uint32_t size) {
    auto &owner=Owner(self); std::lock_guard guard(owner.parameterGate); owner.SetParameters(data,size);
}
void FAPOCALL CXAPOBase::GetEffect(void *self,void *data,uint32_t size) {
    auto &owner=Owner(self); std::lock_guard guard(owner.parameterGate); owner.GetParameters(data,size);
}
CXAPOBase::CXAPOBase(const XAPO_REGISTRATION_PROPERTIES *props) {
    static_assert(offsetof(Adapter,api)==0);
    if(!props) Fatal("null effect registration");
    static_assert(sizeof(properties.clsid)==sizeof(props->clsid));
    std::memcpy(&properties.clsid,&props->clsid,sizeof(properties.clsid));
    for(unsigned i=0;i<256;++i) {
        if(static_cast<unsigned>(props->FriendlyName[i])>0xFFFF || static_cast<unsigned>(props->CopyrightInfo[i])>0xFFFF) Fatal("invalid effect registration UTF16");
        properties.FriendlyName[i]=static_cast<std::int16_t>(props->FriendlyName[i]);
        properties.CopyrightInfo[i]=static_cast<std::int16_t>(props->CopyrightInfo[i]);
    }
    properties.MajorVersion=props->MajorVersion;properties.MinorVersion=props->MinorVersion;
    properties.Flags=props->Flags;properties.MinInputBufferCount=props->MinInputBufferCount;
    properties.MaxInputBufferCount=props->MaxInputBufferCount;properties.MinOutputBufferCount=props->MinOutputBufferCount;
    properties.MaxOutputBufferCount=props->MaxOutputBufferCount;
    adapter.owner=this;
    CreateFAPOBase(&adapter.api,&properties,nullptr,0,0);
    adapter.api.Destructor=Dispose;
    adapter.api.base.LockForProcess=Lock;adapter.api.base.UnlockForProcess=Unlock;
    adapter.api.base.Reset=ResetEffect;adapter.api.base.Process=ProcessEffect;
    adapter.api.base.SetParameters=SetEffect;adapter.api.base.GetParameters=GetEffect;
}
CXAPOBase::~CXAPOBase() {
    if(ReferenceCount()!=1) Fatal("effect owner destroyed while retained by graph");
    FAPOBase_Release(&adapter.api);
}
int CXAPOBase::AddRef() { return FAPOBase_AddRef(&adapter.api); }
int CXAPOBase::Release() { return FAPOBase_Release(&adapter.api); }
int CXAPOBase::ReferenceCount() { const int refs=AddRef(); Release(); return refs-1; }
void FAUDIOCALL IXAudio2::Critical(FAudioEngineCallback *cb,uint32_t error) { reinterpret_cast<Callback *>(cb)->owner->ReportError(error); }
void FAUDIOCALL IXAudio2::Pass(FAudioEngineCallback *) {} // No engine processing-pass consumer is registered.
IXAudio2::IXAudio2(FAudio *value):engine(value) {
    callback.owner=this;callback.api={Critical,Pass,Pass};
    const auto result=FAudio_RegisterForCallbacks(engine,&callback.api);
    if(result) ReportError(result);
    FAudio_StopEngine(engine); // Graph construction/initial parameter publication precede mixer activation.
}
HRESULT IXAudio2::Create(IXAudio2 **out,unsigned flags,unsigned processor) {
    if(!out) return invalid;
    *out=nullptr;
    if(!bo1::android::IsSDLEventOwner()) { SDL_SetError("audio construction requires actual SDL event owner"); return invalid; }
#if defined(__ANDROID__)
    if(!bo1::android::SDLJavaSetupComplete()) { SDL_SetError("audio requires completed actual SDL Java setup"); return invalid; }
#endif
    FAudio *value=nullptr;
    const auto result=FAudioCreate(&value,flags,processor);
    if(result || !value) return result?Result(result):invalid;
    auto *owner=new(std::nothrow) IXAudio2(value);
    if(!owner) { FAudio_Release(value);return Result(FAUDIO_E_OUT_OF_MEMORY); }
    if(owner->Failure()) { const auto error=owner->Failure();owner->Release();return Result(error); }
    *out=owner;return S_OK;
}
unsigned IXAudio2::Release() {
    if(voices.load(std::memory_order_acquire)) Fatal("engine owner released before all voices retire");
    FAudio_UnregisterForCallbacks(engine,&callback.api);
    const auto result=FAudio_Release(engine);engine=nullptr;delete this;return result;
}
void IXAudio2::StopEngine() { FAudio_StopEngine(engine); }
HRESULT IXAudio2::StartEngine() { return Failure()?Result(Failure()):Result(FAudio_StartEngine(engine)); }
std::uint32_t IXAudio2::Failure() const {
    const auto error=failure.load(std::memory_order_acquire);
    if(error) return error;
    // The pinned SDL2 backend owns this actual SDL device ID. Observe its live
    // status rather than manufacturing readiness from a retained engine pointer.
    if(mastering.load(std::memory_order_acquire) &&
        (!engine->platform || SDL_GetAudioDeviceStatus(static_cast<SDL_AudioDeviceID>(reinterpret_cast<std::uintptr_t>(engine->platform)))==SDL_AUDIO_STOPPED))
        return FAUDIO_E_DEVICE_INVALIDATED;
    return 0;
}
HRESULT IXAudio2::GetDeviceCount(unsigned *count) { return Result(FAudio_GetDeviceCount(engine,count)); }
HRESULT IXAudio2::GetDeviceDetails(unsigned index,XAUDIO2_DEVICE_DETAILS *out) {
    if(!out) return invalid;
    FAudioDeviceDetails native{};const auto result=FAudio_GetDeviceDetails(engine,index,&native);
    if(result) return Result(result);
    XAUDIO2_DEVICE_DETAILS converted{};
    for(unsigned i=0;i<256;++i) { converted.DeviceID[i]=static_cast<std::uint16_t>(native.DeviceID[i]);converted.DisplayName[i]=static_cast<std::uint16_t>(native.DisplayName[i]); }
    static_assert(sizeof(converted.OutputFormat)==sizeof(native.OutputFormat));
    converted.Role=native.Role;std::memcpy(&converted.OutputFormat,&native.OutputFormat,sizeof(native.OutputFormat));*out=converted;return S_OK;
}
HRESULT IXAudio2::CreateMasteringVoice(IXAudio2Voice **out,unsigned channels,unsigned rate,unsigned flags,unsigned device,const XAUDIO2_EFFECT_CHAIN *effects) {
    if(!out) return invalid;*out=nullptr;Lists lists;if(!lists.Convert(nullptr,effects)) return invalid;
    auto *owner=new(std::nothrow) IXAudio2Voice(this,nullptr,false);if(!owner)return Result(FAUDIO_E_OUT_OF_MEMORY);
    FAudioMasteringVoice *native=nullptr;
    const auto result=FAudio_CreateMasteringVoice(engine,&native,channels,rate,flags,device,effects?&lists.effectList:nullptr);
    if(result || !native) { delete owner;return result?Result(result):invalid; }
    owner->voice=native;owner->master=true;mastering.store(true,std::memory_order_release);
    voices.fetch_add(1,std::memory_order_relaxed);*out=owner;return S_OK;
}
HRESULT IXAudio2::CreateSubmixVoice(IXAudio2Voice **out,unsigned channels,unsigned rate,unsigned flags,unsigned stage,const XAUDIO2_VOICE_SENDS *sends,const XAUDIO2_EFFECT_CHAIN *effects) {
    if(!out)return invalid;*out=nullptr;Lists lists;if(!lists.Convert(sends,effects))return invalid;
    auto *owner=new(std::nothrow) IXAudio2Voice(this,nullptr,false);if(!owner)return Result(FAUDIO_E_OUT_OF_MEMORY);
    FAudioSubmixVoice *native=nullptr;
    const auto result=FAudio_CreateSubmixVoice(engine,&native,channels,rate,flags,stage,sends?&lists.sendList:nullptr,effects?&lists.effectList:nullptr);
    if(result || !native) { delete owner;return result?Result(result):invalid; }
    owner->voice=native;voices.fetch_add(1,std::memory_order_relaxed);*out=owner;return S_OK;
}
HRESULT IXAudio2::CreateSourceVoice(IXAudio2Voice **out,const WAVEFORMATEX *format,unsigned flags,float ratio,IXAudio2VoiceCallback *callback,const XAUDIO2_VOICE_SENDS *sends,const XAUDIO2_EFFECT_CHAIN *effects) {
    if(!out)return invalid;*out=nullptr;
    // SDL FAudio25.02 has no actual WMA decoder: never enter its error-to-silence fallback.
    if(!format || !format->nChannels || format->nChannels>8 || !format->nSamplesPerSec || !format->nBlockAlign) return invalid;
    if(format->wFormatTag!=FAUDIO_FORMAT_PCM && format->wFormatTag!=FAUDIO_FORMAT_IEEE_FLOAT &&
        format->wFormatTag!=FAUDIO_FORMAT_MSADPCM && format->wFormatTag!=FAUDIO_FORMAT_EXTENSIBLE) return invalid;
    if(format->wFormatTag==FAUDIO_FORMAT_EXTENSIBLE) {
        if(format->cbSize<22) return invalid;
        const auto *extended=reinterpret_cast<const FAudioWaveFormatExtensible *>(format);
        if(extended->SubFormat.Data1!=FAUDIO_FORMAT_PCM && extended->SubFormat.Data1!=FAUDIO_FORMAT_IEEE_FLOAT) return invalid;
        static constexpr std::uint8_t tail[]={0x80,0,0,0xAA,0,0x38,0x9B,0x71};
        if(extended->SubFormat.Data2 || extended->SubFormat.Data3!=0x10 || std::memcmp(extended->SubFormat.Data4,tail,8)) return invalid;
        if(extended->SubFormat.Data1==FAUDIO_FORMAT_IEEE_FLOAT && format->wBitsPerSample!=32) return invalid;
    }
    if(format->wFormatTag==FAUDIO_FORMAT_IEEE_FLOAT && format->wBitsPerSample!=32) return invalid;
    if((format->wFormatTag==FAUDIO_FORMAT_PCM || format->wFormatTag==FAUDIO_FORMAT_EXTENSIBLE) &&
        format->wBitsPerSample!=8 && format->wBitsPerSample!=16 && format->wBitsPerSample!=24 && format->wBitsPerSample!=32) return invalid;
    std::uint16_t samplesPerBlock=0;
    if(format->wFormatTag==FAUDIO_FORMAT_MSADPCM) {
        if(format->nChannels>2 || format->wBitsPerSample!=4 || format->cbSize<32 || format->nBlockAlign<7*format->nChannels) return invalid;
        const auto *adpcm=reinterpret_cast<const FAudioADPCMWaveFormat *>(format);
        const auto expected=static_cast<unsigned>(2*(format->nBlockAlign/format->nChannels-7)+2);
        if(adpcm->wNumCoef!=7 || adpcm->wSamplesPerBlock!=expected) return invalid;
        samplesPerBlock=adpcm->wSamplesPerBlock;
    } else if(format->nBlockAlign!=format->nChannels*(format->wBitsPerSample/8) || !format->wBitsPerSample) return invalid;
    Lists lists;if(!lists.Convert(sends,effects))return invalid;
    auto *owner=new(std::nothrow) IXAudio2Voice(this,callback,true);if(!owner)return Result(FAUDIO_E_OUT_OF_MEMORY);
    FAudioSourceVoice *native=nullptr;
    static_assert(sizeof(WAVEFORMATEX)==sizeof(FAudioWaveFormatEx));
    if(format->cbSize>46) { delete owner;return invalid; }
    alignas(8) std::array<std::uint8_t,64> nativeFormat{};
    std::memcpy(nativeFormat.data(),format,sizeof(WAVEFORMATEX)+format->cbSize);
    const auto result=FAudio_CreateSourceVoice(engine,&native,reinterpret_cast<const FAudioWaveFormatEx *>(nativeFormat.data()),flags,ratio,callback?&owner->callback.api:nullptr,sends?&lists.sendList:nullptr,effects?&lists.effectList:nullptr);
    if(result || !native) { delete owner;return result?Result(result):invalid; }
    owner->voice=native;owner->formatTag=format->wFormatTag;owner->blockAlign=format->nBlockAlign;
    owner->channels=format->nChannels;owner->samplesPerBlock=samplesPerBlock;
    voices.fetch_add(1,std::memory_order_relaxed);*out=owner;return S_OK;
}
HRESULT XAudio2Create(IXAudio2 **out,unsigned flags,XAUDIO2_WINDOWS_PROCESSOR_SPECIFIER processor) { return IXAudio2::Create(out,flags,processor); }
IXAudio2Voice::IXAudio2Voice(IXAudio2 *audio,IXAudio2VoiceCallback *input,bool isSource):engine(audio),consumer(input),source(isSource) {
    callback.owner=this;callback.api={BufferEnd,BufferStart,LoopEnd,StreamEnd,VoiceError,PassEnd,PassStart};
}
void FAUDIOCALL IXAudio2Voice::BufferEnd(FAudioVoiceCallback *cb,void *context) { auto *self=reinterpret_cast<Callback *>(cb)->owner;self->consumer->OnBufferEnd(context); }
void FAUDIOCALL IXAudio2Voice::BufferStart(FAudioVoiceCallback *cb,void *context) { auto *self=reinterpret_cast<Callback *>(cb)->owner;self->consumer->OnBufferStart(context); }
void FAUDIOCALL IXAudio2Voice::LoopEnd(FAudioVoiceCallback *cb,void *context) { auto *self=reinterpret_cast<Callback *>(cb)->owner;self->consumer->OnLoopEnd(context); }
void FAUDIOCALL IXAudio2Voice::StreamEnd(FAudioVoiceCallback *cb) { reinterpret_cast<Callback *>(cb)->owner->consumer->OnStreamEnd(); }
void FAUDIOCALL IXAudio2Voice::VoiceError(FAudioVoiceCallback *cb,void *context,uint32_t error) { auto *self=reinterpret_cast<Callback *>(cb)->owner;self->engine->ReportError(error);self->consumer->OnVoiceError(context,Result(error)); }
void FAUDIOCALL IXAudio2Voice::PassEnd(FAudioVoiceCallback *cb) { reinterpret_cast<Callback *>(cb)->owner->consumer->OnVoiceProcessingPassEnd(); }
void FAUDIOCALL IXAudio2Voice::PassStart(FAudioVoiceCallback *cb,uint32_t bytes) { reinterpret_cast<Callback *>(cb)->owner->consumer->OnVoiceProcessingPassStart(bytes); }
void IXAudio2Voice::DestroyVoice() {
    if(master) engine->mastering.store(false,std::memory_order_release);
    const auto result=FAudioVoice_DestroyVoiceSafeEXT(voice);
    if(result) Fatal("voice destruction did not quiesce");
    voice=nullptr;engine->voices.fetch_sub(1,std::memory_order_release);delete this;
}
HRESULT IXAudio2Voice::SetOutputMatrix(IXAudio2Voice *out,unsigned inChannels,unsigned outChannels,const float *matrix,unsigned operation) { return !out?invalid:Result(FAudioVoice_SetOutputMatrix(voice,out->voice,inChannels,outChannels,matrix,operation)); }
HRESULT IXAudio2Voice::SetEffectParameters(unsigned effect,const void *data,unsigned size,unsigned operation) { return Result(FAudioVoice_SetEffectParameters(voice,effect,data,size,operation)); }
HRESULT IXAudio2Voice::SetVolume(float gain,unsigned operation) { return Result(FAudioVoice_SetVolume(voice,gain,operation)); }
void IXAudio2Voice::GetVoiceDetails(XAUDIO2_VOICE_DETAILS *details) { FAudioVoice_GetVoiceDetails(voice,details); }
HRESULT IXAudio2Voice::SubmitSourceBuffer(const XAUDIO2_BUFFER *buffer,const XAUDIO2_BUFFER_WMA *wma) {
    if(!source || wma || !buffer) return invalid;
    if(!buffer->pAudioData || !buffer->AudioBytes) return invalid;
    std::uint64_t frames=0;
    if(formatTag==FAUDIO_FORMAT_MSADPCM) {
        const unsigned full=buffer->AudioBytes/blockAlign,tail=buffer->AudioBytes%blockAlign;
        if(tail && tail<7*channels) return invalid;
        for(unsigned block=0;block<full+(tail?1:0);++block)
            for(unsigned channel=0;channel<channels;++channel)
                if(buffer->pAudioData[static_cast<std::size_t>(block)*blockAlign+channel]>=7) return invalid;
        frames=static_cast<std::uint64_t>(full)*samplesPerBlock+(tail?2+(tail-7*channels)*2/channels:0);
    } else {
        if(buffer->AudioBytes%blockAlign) return invalid;
        frames=buffer->AudioBytes/blockAlign;
    }
    if(buffer->PlayBegin>=frames || (buffer->PlayLength && static_cast<std::uint64_t>(buffer->PlayBegin)+buffer->PlayLength>frames) ||
        buffer->LoopBegin>=frames || (buffer->LoopLength && static_cast<std::uint64_t>(buffer->LoopBegin)+buffer->LoopLength>frames)) return invalid;
    FAudioBuffer native{buffer->Flags,buffer->AudioBytes,buffer->pAudioData,
        buffer->PlayBegin,buffer->PlayLength,buffer->LoopBegin,buffer->LoopLength,buffer->LoopCount,buffer->pContext};
    return Result(FAudioSourceVoice_SubmitSourceBuffer(voice,&native,nullptr));
}
HRESULT IXAudio2Voice::Start(unsigned flags,unsigned operation) { return !source?invalid:Result(FAudioSourceVoice_Start(voice,flags,operation)); }
HRESULT IXAudio2Voice::Stop(unsigned flags,unsigned operation) { return !source?invalid:Result(FAudioSourceVoice_Stop(voice,flags,operation)); }
void IXAudio2Voice::GetState(XAUDIO2_VOICE_STATE *state) { if(!source)Fatal("state query requires source voice");FAudioSourceVoice_GetState(voice,state,0); }
HRESULT IXAudio2Voice::SetFrequencyRatio(float ratio,unsigned operation) { return !source?invalid:Result(FAudioSourceVoice_SetFrequencyRatio(voice,ratio,operation)); }
