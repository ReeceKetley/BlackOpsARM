#pragma once
// Native typed forwarding boundary to pinned FAudio; no Windows COM ABI emulation.
#include <Windows.h>
#include <FAudio.h>
#include <FAPOBase.h>
#include <atomic>
#include <mutex>
#include <cstdint>
#include <cstdlib>

#if defined(__ANDROID__)
using WAVEFORMATEX=FAudioWaveFormatEx;
using WAVEFORMATEXTENSIBLE=FAudioWaveFormatExtensible;
using ADPCMCOEFSET=FAudioADPCMCoefSet;
#else
#include <mmreg.h>
#endif
// FAudio packs its C buffer at one byte; engine callback retirement needs an
// aligned native pointer slot, so convert metadata instead of aliasing that ABI.
struct XAUDIO2_BUFFER {
    std::uint32_t Flags{},AudioBytes{};
    const std::uint8_t *pAudioData{};
    std::uint32_t PlayBegin{},PlayLength{},LoopBegin{},LoopLength{},LoopCount{};
    void *pContext{};
};
static_assert(alignof(XAUDIO2_BUFFER)>=alignof(void *));
static_assert(offsetof(XAUDIO2_BUFFER,pAudioData)%alignof(void *)==0);
#if defined(__ANDROID__)
struct _GUID { std::uint32_t Data1;std::uint16_t Data2,Data3;std::uint8_t Data4[8]; };
#endif
static_assert(sizeof(_GUID)==16);
using XAUDIO2_BUFFER_WMA=FAudioBufferWMA;
using XAUDIO2_VOICE_STATE=FAudioVoiceState;
using XAUDIO2_VOICE_DETAILS=FAudioVoiceDetails;
using XAUDIO2_PERFORMANCE_DATA=FAudioPerformanceData;
using XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS=FAPOLockForProcessBufferParameters;
using XAPO_PROCESS_BUFFER_PARAMETERS=FAPOProcessBufferParameters;
constexpr auto XAPO_BUFFER_SILENT=FAPO_BUFFER_SILENT;
constexpr auto XAPO_BUFFER_VALID=FAPO_BUFFER_VALID;
#if !defined(WAVE_FORMAT_EXTENSIBLE)
constexpr unsigned WAVE_FORMAT_EXTENSIBLE=FAUDIO_FORMAT_EXTENSIBLE;
#endif
constexpr unsigned XAUDIO2_END_OF_STREAM=FAUDIO_END_OF_STREAM;
constexpr unsigned XAUDIO2_ANY_PROCESSOR=FAUDIO_DEFAULT_PROCESSOR;
using XAUDIO2_WINDOWS_PROCESSOR_SPECIFIER=FAudioProcessor;
constexpr auto GlobalDefaultDevice=FAudioGlobalDefaultDevice;
#define XAPO_FLAG_CHANNELS_MUST_MATCH FAPO_FLAG_CHANNELS_MUST_MATCH
#define XAPO_FLAG_FRAMERATE_MUST_MATCH FAPO_FLAG_FRAMERATE_MUST_MATCH
#define XAPO_FLAG_BITSPERSAMPLE_MUST_MATCH FAPO_FLAG_BITSPERSAMPLE_MUST_MATCH
#define XAPO_FLAG_BUFFERCOUNT_MUST_MATCH FAPO_FLAG_BUFFERCOUNT_MUST_MATCH
#define XAPO_FLAG_INPLACE_SUPPORTED FAPO_FLAG_INPLACE_SUPPORTED
#define XAPO_FLAG_INPLACE_REQUIRED FAPO_FLAG_INPLACE_REQUIRED

