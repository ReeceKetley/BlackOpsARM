"""Actual SDP2 codec/publication with canonical types; synthetic asset registry only.

This checks wire/ABI/corruption guards, not device DSP or audible output.
"""
from pathlib import Path
import argparse, hashlib, json

root = Path(__file__).resolve().parents[3]
p = argparse.ArgumentParser()
p.add_argument('--out', type=Path, required=True)
a = p.parse_args()
owners = root/'src/game_sp/g_sp_native_owners_checkpoint.inl'
mixer = root/'src/game_sp/g_sp_audio_mixer_checkpoint.inl'
s = owners.read_text()
start = s.index('struct SPNativeCheckpointCodec')
end = s.index('{', start)+1
depth = 1
while depth:
    depth += (s[end] == '{')-(s[end] == '}')
    end += 1
codec = s[start:end]+';'
fixture = r'''
#include <universal/memfile.h>
#include <qcommon/common.h>
#include <sound/snd.h>
#include <sound/snd_bank.h>
#include <sound/snd_radverb.h>
#include <type_traits>
#include <stdexcept>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <limits>
#include <vector>
snd_local_t g_snd{};
static unsigned checks;
void Com_Error(errorParm_t,const char*,...){throw std::runtime_error("rejected");}
bool Assert_MyHandler(const char*,int,int,const char*,...){std::abort();}
void MemFile_ReadData(MemoryFile*f,int n,unsigned char*out){
 if(n<0 || n>f->bufferSize-f->bytesUsed)Com_Error(ERR_DROP,"truncated");
 memcpy(out,f->buffer+f->bytesUsed,n);f->bytesUsed+=n;
}
char *MemFile_ReadCString(MemoryFile*f){
 char *p=(char*)f->buffer+f->bytesUsed;
 for(;;){unsigned char b;MemFile_ReadData(f,1,&b);if(!b)return p;}
}
void MemFile_WriteDataInternal(MemoryFile*,unsigned,unsigned char*){Com_Error(ERR_DROP,"unexpected flush");}
static snd_radverb reverb{};
static snd_snapshot snapshot{};
const snd_radverb *SND_GetRadverb(unsigned){return &reverb;}
const snd_snapshot *SND_GetSnapshotById(unsigned){return &snapshot;}
ACTUAL_CODEC
ACTUAL_MIXER
static void check(bool b){if(!b)std::abort();++checks;}
static MemoryFile encoded,decoded,input;
static void encode(SPAudioMixerCheckpoint &m){encoded={};SPNativeCheckpointCodec c{nullptr,&encoded};SP_ClientAudioMixerCodec(c,m);}
static void decode(SPAudioMixerCheckpoint &m){
 input={};decoded={};input.buffer=encoded.cacheBuffer;input.bufferSize=encoded.cacheBufferUsed;
 SPNativeCheckpointCodec c{&input,&decoded};SP_ClientAudioMixerCodec(c,m);
}
template<class F>static void rejects(F f){bool rejected=false;try{f();}catch(const std::runtime_error&){rejected=true;}check(rejected);}
int main(){
 static snd_group groups[2]{};strcpy(groups[0].name,"first");strcpy(groups[1].name,"second");groups[0].id=11;groups[1].id=12;
 static snd_snapshot_group channels[2]{};strcpy(channels[0].name,"front");strcpy(channels[1].name,"back");
 static snd_context contexts[1]{};contexts[0].type=21;contexts[0].valueCount=2;contexts[0].values[0]=22;contexts[0].values[1]=23;
 static SndDriverGlobals globals{};globals.name="fixture";globals.groups=groups;globals.groupCount=2;globals.snapshotGroups=channels;globals.snapshotGroupCount=2;globals.contexts=contexts;globals.contextCount=1;
 g_snd.global_constants=&globals;strcpy(reverb.name,"room");reverb.id=31;strcpy(snapshot.name,"underwater");snapshot.id=41;
 SPAudioMixerCheckpoint m{};m.effect=2;m.envEffects[2].active=true;m.envEffects[2].reverbId=31;m.currentContexts[0]=22;
 for(auto &s:m.snapshots)s.snapshot=41;
 std::vector<float*> floats;
 auto add=[&](float &f){floats.push_back(&f);};
 auto array=[&](auto &a){for(float &f:a)add(f);};
 auto fader=[&](snd_fader_t &f){add(f.value);add(f.goal);add(f.rate);};
 add(m.gameState.timescale);add(m.timescale);add(m.scriptTimescale);fader(m.volume);fader(m.minimumPriority);add(m.hdrLoad);add(m.maximumPriority);
 for(auto &e:m.envEffects){add(e.drylevel);add(e.drygoal);add(e.dryrate);add(e.wetlevel);add(e.wetgoal);add(e.wetrate);}
 array(m.listener.orient.origin);for(auto &a:m.listener.orient.axis)array(a);
 for(auto &s:m.snapshots){add(s.length);add(s.amount);for(auto &f:s.attenuation)fader(f);for(auto &f:s.occlusion)fader(f);}
 array(m.attenuation);array(m.occlusion);array(m.proximityOrigin);array(m.proximityFront);array(m.proximityDistance);array(m.proximityReflectance);array(m.proximityLateModifier);
 for(unsigned i=0;i<floats.size();++i)*floats[i]=float(i%47)*.03125f;
 m.listener.active=true;m.listener.clientNum=0;m.gameState.seed=0xABCDE;m.gameState.cgTime=12345;
 encode(m);SPAudioMixerCheckpoint restored{};decode(restored);
 check(encoded.cacheBufferUsed==decoded.cacheBufferUsed&&!memcmp(encoded.cacheBuffer,decoded.cacheBuffer,encoded.cacheBufferUsed));
 SP_PublishAudioMixer(restored);SPAudioMixerCheckpoint query{};SP_ReadLiveAudioMixer(query);encode(query);decode(restored);
 check(g_snd.effect==&g_snd.envEffects[2]&&g_snd.currentContexts[0]==22);
 check(encoded.cacheBufferUsed==decoded.cacheBufferUsed&&!memcmp(encoded.cacheBuffer,decoded.cacheBuffer,encoded.cacheBufferUsed));
 // Every persisted float is guarded; rejection must never publish to g_snd.
 static snd_local_t before;memcpy(&before,&g_snd,sizeof(g_snd));
 for(float *f:floats){float old=*f;for(float bad:{std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity()}){*f=bad;rejects([&]{encode(m);});check(!memcmp(&before,&g_snd,sizeof(g_snd)));}*f=old;}
 for(int bad:{-2,3,100}){m.effect=bad;rejects([&]{encode(m);});}m.effect=2;
 m.currentContexts[0]=99;rejects([&]{encode(m);});m.currentContexts[0]=22;
 m.currentContexts[15]=23;rejects([&]{encode(m);});m.currentContexts[15]=0;
 // Production context lookup accepts any matching one of all eight slots.
 contexts[0].values[7]=24;m.currentContexts[0]=24;encode(m);decode(restored);check(restored.currentContexts[0]==24);contexts[0].values[7]=0;m.currentContexts[0]=22;
 m.envEffects[2].reverbId=99;rejects([&]{encode(m);});m.envEffects[2].reverbId=31;
 m.snapshots[3].snapshot=99;rejects([&]{encode(m);});m.snapshots[3].snapshot=41;
 for(int bad:{-1,2,100}){m.paused=bad;rejects([&]{encode(m);});}m.paused=0;
 encode(m);groups[0].id=99;rejects([&]{decode(restored);});groups[0].id=11;
 strcpy(groups[1].name,"changed");rejects([&]{decode(restored);});strcpy(groups[1].name,"second");
 contexts[0].values[0]=99;rejects([&]{decode(restored);});contexts[0].values[0]=22;
 globals.contextCount=17;rejects([&]{decode(restored);});globals.contextCount=1;
 globals.snapshotGroupCount=65;rejects([&]{decode(restored);});globals.snapshotGroupCount=2;
 globals.contexts[0].valueCount=9;rejects([&]{decode(restored);});globals.contexts[0].valueCount=2;
 m.effect=-1;encode(m);decode(restored);SP_PublishAudioMixer(restored);check(!g_snd.effect);
 printf("actual SDP2 mixer %u checks PASS, %zu guarded float fields, canonical pointer %zu bytes\n",checks,floats.size(),sizeof(void*));
}
'''
a.out.mkdir(parents=True, exist_ok=False)
(a.out/'fixture.cpp').write_text(fixture.replace('ACTUAL_CODEC', codec).replace('ACTUAL_MIXER', mixer.read_text()), encoding='utf-8')
(a.out/'source-manifest.json').write_text(json.dumps([{'file':str(f),'sha256':hashlib.sha256(f.read_bytes()).hexdigest()} for f in [owners,mixer]],indent=2),encoding='utf-8')
