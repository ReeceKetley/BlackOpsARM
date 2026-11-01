"""Compare Android's real decoded PCM against independently decoded pinned FAudio."""
from pathlib import Path
import argparse
root=Path(__file__).resolve().parents[3]
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);a=p.parse_args()
s=(root/'tools/port/tests/native_faudio_graph_tests.cpp').read_text(encoding='utf-8')
prefix=s[:s.index('static void Graph()')]
test=r'''
struct AdpcmTap:CXAPOBase {
 std::array<float,4096> values{};std::atomic<unsigned> count{};unsigned channels;
 explicit AdpcmTap(unsigned ch):CXAPOBase(&captureProps),channels(ch){}
 HRESULT LockForProcess(unsigned a,const XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS*,unsigned b,const XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS*)override{return a==1&&b==1?S_OK:E_FAIL;}
 void UnlockForProcess()override{}void Reset()override{}
 void SetParameters(const void*,unsigned)override{}void GetParameters(void*,unsigned)override{}
 void Process(unsigned,const XAPO_PROCESS_BUFFER_PARAMETERS*in,unsigned,XAPO_PROCESS_BUFFER_PARAMETERS*out,int)override{
  if(in->BufferFlags==XAPO_BUFFER_VALID){unsigned n=count.load();const float*v=(const float*)in->pBuffer;
   for(unsigned i=0;i<in->ValidFrameCount*channels&&n<2048*channels;++i)values[n++]=v[i];
   count.store(n,std::memory_order_release);}
  if(out->pBuffer!=in->pBuffer&&in->BufferFlags==XAPO_BUFFER_VALID)memcpy(out->pBuffer,in->pBuffer,in->ValidFrameCount*channels*sizeof(float));
  out->BufferFlags=in->BufferFlags;out->ValidFrameCount=in->ValidFrameCount;
 }
};
static unsigned long long verified;
static void Compare(unsigned channels,unsigned predictor,const std::vector<unsigned char>&encoded,const std::vector<short>&pcm,unsigned offset,bool compressed){
 IXAudio2*engine=nullptr;CHECK(XAudio2Create(&engine,0,XAUDIO2_ANY_PROCESSOR)==S_OK);
 IXAudio2MasteringVoice*master=nullptr;CHECK(engine->CreateMasteringVoice(&master,2,48000,0,0,nullptr)==S_OK);
 AdpcmTap tap(channels);XAUDIO2_EFFECT_DESCRIPTOR desc{&tap,1,channels};XAUDIO2_EFFECT_CHAIN chain{1,&desc};
 WAVEFORMATEX format{};format.wFormatTag=FAUDIO_FORMAT_PCM;format.nChannels=channels;format.nSamplesPerSec=48000;
 format.wBitsPerSample=16;format.nBlockAlign=channels*2;format.nAvgBytesPerSec=48000*format.nBlockAlign;
 struct ADPCM{WAVEFORMATEX base;unsigned short samples,coefficients;ADPCMCOEFSET coeff[7];}adpcm{};
 adpcm.base=format;adpcm.base.wFormatTag=FAUDIO_FORMAT_MSADPCM;adpcm.base.wBitsPerSample=4;adpcm.base.nBlockAlign=262*channels;adpcm.base.cbSize=32;adpcm.samples=512;adpcm.coefficients=7;
 const ADPCMCOEFSET coeff[]={{256,0},{512,-256},{0,0},{192,64},{240,0},{460,-208},{392,-232}};memcpy(adpcm.coeff,coeff,sizeof(coeff));
 IXAudio2SourceVoice*voice=nullptr;CHECK(engine->CreateSourceVoice(&voice,compressed?&adpcm.base:&format,0,2,nullptr,nullptr,&chain)==S_OK);
 XAUDIO2_BUFFER buffer{};buffer.AudioBytes=compressed?(unsigned)encoded.size():(unsigned)pcm.size()*sizeof(short);
 buffer.pAudioData=compressed?encoded.data():(const unsigned char*)pcm.data();buffer.PlayBegin=offset;buffer.PlayLength=1536-offset;buffer.LoopLength=1536;buffer.LoopCount=255;
 CHECK(voice->SubmitSourceBuffer(&buffer,nullptr)==S_OK&&voice->Start(0,0)==S_OK&&engine->StartEngine()==S_OK);
 Await([&]{return tap.count.load(std::memory_order_acquire)==2048*channels;});engine->StopEngine();
 for(unsigned i=0;i<2048*channels;++i){unsigned index=(((offset+i/channels)%1536)*channels)+i%channels;float expected=(float)pcm[index]/32768.f;
  if(std::fabs(tap.values[i]-expected)>1e-6f){fprintf(stderr,"wave mismatch ch%u predictor%u compressed%i offset%u sample%u actual%.9f expected%.9f\n",channels,predictor,compressed,offset,i,tap.values[i],expected);std::abort();}++verified;}
 voice->DestroyVoice();master->DestroyVoice();engine->Release();CHECK(tap.ReferenceCount()==1);
 printf("PASS ch%u predictor%u compressed%i exact start%u and loop wrap\n",channels,predictor,compressed,offset);
}
#endif
int main(int argc,char**argv){CHECK(argc==2);SDL_SetMainReady();bo1::android::InstallSDLEventOwner(Input,nullptr);CHECK(SDL_SetHint(SDL_HINT_AUDIODRIVER,"dummy"));FILE*f=fopen(argv[1],"rb");CHECK(f);
 for(unsigned channels:{1u,2u})for(unsigned predictor=0;predictor<7;++predictor){std::vector<unsigned char>encoded(262*channels*3);std::vector<short>pcm(1536*channels);
  CHECK(fread(encoded.data(),1,encoded.size(),f)==encoded.size());CHECK(fread(pcm.data(),sizeof(short),pcm.size(),f)==pcm.size());
  Compare(channels,predictor,encoded,pcm,0,true);
  for(unsigned offset:{1u,511u,513u,1535u})Compare(channels,predictor,encoded,pcm,offset,false);
 }CHECK(fgetc(f)==EOF);CHECK(!fclose(f));printf("ADPCM WAVEFORM PASS %llu actual FAudio output samples; Android decoded PCM equals independent SDK compressed decoding and exact unaligned seek/loop wrap\n",verified);
}
'''
a.out.mkdir(parents=True,exist_ok=False)
(a.out/'fixture.cpp').write_text(prefix+test,encoding='utf-8')
cmd=(root/'build-android-audit/native_registry_closure/faudio-graph-host-build.cmd').read_text()
cmd=cmd.replace(str(root/'tools/port/tests/native_faudio_graph_tests.cpp'),str(a.out.resolve()/'fixture.cpp'))
cmd=cmd.replace('native_registry_closure',a.out.resolve().relative_to(root/'build-android-audit').as_posix())
(a.out/'build.cmd').write_text(cmd,encoding='utf-8')