struct XAPO_REGISTRATION_PROPERTIES {
    GUID clsid;
    wchar_t FriendlyName[256],CopyrightInfo[256];
    unsigned MajorVersion,MinorVersion,Flags,MinInputBufferCount,MaxInputBufferCount,MinOutputBufferCount,MaxOutputBufferCount;
};
class CXAPOBase {
    struct Adapter { FAPOBase api; CXAPOBase *owner; } adapter{};
    FAPORegistrationProperties properties{};
    static CXAPOBase &Owner(void *);
    static void FAPOCALL Dispose(void *);
    static uint32_t FAPOCALL Lock(void *,uint32_t,const FAPOLockForProcessBufferParameters *,uint32_t,const FAPOLockForProcessBufferParameters *);
    static void FAPOCALL Unlock(void *);
    static void FAPOCALL ResetEffect(void *);
    static void FAPOCALL ProcessEffect(void *,uint32_t,const FAPOProcessBufferParameters *,uint32_t,FAPOProcessBufferParameters *,int32_t);
    static void FAPOCALL SetEffect(void *,const void *,uint32_t);
    static void FAPOCALL GetEffect(void *,void *,uint32_t);
    std::mutex parameterGate;
public:
    explicit CXAPOBase(const XAPO_REGISTRATION_PROPERTIES *);
    virtual ~CXAPOBase();
    int AddRef();
    int Release();
    int ReferenceCount();
    FAPO *NativeEffect() { return &adapter.api.base; }
    virtual HRESULT LockForProcess(unsigned,const XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS *,unsigned,const XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS *)=0;
    virtual void UnlockForProcess()=0;
    virtual void Reset()=0;
    virtual void Process(unsigned,const XAPO_PROCESS_BUFFER_PARAMETERS *,unsigned,XAPO_PROCESS_BUFFER_PARAMETERS *,int)=0;
    virtual void SetParameters(const void *,unsigned)=0;
    virtual void GetParameters(void *,unsigned)=0;
};
struct IXAPOParameters { virtual ~IXAPOParameters()=default; };
class IXAudio2Voice;
struct XAUDIO2_SEND_DESCRIPTOR { unsigned Flags; IXAudio2Voice *pOutputVoice; };
struct XAUDIO2_VOICE_SENDS { unsigned SendCount; XAUDIO2_SEND_DESCRIPTOR *pSends; };
struct XAUDIO2_EFFECT_DESCRIPTOR { CXAPOBase *pEffect; int InitialState; unsigned OutputChannels; };
struct XAUDIO2_EFFECT_CHAIN { unsigned EffectCount; XAUDIO2_EFFECT_DESCRIPTOR *pEffectDescriptors; };
struct XAUDIO2_DEVICE_DETAILS {
    wchar_t DeviceID[256],DisplayName[256];
    FAudioDeviceRole Role;
    WAVEFORMATEXTENSIBLE OutputFormat;
};
struct IXAudio2VoiceCallback {
    virtual ~IXAudio2VoiceCallback()=default;
    virtual void OnVoiceProcessingPassStart(UINT32)=0;
    virtual void OnVoiceProcessingPassEnd()=0;
    virtual void OnStreamEnd()=0;
    virtual void OnBufferStart(void *)=0;
    virtual void OnBufferEnd(void *)=0;
    virtual void OnLoopEnd(void *)=0;
    virtual void OnVoiceError(void *,HRESULT)=0;
};
class IXAudio2 {
    friend class IXAudio2Voice;
    FAudio *engine{};
    struct Callback { FAudioEngineCallback api{}; IXAudio2 *owner{}; } callback;
    std::atomic<std::uint32_t> failure{};
    std::atomic<unsigned> voices{};
    std::atomic<bool> mastering{};
    static void FAUDIOCALL Critical(FAudioEngineCallback *,uint32_t);
    static void FAUDIOCALL Pass(FAudioEngineCallback *);
    explicit IXAudio2(FAudio *);
public:
    static HRESULT Create(IXAudio2 **,unsigned,unsigned);
    unsigned Release();
    void StopEngine();
    HRESULT StartEngine();
    HRESULT GetDeviceCount(unsigned *);
    HRESULT GetDeviceDetails(unsigned,XAUDIO2_DEVICE_DETAILS *);
    HRESULT CreateMasteringVoice(IXAudio2Voice **,unsigned,unsigned,unsigned,unsigned,const XAUDIO2_EFFECT_CHAIN *);
    HRESULT CreateSubmixVoice(IXAudio2Voice **,unsigned,unsigned,unsigned,unsigned,const XAUDIO2_VOICE_SENDS *,const XAUDIO2_EFFECT_CHAIN *);
    HRESULT CreateSourceVoice(IXAudio2Voice **,const WAVEFORMATEX *,unsigned,float,IXAudio2VoiceCallback *,const XAUDIO2_VOICE_SENDS *,const XAUDIO2_EFFECT_CHAIN *);
    std::uint32_t Failure() const;
    void ReportError(std::uint32_t error) { failure.store(error,std::memory_order_release); }
};
class IXAudio2Voice {
    friend class IXAudio2;
    struct Callback { FAudioVoiceCallback api{}; IXAudio2Voice *owner{}; } callback;
    IXAudio2 *engine{};
    IXAudio2VoiceCallback *consumer{};
    FAudioVoice *voice{};
    bool source{};
    bool master{};
    std::uint16_t formatTag{},blockAlign{},channels{},samplesPerBlock{};
    IXAudio2Voice(IXAudio2 *,IXAudio2VoiceCallback *,bool);
    static void FAUDIOCALL BufferEnd(FAudioVoiceCallback *,void *);
    static void FAUDIOCALL BufferStart(FAudioVoiceCallback *,void *);
    static void FAUDIOCALL LoopEnd(FAudioVoiceCallback *,void *);
    static void FAUDIOCALL StreamEnd(FAudioVoiceCallback *);
    static void FAUDIOCALL VoiceError(FAudioVoiceCallback *,void *,uint32_t);
    static void FAUDIOCALL PassEnd(FAudioVoiceCallback *);
    static void FAUDIOCALL PassStart(FAudioVoiceCallback *,uint32_t);
public:
    FAudioVoice *NativeVoice() const { return voice; }
    void DestroyVoice();
    HRESULT SetOutputMatrix(IXAudio2Voice *,unsigned,unsigned,const float *,unsigned);
    HRESULT SetEffectParameters(unsigned,const void *,unsigned,unsigned);
    HRESULT SetVolume(float,unsigned);
    void GetVoiceDetails(XAUDIO2_VOICE_DETAILS *);
    HRESULT SubmitSourceBuffer(const XAUDIO2_BUFFER *,const XAUDIO2_BUFFER_WMA *);
    HRESULT Start(unsigned,unsigned);
    HRESULT Stop(unsigned,unsigned);
    void GetState(XAUDIO2_VOICE_STATE *);
    HRESULT SetFrequencyRatio(float,unsigned);
};
using IXAudio2SourceVoice=IXAudio2Voice;
using IXAudio2SubmixVoice=IXAudio2Voice;
using IXAudio2MasteringVoice=IXAudio2Voice;
HRESULT XAudio2Create(IXAudio2 **,unsigned,XAUDIO2_WINDOWS_PROCESSOR_SPECIFIER);
