"""Generate pinned-FAudio output-sample tests for checkpoint PlayBegin/loop bounds."""
from pathlib import Path
import argparse
root=Path(__file__).resolve().parents[3]
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);a=p.parse_args()
s=(root/'tools/port/tests/native_faudio_graph_tests.cpp').read_text(encoding='utf-8')
prefix=s[:s.index('static void Graph()')]
test=r'''
struct SeekTap:CXAPOBase {
 std::array<float,2048> values{};std::atomic<unsigned> count{};
 SeekTap():CXAPOBase(&captureProps){}
 HRESULT LockForProcess(unsigned a,const XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS*,unsigned b,const XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS*) override{return a==1&&b==1?S_OK:E_FAIL;}
 void UnlockForProcess() override{} void Reset() override{}
 void SetParameters(const void*,unsigned) override{} void GetParameters(void*,unsigned) override{}
 void Process(unsigned,const XAPO_PROCESS_BUFFER_PARAMETERS*in,unsigned,XAPO_PROCESS_BUFFER_PARAMETERS*out,int) override{
  if(in->BufferFlags==XAPO_BUFFER_VALID){unsigned n=count.load();const float*v=(const float*)in->pBuffer;
   for(unsigned i=0;i<in->ValidFrameCount && n<values.size();++i)values[n++]=v[i];
   count.store(n,std::memory_order_release);}
  if(out->pBuffer!=in->pBuffer && in->BufferFlags==XAPO_BUFFER_VALID)memcpy(out->pBuffer,in->pBuffer,in->ValidFrameCount*sizeof(float));
  out->BufferFlags=in->BufferFlags;out->ValidFrameCount=in->ValidFrameCount;
 }
};
static void SeekCase(unsigned mode){
 IXAudio2*engine=nullptr;CHECK(XAudio2Create(&engine,0,XAUDIO2_ANY_PROCESSOR)==S_OK);
 IXAudio2MasteringVoice*master=nullptr;CHECK(engine->CreateMasteringVoice(&master,2,48000,0,0,nullptr)==S_OK);
 SeekTap tap;XAUDIO2_EFFECT_DESCRIPTOR desc{&tap,1,1};XAUDIO2_EFFECT_CHAIN chain{1,&desc};
 WAVEFORMATEX format{};format.wFormatTag=mode==1?FAUDIO_FORMAT_IEEE_FLOAT:FAUDIO_FORMAT_PCM;format.nChannels=1;
 format.nSamplesPerSec=48000;format.wBitsPerSample=mode==1?32:16;format.nBlockAlign=format.wBitsPerSample/8;format.nAvgBytesPerSec=48000*format.nBlockAlign;
 struct ADPCM{WAVEFORMATEX base;unsigned short samples,coefficients;ADPCMCOEFSET coeff[7];}adpcm{};
 adpcm.base=format;adpcm.base.wFormatTag=FAUDIO_FORMAT_MSADPCM;adpcm.base.wBitsPerSample=4;adpcm.base.nBlockAlign=262;adpcm.base.cbSize=32;adpcm.samples=512;adpcm.coefficients=7;
 const ADPCMCOEFSET coeff[]={{256,0},{512,-256},{0,0},{192,64},{240,0},{460,-208},{392,-232}};memcpy(adpcm.coeff,coeff,sizeof(coeff));
 IXAudio2SourceVoice*voice=nullptr;CHECK(engine->CreateSourceVoice(&voice,mode==2?&adpcm.base:&format,0,2,nullptr,nullptr,&chain)==S_OK);
 std::vector<short> pcm(8192);std::vector<float> floats(8192);std::array<unsigned char,786> encoded{};
 for(unsigned i=0;i<8192;++i){pcm[i]=(short)(i*3-10000);floats[i]=(float)(i+1)/10000.f;}
 for(unsigned b=0;b<3;++b){unsigned short value=1000*(b+1);auto*p=encoded.data()+b*262;p[1]=16;p[3]=p[5]=value&255;p[4]=p[6]=value>>8;}
 XAUDIO2_BUFFER buffer{};unsigned frames=mode==2?1536:8192,offset=mode==2?512:7311;
 buffer.AudioBytes=mode==2?(unsigned)encoded.size():8192*format.nBlockAlign;
 buffer.pAudioData=mode==2?encoded.data():mode==1?(const unsigned char*)floats.data():(const unsigned char*)pcm.data();
 buffer.PlayBegin=offset;buffer.PlayLength=frames-offset;buffer.LoopBegin=0;buffer.LoopLength=frames;buffer.LoopCount=255;
 CHECK(voice->SubmitSourceBuffer(&buffer,nullptr)==S_OK && voice->Start(0,0)==S_OK && engine->StartEngine()==S_OK);
 Await([&]{return tap.count.load(std::memory_order_acquire)==tap.values.size();});engine->StopEngine();
 for(unsigned i=0;i<tap.values.size();++i){unsigned index=(offset+i)%frames;float expected=mode==2?(float)(1000*(index/512+1))/32768.f:mode==1?floats[index]:(float)pcm[index]/32768.f;
  if(std::fabs(tap.values[i]-expected)>1e-6f){std::printf("wave mismatch mode %u sample %u actual %.9f expected %.9f\n",mode,i,tap.values[i],expected);std::abort();}}
 voice->DestroyVoice();master->DestroyVoice();engine->Release();CHECK(tap.ReferenceCount()==1);
 std::printf("PASS mode %u exact seek start %u and full-loop wrap: 2048 actual decoded output samples\n",mode,offset);
}
#endif
int main(){SDL_SetMainReady();bo1::android::InstallSDLEventOwner(Input,nullptr);CHECK(SDL_SetHint(SDL_HINT_AUDIODRIVER,"dummy"));SeekCase(0);SeekCase(1);SeekCase(2);puts("CHECKPOINT WAVEFORM PASS: 6144 actual FAudio output samples, PCM16/decoded-float/aligned-ADPCM, nonzero offset and loop wrap");}
'''
a.out.mkdir(parents=True,exist_ok=False)
(a.out/'fixture.cpp').write_text(prefix+test,encoding='utf-8')
cmd=(root/'build-android-audit/native_registry_closure/faudio-graph-host-build.cmd').read_text()
cmd=cmd.replace(str(root/'tools/port/tests/native_faudio_graph_tests.cpp'),str(a.out.resolve()/'fixture.cpp'))
relative=a.out.resolve().relative_to(root/'build-android-audit').as_posix()
cmd=cmd.replace('native_registry_closure',relative)
(a.out/'build.cmd').write_text(cmd,encoding='utf-8')
