#define SDL_MAIN_HANDLED
#include <sound/snd_driver_xaudio2_dsp.h>
#include <sound/snd.h>
#include <universal/profile.h>
#include <platform/android/window_lifecycle.h>
#include <SDL.h>
#include <platform/android/voice_audio.h>
#include <memory>
#include <atomic>
#include <array>
#include <vector>
#include <thread>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <cmath>
#if defined(_WIN32)
#include <crtdbg.h>
#endif

// Isolated engine meter/profiling/encoding consumers, no substituted audio producer.
snd_local_t g_snd;
bool g_bo1ProfOn=false;
unsigned long g_bo1ProfMainThread;
BO1ProfZone *volatile g_bo1ProfZones;
void BO1_ProfRegister(BO1ProfZone *) { std::abort(); }
bool mjpeg_is_encoding() { return false; } // Fixture does not run a recording session.
void mjpeg_add_samples(short *,int) { std::abort(); }
static std::atomic<bool> quitRequested{};
void Sys_SetQuitRequested() { quitRequested=true; }
bool Assert_MyHandler(const char *file,int line,int,const char *format,...) {
    fprintf(stderr,"engine assertion %s:%d ",file,line);va_list args;va_start(args,format);vfprintf(stderr,format,args);va_end(args);std::abort();
}
char *va(const char *format,...) {
    static thread_local char output[4096];va_list args;va_start(args,format);vsnprintf(output,sizeof(output),format,args);va_end(args);return output;
}
// Complete original math TU supplies real normalization; unrelated unused consumers fail closed.
double vectosignedpitch(const float *) { std::abort(); }
bool Com_IsSPLevel() { std::abort(); }
void AngleVectors(const float *,float *,float *,float *) { std::abort(); }
static void Check(bool ok,int line) { if(!ok) { fprintf(stderr,"FAIL %d (%s)\n",line,SDL_GetError());std::abort(); } }
#define CHECK(x) Check((x),__LINE__)
static void Input(const SDL_Event &,void *) {}
static void SoftwareDSP() {
    SDXA2SourceEffect source;
    FAudioWaveFormatExtensible format{};
    format.Format={FAUDIO_FORMAT_EXTENSIBLE,1,48000,192000,4,32,22};
    format.SubFormat={3,0,0x10,{0x80,0,0,0xAA,0,0x38,0x9B,0x71}};
    XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS lock{&format.Format,2048};
    auto *effect=source.NativeEffect();
    CHECK(effect->LockForProcess(effect,1,&lock,1,&lock)!=0);
    lock.MaxFrameCount=512;CHECK(!effect->LockForProcess(effect,1,&lock,1,&lock));
    snd_dsp_source_params params{};params.frameRate=48000;params.lpfAttenuation=-6;params.lpfRatio=.25f;
    effect->SetParameters(effect,&params,sizeof(params));
    alignas(128) std::array<float,528> samples{};
    for(unsigned i=0;i<samples.size();++i)samples[i]=1234;
    for(unsigned i=0;i<512;++i)samples[i+8]=(i&1)?.25f:-.25f;
    XAPO_PROCESS_BUFFER_PARAMETERS data{samples.data()+8,XAPO_BUFFER_VALID,512};
    effect->Process(effect,1,&data,1,&data,1);
    CHECK(data.ValidFrameCount==512);
    for(unsigned i=0;i<8;++i)CHECK(samples[i]==1234 && samples[520+i]==1234);
    for(unsigned i=8;i<520;++i)CHECK(std::isfinite(samples[i]));
    effect->UnlockForProcess(effect);
    CHECK(source.ReferenceCount()==1);
    // Both pointer slots used by canonical stream retirement must be naturally aligned.
    XAUDIO2_BUFFER buffers[2]{};
    for(auto &buffer:buffers) {
        CHECK(reinterpret_cast<std::uintptr_t>(&buffer.pAudioData)%alignof(void *)==0);
        std::atomic_ref(buffer.pAudioData).store(reinterpret_cast<const std::uint8_t *>(samples.data()));
        CHECK(std::atomic_ref(buffer.pAudioData).exchange(nullptr)!=nullptr);
        CHECK(std::atomic_ref(buffer.pAudioData).exchange(nullptr)==nullptr);
    }
}
#if !defined(__ANDROID__)
static XAPO_REGISTRATION_PROPERTIES captureProps={{9,1,2,{3,4,5,6,7,8,9,10}},L"Explicit null-sink fixture capture",L"BlackOpsARM",1,0,
    XAPO_FLAG_CHANNELS_MUST_MATCH|XAPO_FLAG_FRAMERATE_MUST_MATCH|XAPO_FLAG_BITSPERSAMPLE_MUST_MATCH|XAPO_FLAG_BUFFERCOUNT_MUST_MATCH|XAPO_FLAG_INPLACE_SUPPORTED|XAPO_FLAG_INPLACE_REQUIRED,1,1,1,1};
