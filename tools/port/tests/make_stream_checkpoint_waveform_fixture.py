"""Pinned SDK streaming waveform and actual production callback retirement body."""
from pathlib import Path
import argparse,hashlib,json
root=Path(__file__).resolve().parents[3]
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);a=p.parse_args()
s=(root/'tools/port/tests/native_faudio_graph_tests.cpp').read_text(encoding='utf-8')
prefix=s[:s.index('static void Graph()')]
driver=root/'src/sound/snd_driver_xaudio2.cpp';body=driver.read_text()
begin=body.index('void __cdecl iSND_ReleaseStreamBuffer(');end=body.index('{',begin)+1;depth=1
while depth:depth+=(body[end]=='{')-(body[end]=='}');end+=1
tap=(root/'tools/port/tests/make_adpcm_checkpoint_waveform_fixture.py').read_text()
tap=tap[tap.index('struct AdpcmTap:'):tap.index('static unsigned long long verified;')]
tap=tap.replace('4096','32768').replace('2048','16384')
test=r'''
#include <sound/snd_driver_xaudio2.h>
#include <platform/android/wma_decoder.h>
SoundState g_sd;
static std::atomic<bool> sd_checkpointStreamPcm[SND_MAX_STREAM_VOICES];
static std::unique_ptr<bo1::android::DecodedMsAdpcm> sd_checkpointStreamWindows[SND_MAX_STREAM_VOICES][2];
// A PCM callback must never return a pointer to the compressed file-window pool.
void Snd_StreamReleaseWindow(unsigned,char*){std::abort();}
ACTUAL_RELEASE
TAP
static unsigned long long verified;static unsigned windows,retired;
static void Run(unsigned channels,unsigned phase,const std::vector<short>&pcm){
 IXAudio2*engine=nullptr;CHECK(XAudio2Create(&engine,0,XAUDIO2_ANY_PROCESSOR)==S_OK);
 IXAudio2MasteringVoice*master=nullptr;CHECK(engine->CreateMasteringVoice(&master,2,48000,0,0,nullptr)==S_OK);
 AdpcmTap tap(channels);XAUDIO2_EFFECT_DESCRIPTOR desc{&tap,1,channels};XAUDIO2_EFFECT_CHAIN chain{1,&desc};
 WAVEFORMATEX format{};format.wFormatTag=FAUDIO_FORMAT_PCM;format.nChannels=channels;format.nSamplesPerSec=48000;
 format.wBitsPerSample=16;format.nBlockAlign=channels*2;format.nAvgBytesPerSec=48000*format.nBlockAlign;
 auto&sv=g_sd.streamVoices[0];sv.bufferQueuedCount=0;
 CHECK(engine->CreateSourceVoice(&g_sd.voices[0],&format,0,2,&sv,nullptr,&chain)==S_OK);
 sd_checkpointStreamPcm[0].store(true,std::memory_order_release);
 unsigned block=phase/512,skip=phase%512;
 auto Pump=[&]{
  XAUDIO2_VOICE_STATE state{};g_sd.voices[0]->GetState(&state);
  for(unsigned queued=state.BuffersQueued;queued<2;++queued){unsigned slot=sv.bufferQueuedCount%2;auto&buffer=sv.buffers[slot];
   if(std::atomic_ref(buffer.pAudioData).load(std::memory_order_acquire))break;
   if(sd_checkpointStreamWindows[0][slot])++retired;
   auto next=std::make_unique<bo1::android::DecodedMsAdpcm>();next->samples.assign(pcm.begin()+block*512*channels,pcm.begin()+(block+1)*512*channels);
   sd_checkpointStreamWindows[0][slot]=std::move(next);memset(&buffer,0,sizeof(buffer));
   buffer.AudioBytes=512*channels*sizeof(short);buffer.PlayBegin=skip;buffer.PlayLength=512-skip;skip=0;
   buffer.pContext=reinterpret_cast<void*>(static_cast<uintptr_t>(slot));
   std::atomic_ref(buffer.pAudioData).store(reinterpret_cast<const std::uint8_t*>(sd_checkpointStreamWindows[0][slot]->samples.data()),std::memory_order_release);
   CHECK(g_sd.voices[0]->SubmitSourceBuffer(&buffer,nullptr)==S_OK);++sv.bufferQueuedCount;++windows;block=(block+1)%3;
  }
 };
 Pump();CHECK(g_sd.voices[0]->Start(0,0)==S_OK&&engine->StartEngine()==S_OK);
 Await([&]{Pump();return tap.count.load(std::memory_order_acquire)==16384*channels;});engine->StopEngine();
 for(unsigned i=0;i<16384*channels;++i){unsigned index=(((phase+i/channels)%1536)*channels)+i%channels;float expected=(float)pcm[index]/32768.f;
  if(std::fabs(tap.values[i]-expected)>1e-6f){fprintf(stderr,"stream mismatch ch%u phase%u sample%u actual%.9f expected%.9f\n",channels,phase,i,tap.values[i],expected);std::abort();}++verified;}
 g_sd.voices[0]->DestroyVoice();g_sd.voices[0]=nullptr;
 for(unsigned slot=0;slot<2;++slot){iSND_ReleaseStreamBuffer(0,slot);CHECK(!std::atomic_ref(sv.buffers[slot].pAudioData).load(std::memory_order_acquire));sd_checkpointStreamWindows[0][slot].reset();}
 sd_checkpointStreamPcm[0].store(false,std::memory_order_release);master->DestroyVoice();engine->Release();CHECK(tap.ReferenceCount()==1);
 printf("PASS streamed PCM ch%u exact start%u, continuous file-loop/window boundaries and actual callback retirement\n",channels,phase);
}
#endif
int main(int argc,char**argv){CHECK(argc==2);SDL_SetMainReady();bo1::android::InstallSDLEventOwner(Input,nullptr);CHECK(SDL_SetHint(SDL_HINT_AUDIODRIVER,"dummy"));FILE*f=fopen(argv[1],"rb");CHECK(f);
 for(unsigned channels:{1u,2u})for(unsigned predictor=0;predictor<7;++predictor){std::vector<unsigned char>encoded(262*channels*3);std::vector<short>pcm(1536*channels);
  CHECK(fread(encoded.data(),1,encoded.size(),f)==encoded.size());CHECK(fread(pcm.data(),sizeof(short),pcm.size(),f)==pcm.size());
  if(predictor==6)for(unsigned phase:{1u,511u,513u,1535u})Run(channels,phase,pcm);
 }CHECK(fgetc(f)==EOF);CHECK(!fclose(f));printf("STREAM WAVEFORM PASS %llu actual FAudio output samples, %u submitted windows / %u callback-retired PCM owners, mono/stereo/unaligned starts/loop wrap\n",verified,windows,retired);
}
'''
a.out.mkdir(parents=True,exist_ok=False)
(a.out/'fixture.cpp').write_text(prefix+test.replace('ACTUAL_RELEASE',body[begin:end]).replace('TAP',tap),encoding='utf-8')
cmd=(root/'build-android-audit/native_registry_closure/faudio-graph-host-build.cmd').read_text()
cmd=cmd.replace(str(root/'tools/port/tests/native_faudio_graph_tests.cpp'),str(a.out.resolve()/'fixture.cpp'))
cmd=cmd.replace('native_registry_closure',a.out.resolve().relative_to(root/'build-android-audit').as_posix())
(a.out/'build.cmd').write_text(cmd,encoding='utf-8')
(a.out/'source-manifest.json').write_text(json.dumps({'file':str(driver),'sha256':hashlib.sha256(driver.read_bytes()).hexdigest()},indent=2),encoding='utf-8')
