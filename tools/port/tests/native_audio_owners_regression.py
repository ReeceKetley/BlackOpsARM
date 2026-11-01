"""Actual SDP3 codec/graph publication and canonical lookup/expiry on native types.

Reuses the SDP2 fixture's bounded in-memory IO and synthetic asset registry;
does not claim a device audio or gameplay frame test.
"""
from pathlib import Path
import argparse,subprocess,sys,json,hashlib
root=Path(__file__).resolve().parents[3]
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);a=p.parse_args()
subprocess.run([sys.executable,str(root/'tools/port/tests/native_audio_mixer_regression.py'),'--out',str(a.out)],check=True)
def extract(s,key):
 begin=s.index(key);end=s.index('{',begin)+1;depth=1
 while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
 return s[begin:end]
owner=root/'src/game_sp/g_sp_audio_owners_checkpoint.inl'
canonical=root/'src/sound/snd_public_async.cpp'
text=canonical.read_text()
helpers=r'''
void Com_Printf(int,const char*,...){}
static snd_alias_t testAlias{};
static snd_alias_list_t testList{"fixture_alias",51,&testAlias,1,0};
snd_alias_list_t *SND_FindAliasFromId(unsigned id){return id==51?&testList:nullptr;}
snd_alias_list_t *SND_FindAlias(const char *name){return !strcmp(name,"fixture_alias")?&testList:nullptr;}
void SND_EntStateRequest(SndEntHandle){std::abort();}
#undef PROF_SCOPED
#define PROF_SCOPED(...) ((void)0)
'''
helpers+='\n'.join(extract(text,k) for k in ['snd_ent_state *__cdecl SND_FindEntState(', 'void __cdecl SND_ResetEntState()', 'void __cdecl SND_EntStateFrame()'])
helpers+='\n'+owner.read_text()
extra=r'''
 static SPAudioOwnersCheckpoint owners{};
 owners.frame=100;for(auto &n:owners.next)n=-1;for(auto &n:owners.heads)n=-1;
 for(int i:{0,17,295}){auto &e=owners.entities[i];e.lastUsed=100;e.handle.handle=(i==295?200:7)|(unsigned(i+1)<<12);e.origin[0]=float(i);e.velocity[1]=3.25f;e.orientation[0][0]=1;e.orientation[1][1]=1;e.orientation[2][2]=1;}
 owners.heads[7]=17;owners.next[17]=0;owners.heads[200]=295;
 owners.lines[127].id=51;owners.lines[127].alias=&testList;owners.lines[127].origin[0][0]=25;
 owners.loops[255].id=51;owners.loops[255].origin[2]=300;
 check(SP_ValidAudioEntityGraph(owners));
 encoded={};SPNativeCheckpointCodec ownCapture{nullptr,&encoded};SP_ClientAudioOwnersCodec(ownCapture,owners);
 static SPAudioOwnersCheckpoint staged{};
 input={};decoded={};input.buffer=encoded.cacheBuffer;input.bufferSize=encoded.cacheBufferUsed;
 SPNativeCheckpointCodec ownDecode{&input,&decoded};SP_ClientAudioOwnersCodec(ownDecode,staged);
 check(encoded.cacheBufferUsed==decoded.cacheBufferUsed&&!memcmp(encoded.cacheBuffer,decoded.cacheBuffer,encoded.cacheBufferUsed));
 SP_PublishAudioOwners(staged);check(g_snd.entStateIndex[7]==&g_snd.entState[17]&&g_snd.entState[17].next==&g_snd.entState[0]);
 check(g_snd.lineEmitters[127].alias==&testList&&!g_snd.loopEmitters[255].alias&&g_snd.frame==100);
 static SPAudioOwnersCheckpoint queried{};SP_ReadLiveAudioOwners(queried);
 decoded={};SPNativeCheckpointCodec ownQuery{nullptr,&decoded};SP_ClientAudioOwnersCodec(ownQuery,queried);
 check(encoded.cacheBufferUsed==decoded.cacheBufferUsed&&!memcmp(encoded.cacheBuffer,decoded.cacheBuffer,encoded.cacheBufferUsed));
 for(int target=-3;target<300;++target){auto original=staged.heads[0];staged.heads[0]=target;check(SP_ValidAudioEntityGraph(staged)==(target==-1));staged.heads[0]=original;}
 for(int bad:{-3,-2,296,297,0,17}){int old=staged.next[0];staged.next[0]=bad;check(!SP_ValidAudioEntityGraph(staged));staged.next[0]=old;}
 staged.heads[7]=-1;check(!SP_ValidAudioEntityGraph(staged));staged.heads[7]=17;
 auto handle=staged.entities[0].handle.handle;
 for(unsigned bad:{0xFFFu,0xFFEu,7u|(1u<<21),200u}){staged.entities[0].handle.handle=bad;check(!SP_ValidAudioEntityGraph(staged));}
 staged.entities[0].handle=staged.entities[17].handle;check(!SP_ValidAudioEntityGraph(staged));staged.entities[0].handle.handle=handle;
 staged.entities[0].lastUsed=101;check(!SP_ValidAudioEntityGraph(staged));staged.entities[0].lastUsed=100;
 staged.frame=0;check(!SP_ValidAudioEntityGraph(staged));staged.frame=100;
 static SPAudioOwnersCheckpoint invalid;invalid=staged;invalid.next[0]=0;
 auto *beforeHead=g_snd.entStateIndex[7];rejects([&]{SP_PublishAudioOwners(invalid);});check(g_snd.entStateIndex[7]==beforeHead);
 // Real SND_EntStateFrame removes expired entries through their actual chains.
 g_snd.frame=103;SND_EntStateFrame();SP_ReadLiveAudioOwners(queried);check(SP_ValidAudioEntityGraph(queried));
 check(!g_snd.entStateIndex[7]&&!g_snd.entStateIndex[200]);
 for(int i:{0,17,295})check(!g_snd.entState[i].lastUsed&&!g_snd.entState[i].next);
 // Lazy alias state and fixed final slots survive; unknown aliases refuse.
 auto id=staged.loops[255].id;staged.loops[255].id=99;decoded={};rejects([&]{SP_ClientAudioOwnersCodec(ownQuery,staged);});staged.loops[255].id=id;
 auto value=staged.lines[127].origin[1][2];staged.lines[127].origin[1][2]=std::numeric_limits<float>::quiet_NaN();decoded={};rejects([&]{SP_ClientAudioOwnersCodec(ownQuery,staged);});staged.lines[127].origin[1][2]=value;
 printf("actual SDP3 owners and canonical lookup/expiry PASS; ");
'''
f=a.out/'fixture.cpp';s=f.read_text();s=s.replace('static void check(bool b)',helpers+'\nstatic void check(bool b)')
s=s.replace(' printf("actual SDP2 mixer',extra+'\n printf("actual SDP2 mixer')
f.write_text(s,encoding='utf-8')
manifest=json.loads((a.out/'source-manifest.json').read_text())
manifest += [{'file':str(f),'sha256':hashlib.sha256(f.read_bytes()).hexdigest()} for f in [owner,canonical]]
(a.out/'source-manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
