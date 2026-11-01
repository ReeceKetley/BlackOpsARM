"""Actual SDP4 logical trace codec, canonical types and derived-hint invalidation."""
from pathlib import Path
import argparse,subprocess,sys,json,hashlib
root=Path(__file__).resolve().parents[3]
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);a=p.parse_args()
subprocess.run([sys.executable,str(root/'tools/port/tests/native_audio_owners_regression.py'),'--out',str(a.out)],check=True)
source=root/'src/game_sp/g_sp_audio_occlusion_checkpoint.inl'
helpers='static bool testOcclusionGate;\nbool SND_OcclusionCheckpointActive(){return testOcclusionGate;}\n'+source.read_text()
extra=r'''
 static SPAudioOcclusionCheckpoint occlusion{};
 rejects([&]{SP_ReadLiveAudioOcclusion(occlusion);});rejects([&]{SP_PublishAudioOcclusion(occlusion);});
 testOcclusionGate=true;
 for(auto &t:g_snd.occlusionTraces){t={};t.id=-1;}
 g_snd.traceSequence=g_snd.lastTraceSequence=123;g_snd.framesSinceLastOcclusionUpdate=2;
 g_snd.occlusionTraces[73].id=99;g_snd.occlusionTraces[73].occlusion=.375f;g_snd.occlusionTraces[73].cache=317;
 SP_ReadLiveAudioOcclusion(occlusion);check(!occlusion.traces[73].cache&&g_snd.occlusionTraces[73].cache==317);
 encoded={};SPNativeCheckpointCodec ocCapture{nullptr,&encoded};SP_ClientAudioOcclusionCodec(ocCapture,occlusion);
 static SPAudioOcclusionCheckpoint ocStaged{};
 input={};decoded={};input.buffer=encoded.cacheBuffer;input.bufferSize=encoded.cacheBufferUsed;
 SPNativeCheckpointCodec ocDecode{&input,&decoded};SP_ClientAudioOcclusionCodec(ocDecode,ocStaged);
 check(encoded.cacheBufferUsed==decoded.cacheBufferUsed&&!memcmp(encoded.cacheBuffer,decoded.cacheBuffer,encoded.cacheBufferUsed));
 SP_PublishAudioOcclusion(ocStaged);check(!g_snd.occlusionTraces[73].cache&&g_snd.occlusionTraces[73].id==99&&g_snd.occlusionTraces[73].occlusion==.375f);
 check(g_snd.traceSequence==123&&g_snd.lastTraceSequence==123&&g_snd.framesSinceLastOcclusionUpdate==2);
 for(auto &t:ocStaged.traces){
  std::vector<float*> fields;for(auto &v:t.listener)fields.push_back(&v);for(auto &v:t.playback)fields.push_back(&v);fields.push_back(&t.occlusion);
  for(float *f:fields){float old=*f;for(float bad:{std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity()}){*f=bad;decoded={};rejects([&]{SP_ClientAudioOcclusionCodec(ownQuery,ocStaged);});check(g_snd.occlusionTraces[73].occlusion==.375f);}*f=old;}
  float old=t.occlusion;for(float bad:{-.01f,1.01f}){t.occlusion=bad;decoded={};rejects([&]{SP_ClientAudioOcclusionCodec(ownQuery,ocStaged);});}t.occlusion=old;
  int id=t.id;t.id=-2;decoded={};rejects([&]{SP_ClientAudioOcclusionCodec(ownQuery,ocStaged);});t.id=id;
 }
 for(auto pair:{std::pair<unsigned,unsigned>{0,0},{0,0xFFFFFFFFu},{124,123}}){ocStaged.sequence=pair.first;ocStaged.completed=pair.second;decoded={};SP_ClientAudioOcclusionCodec(ownQuery,ocStaged);check(true);}
 for(auto pair:{std::pair<unsigned,unsigned>{2,0},{0,1},{123,121}}){ocStaged.sequence=pair.first;ocStaged.completed=pair.second;decoded={};rejects([&]{SP_ClientAudioOcclusionCodec(ownQuery,ocStaged);});}
 printf("actual SDP4 occlusion codec/hint invalidation PASS; ");
'''
f=a.out/'fixture.cpp';s=f.read_text().replace('static void check(bool b)',helpers+'\nstatic void check(bool b)')
s=s.replace(' printf("actual SDP2 mixer',extra+'\n printf("actual SDP2 mixer');f.write_text(s,encoding='utf-8')
m=json.loads((a.out/'source-manifest.json').read_text());m.append({'file':str(source),'sha256':hashlib.sha256(source.read_bytes()).hexdigest()})
(a.out/'source-manifest.json').write_text(json.dumps(m,indent=2),encoding='utf-8')