struct Capture:CXAPOBase {
    std::atomic<unsigned> passes{},nonzero{},frames{},decodedValues{};
    unsigned channels;
    bool inspectDecoded;
    explicit Capture(unsigned count=2,bool decoded=false):CXAPOBase(&captureProps),channels(count),inspectDecoded(decoded) {}
    HRESULT LockForProcess(unsigned,const XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS *input,unsigned,const XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS *) override {
        return input->pFormat->nChannels==channels && input->pFormat->nSamplesPerSec==48000 ? S_OK:E_FAIL;
    }
    void UnlockForProcess() override {}
    void Reset() override {}
    void SetParameters(const void *,unsigned) override { std::abort(); }
    void GetParameters(void *,unsigned) override { std::abort(); }
    void Process(unsigned,const XAPO_PROCESS_BUFFER_PARAMETERS *input,unsigned,XAPO_PROCESS_BUFFER_PARAMETERS *output,int) override {
        const auto count=input->ValidFrameCount;
        const auto *values=static_cast<const float *>(input->pBuffer);
        for(unsigned i=0;i<count*channels;++i) { CHECK(std::isfinite(values[i]));if(std::fabs(values[i])>1e-5f)nonzero.fetch_add(1);if(inspectDecoded && std::fabs(values[i]-1000.0f/32768)<1e-6f)decodedValues.fetch_add(1); }
        if(output->pBuffer!=input->pBuffer)memcpy(output->pBuffer,input->pBuffer,count*channels*sizeof(float));
        output->ValidFrameCount=count;output->BufferFlags=input->BufferFlags;
        frames.fetch_add(count);passes.fetch_add(1,std::memory_order_release);
    }
};
struct Completion:IXAudio2VoiceCallback {
    std::atomic<unsigned> ends{},streams{},passes{},loops{};
    void OnVoiceProcessingPassStart(UINT32) override {}
    void OnVoiceProcessingPassEnd() override { passes.fetch_add(1,std::memory_order_release); }
    void OnStreamEnd() override { streams.fetch_add(1); }
    void OnBufferStart(void *) override {}
    void OnBufferEnd(void *) override { ends.fetch_add(1,std::memory_order_release); }
    void OnLoopEnd(void *) override { loops.fetch_add(1,std::memory_order_release); }
    void OnVoiceError(void *,HRESULT) override { std::abort(); }
};
template<class Predicate> void Await(Predicate predicate) {
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    while(!predicate()) { CHECK(std::chrono::steady_clock::now()<deadline);std::this_thread::sleep_for(std::chrono::milliseconds(2)); }
}
static void Graph() {
    IXAudio2 *engine=nullptr;CHECK(XAudio2Create(&engine,0,XAUDIO2_ANY_PROCESSOR)==S_OK);
    Capture capture;SDXA2MasterBusEffect masterFx;SDXA2MasterNoVoiceBusEffect worldFx;SDXA2RadverbEffect reverbFx;SDXA2SourceEffect sourceFx;
    memset(masterFx.state,0,sizeof(masterFx.state));memset(worldFx.state,0,sizeof(worldFx.state));memset(&reverbFx.state,0,sizeof(reverbFx.state));
    IXAudio2MasteringVoice *master=nullptr;XAUDIO2_EFFECT_DESCRIPTOR tap{&capture,1,2};XAUDIO2_EFFECT_CHAIN tapChain{1,&tap};
    CHECK(engine->CreateMasteringVoice(&master,2,48000,0,0,&tapChain)==S_OK);
    auto Bus=[&](unsigned channels,unsigned stage,SDXA2Effect *effect,IXAudio2Voice *output) {
        XAUDIO2_SEND_DESCRIPTOR send{0,output};XAUDIO2_VOICE_SENDS sends{1,&send};
        XAUDIO2_EFFECT_DESCRIPTOR descriptor{effect,1,channels};XAUDIO2_EFFECT_CHAIN chain{1,&descriptor};IXAudio2SubmixVoice *result=nullptr;
        CHECK(engine->CreateSubmixVoice(&result,channels,48000,0,stage,&sends,&chain)==S_OK);return result;
    };
    auto *masterBus=Bus(2,3,&masterFx,master),*worldBus=Bus(2,2,&worldFx,masterBus),*reverbBus=Bus(4,1,&reverbFx,worldBus);
    const float reverbMatrix[]={0,1,0,1,1,0,1,0};CHECK(reverbBus->SetOutputMatrix(worldBus,4,2,reverbMatrix,0)==S_OK);
    snd_dsp_master_params masterParams{};masterParams.eqG=1;
    CHECK(masterBus->SetEffectParameters(0,&masterParams,sizeof(masterParams),0)==S_OK);
    snd_dsp_source_params sourceParams{};sourceParams.frameRate=48000;sourceParams.lpfAttenuation=-6;sourceParams.lpfRatio=.25f;
    WAVEFORMATEX format{};format.wFormatTag=FAUDIO_FORMAT_PCM;format.nChannels=1;format.nSamplesPerSec=48000;format.wBitsPerSample=16;format.nBlockAlign=2;format.nAvgBytesPerSec=96000;
    XAUDIO2_SEND_DESCRIPTOR sourceSends[]={{0,worldBus},{0,reverbBus}};XAUDIO2_VOICE_SENDS sends{2,sourceSends};
    XAUDIO2_EFFECT_DESCRIPTOR sourceEffect{&sourceFx,1,1};XAUDIO2_EFFECT_CHAIN sourceChain{1,&sourceEffect};
    Completion completion;IXAudio2SourceVoice *voice=nullptr;
    CHECK(engine->CreateSourceVoice(&voice,&format,0,2,&completion,&sends,&sourceChain)==S_OK);
    CHECK(voice->SetEffectParameters(0,&sourceParams,sizeof(sourceParams),0)==S_OK);
    const float dry[]={.5f,.5f},wet[]={.1f,.1f,.1f,.1f};
    CHECK(voice->SetOutputMatrix(worldBus,1,2,dry,0)==S_OK);CHECK(voice->SetOutputMatrix(reverbBus,1,4,wet,0)==S_OK);
    std::vector<std::int16_t> pcm(4800);for(unsigned i=0;i<pcm.size();++i)pcm[i]=(i%32<16)?8000:-8000;
    XAUDIO2_BUFFER buffers[2]{};
    for(unsigned i=0;i<2;++i) { buffers[i].AudioBytes=pcm.size()*sizeof(std::int16_t);buffers[i].pAudioData=reinterpret_cast<const std::uint8_t *>(pcm.data());buffers[i].pContext=reinterpret_cast<void *>(static_cast<std::uintptr_t>(i+1)); }
    buffers[1].Flags=XAUDIO2_END_OF_STREAM;
    CHECK(voice->SubmitSourceBuffer(&buffers[0],nullptr)==S_OK && voice->SubmitSourceBuffer(&buffers[1],nullptr)==S_OK);
    CHECK(voice->Start(0,0)==S_OK && engine->StartEngine()==S_OK);
    Await([&]{XAUDIO2_VOICE_STATE state{};voice->GetState(&state);return state.SamplesPlayed>=1024;});
    CHECK(voice->Stop(0,0)==S_OK);XAUDIO2_VOICE_STATE paused{};voice->GetState(&paused);
    std::this_thread::sleep_for(std::chrono::milliseconds(25));XAUDIO2_VOICE_STATE again{};voice->GetState(&again);CHECK(paused.SamplesPlayed==again.SamplesPlayed);
    CHECK(voice->SetFrequencyRatio(1.25f,0)==S_OK && voice->Start(0,0)==S_OK);
    Await([&]{return completion.ends.load(std::memory_order_acquire)==2;});
    XAUDIO2_VOICE_STATE ended{};voice->GetState(&ended);
    CHECK(!ended.BuffersQueued && ended.SamplesPlayed==0 && completion.streams.load()==1); // Canonical EOS resets the sample counter.
    CHECK(capture.passes.load()>0 && capture.nonzero.load()>0 && !engine->Failure());
    voice->DestroyVoice();
    // Real upstream MSADPCM decoder, exact512-frame mono block and predictor bounds.
    struct ADPCM { WAVEFORMATEX base;unsigned short samples,coefficients;ADPCMCOEFSET coeff[7]; } adpcm{};
    adpcm.base.wFormatTag=FAUDIO_FORMAT_MSADPCM;adpcm.base.nChannels=1;adpcm.base.nSamplesPerSec=48000;adpcm.base.wBitsPerSample=4;adpcm.base.nBlockAlign=262;adpcm.base.cbSize=32;adpcm.samples=512;adpcm.coefficients=7;
    const ADPCMCOEFSET coeff[]={{256,0},{512,-256},{0,0},{192,64},{240,0},{460,-208},{392,-232}};memcpy(adpcm.coeff,coeff,sizeof(coeff));
    Capture decodeProbe(1,true);XAUDIO2_EFFECT_DESCRIPTOR decodedEffect{&decodeProbe,1,1};XAUDIO2_EFFECT_CHAIN decodedChain{1,&decodedEffect};
    Completion decoded;CHECK(engine->CreateSourceVoice(&voice,&adpcm.base,0,2,&decoded,&sends,&decodedChain)==S_OK);
    std::array<std::uint8_t,262> encoded{};encoded[1]=16;encoded[3]=encoded[5]=0xE8;encoded[4]=encoded[6]=3;
    XAUDIO2_BUFFER compressed{};compressed.AudioBytes=encoded.size();compressed.pAudioData=encoded.data();
    encoded[0]=7;CHECK(voice->SubmitSourceBuffer(&compressed,nullptr)!=S_OK);encoded[0]=0;
    CHECK(voice->SubmitSourceBuffer(&compressed,nullptr)==S_OK && voice->Start(0,0)==S_OK);
    Await([&]{return decoded.ends.load(std::memory_order_acquire)==1;});voice->GetState(&ended);CHECK(ended.SamplesPlayed==512 && decodeProbe.decodedValues.load()>=512);voice->DestroyVoice();
    CHECK(decodeProbe.ReferenceCount()==1);
    // Borrowed storage and callbacks must be quiescent when an active looping voice retires.
    format.wFormatTag=FAUDIO_FORMAT_IEEE_FLOAT;format.wBitsPerSample=32;format.nBlockAlign=4;format.nAvgBytesPerSec=192000;
    std::vector<float> ownedDecoded(pcm.size());for(unsigned i=0;i<pcm.size();++i)ownedDecoded[i]=pcm[i]/32768.0f;
    Completion active;CHECK(engine->CreateSourceVoice(&voice,&format,0,2,&active,&sends,nullptr)==S_OK);
    auto looping=buffers[0];looping.pAudioData=reinterpret_cast<const std::uint8_t *>(ownedDecoded.data());looping.AudioBytes=ownedDecoded.size()*sizeof(float);looping.LoopCount=255;
    CHECK(voice->SubmitSourceBuffer(&looping,nullptr)==S_OK && voice->Start(0,0)==S_OK);
    Await([&]{XAUDIO2_VOICE_STATE playing{};voice->GetState(&playing);return playing.SamplesPlayed>=512;});
    voice->DestroyVoice();ownedDecoded.clear();ownedDecoded.shrink_to_fit();const auto passes=active.passes.load(),ends=active.ends.load(),loops=active.loops.load();
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
    CHECK(active.passes.load()==passes && active.ends.load()==ends && active.loops.load()==loops);
    format.wFormatTag=FAUDIO_FORMAT_WMAUDIO2;voice=reinterpret_cast<IXAudio2SourceVoice *>(1);
    CHECK(engine->CreateSourceVoice(&voice,&format,0,2,nullptr,nullptr,nullptr)!=S_OK && !voice);
    {
        std::array<std::unique_ptr<bo1::android::VoicePlayback>,32> talkers;
        std::vector<short> speech(30000);for(unsigned i=0;i<speech.size();++i)speech[i]=static_cast<short>(1000*std::sin(i*.15));
        for(auto &talker:talkers) {
            talker=std::make_unique<bo1::android::VoicePlayback>(engine,masterBus,8192,1);CHECK(talker->Live());
            CHECK(talker->Queue(speech.data(),60000,51200)==51200);CHECK(talker->BufferedBytes()==51200);
            CHECK(talker->Queue(speech.data(),320,51200)==0);CHECK(talker->Queue(speech.data(),3,51200)==-1);
            CHECK(talker->Frame(6000,320,10000,6000,.01f,.01f) && talker->Playing());
        }
        Await([&]{return talkers.front()->BufferedBytes()<50000;});
        for(auto &talker:talkers){CHECK(talker->Stop());CHECK(talker->BufferedBytes()==0);talker.reset();}
        speech.clear();speech.shrink_to_fit();
        std::this_thread::sleep_for(std::chrono::milliseconds(25));CHECK(!engine->Failure());
        puts("PASS actual32 canonical8192Hz talker voices/51200-byte backlog/partial backpressure/invalid frame boundary/play/jitter/active retirement");
    }
    engine->StopEngine();reverbBus->DestroyVoice();worldBus->DestroyVoice();masterBus->DestroyVoice();master->DestroyVoice();engine->Release();
    CHECK(sourceFx.ReferenceCount()==1 && masterFx.ReferenceCount()==1 && reverbFx.ReferenceCount()==1 && capture.ReferenceCount()==1);
    printf("PASS real FAudio/canonical DSP graph: quantum%u frames, dry/wet buses, PCM/sample counters/pause/pitch/two completions, MSADPCM512 exact decoded1000, WMA rejection, owned float loop retirement, quiescent refs\n",capture.frames.load()/capture.passes.load());
}
#endif
int main() {
#if defined(_WIN32)
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
#endif
    SDL_SetMainReady();SoftwareDSP();
    IXAudio2 *engine=reinterpret_cast<IXAudio2 *>(1);CHECK(XAudio2Create(&engine,0,XAUDIO2_ANY_PROCESSOR)!=S_OK && !engine);
    bo1::android::InstallSDLEventOwner(Input,nullptr);
#if defined(__ANDROID__)
    CHECK(XAudio2Create(&engine,0,XAUDIO2_ANY_PROCESSOR)!=S_OK && !engine);
    CHECK(strstr(SDL_GetError(),"actual SDL Java setup"));
    puts("PASS HONOR canonical DSP512/canaries/typed LPF/aligned pointer retirement and actual missing-Java audio refusal; no Android device opened");
#else
    CHECK(SDL_SetHint(SDL_HINT_AUDIODRIVER,"dummy"));
    Graph();Graph();
    CHECK(SDL_InitSubSystem(SDL_INIT_AUDIO)==0);
    {
        bo1::android::VoiceCapture capture;CHECK(capture.Open(8192,1) && capture.Live());
        CHECK(capture.Controls(65535,false));CHECK(!capture.Controls(65536,false));CHECK(capture.Start());
        std::this_thread::sleep_for(std::chrono::milliseconds(500));std::array<short,4096> input{};
        CHECK(capture.Read(input)>0);CHECK(capture.Stop());CHECK(capture.Read(input)==0);
        CHECK(capture.Controls(10000,true));CHECK(capture.Start());std::this_thread::sleep_for(std::chrono::milliseconds(25));CHECK(capture.Read(input)==0);CHECK(capture.Stop());
    }
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    puts("PASS actual SDL dummy capture callback48000-to8192/ring/application mute+gain/stop/close quiescence; no physical microphone proof");
#endif
    bo1::android::RetireSDLEventOwner();
}
