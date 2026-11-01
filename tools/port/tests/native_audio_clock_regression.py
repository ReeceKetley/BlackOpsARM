"""Actual staged audio clock rebase, canonical voice types, transactional overflow checks."""
from pathlib import Path
import argparse,hashlib,json
root=Path(__file__).resolve().parents[3];p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);a=p.parse_args()
pub=root/'src/game_sp/g_sp_audio_publication.inl';owners=root/'src/game_sp/g_sp_native_owners_checkpoint.inl'
def extract(s,key):
 begin=s.index(key);end=s.index('{',begin)+1;depth=1
 while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
 return s[begin:end]
stage=extract((root/'src/game_sp/g_sp_audio_mixer_checkpoint.inl').read_text(),'struct SPAudioMixerCheckpoint')+';\n'+extract((root/'src/game_sp/g_sp_audio_owners_checkpoint.inl').read_text(),'struct SPAudioOwnersCheckpoint')+';\n'+extract((root/'src/game_sp/g_sp_audio_occlusion_checkpoint.inl').read_text(),'struct SPAudioOcclusionCheckpoint')+';\n'+extract(owners.read_text(),'struct SPAudioCheckpointStage')+';'
rebase='\n'.join(extract(pub.read_text(),name) for name in ['static bool SP_UnpauseSavedAudio(', 'static bool SP_RebaseAudioClock(', 'static bool SP_ValidateSavedAudioRegistry('])
fixture=r'''
#include <sound/snd.h>
#include <limits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
STAGE
REBASE
static unsigned checks;static void check(bool b){if(!b)std::abort();++checks;}
int main(){
 const int values[]={0,1,1000,100000,1000000000,std::numeric_limits<int>::max()};
 const int timestamps[]={std::numeric_limits<int>::min(),-1000000000,-1000,0,1,500,1000,1000000000,std::numeric_limits<int>::max()};
 for(int saved:values)for(int now:values)for(int t:timestamps)for(int selected=0;selected<3;++selected){
  SPAudioCheckpointStage original{};original.time=saved;original.looptime=7;original.counter=42;original.used=1;original.playbacks[0]={41,.75f,30000,1234};original.active[10]=true;
  auto&v=original.voices[10];v.startTime=saved;v.soundFileInfo.endtime=saved;v.voiceStartTime=saved;
  if(selected==0)v.startTime=t;else if(selected==1)v.soundFileInfo.endtime=t;else v.voiceStartTime=t;
  unsigned char ff=255;memcpy(&v.timescale,&ff,1);original.cursors[10]=987654321;original.slots[10]=0;
  SPAudioCheckpointStage actual;memcpy(&actual,&original,sizeof(actual));const auto shift=(long long)now-saved;
  const auto target=(long long)t+shift;const auto interval=selected==1?(long long)t-saved:(long long)saved-t;
  const bool valid=target>=std::numeric_limits<int>::min()&&target<=std::numeric_limits<int>::max()
   &&interval>=std::numeric_limits<int>::min()&&interval<=std::numeric_limits<int>::max();
  check(SP_RebaseAudioClock(actual,now)==valid);
  if(!valid){check(!memcmp(&actual,&original,sizeof(actual)));continue;}
  check(actual.time==now&&actual.looptime==7&&actual.counter==42&&actual.used==1);
  check((long long)actual.time-actual.voices[10].startTime==(long long)original.time-v.startTime);
  check((long long)actual.voices[10].soundFileInfo.endtime-actual.time==(long long)v.soundFileInfo.endtime-original.time);
  check((long long)actual.time-actual.voices[10].voiceStartTime==(long long)original.time-v.voiceStartTime);
  check(!memcmp(actual.playbacks,original.playbacks,sizeof(actual.playbacks))&&actual.cursors[10]==original.cursors[10]&&actual.slots[10]==0);
  unsigned char raw;memcpy(&raw,&actual.voices[10].timescale,1);check(raw==255);
 }
 for(int which=0;which<3;++which){SPAudioCheckpointStage before{};before.time=100;auto after=before;if(which==0)after.time=-1;if(which==1)after.pausetime=1;before=after;check(!SP_RebaseAudioClock(after,which==2?-1:100));check(!memcmp(&after,&before,sizeof(after)));}
 for(int held:{0,1})for(int pause:{0,1,1000,999999}){
  SPAudioCheckpointStage original{};original.time=1000000;original.pausetime=pause;original.hasMixer=true;
  original.mixer.paused=original.mixer.forcePause=1;original.mixer.gameState.gamePaused=true;
  original.active[2]=original.active[12]=true;
  original.voices[2].startTime=original.voices[12].startTime=5;
  original.voices[2].soundFileInfo.endtime=original.voices[12].soundFileInfo.endtime=1000001;
  original.voices[2].voiceStartTime=17;original.voices[2].paused=held;
  original.cursors[2]=1234567;original.slots[2]=17;
  auto actual=original;check(SP_UnpauseSavedAudio(actual));
  const int shift=held&&pause?original.time-pause:0;
  check(actual.voices[2].startTime==5+shift&&actual.voices[2].soundFileInfo.endtime==1000001+shift);
  check(actual.voices[12].startTime==5&&actual.voices[12].soundFileInfo.endtime==1000001);
  check(!actual.pausetime&&!actual.mixer.paused&&!actual.mixer.forcePause&&!actual.mixer.gameState.gamePaused);
  check(!actual.voices[2].paused&&actual.voices[2].voiceStartTime==17&&actual.cursors[2]==1234567&&actual.slots[2]==17);
  auto normalized=actual;check(SP_UnpauseSavedAudio(actual));check(!memcmp(&normalized,&actual,sizeof(actual)));
  check(SP_RebaseAudioClock(actual,30000));
  check(actual.time-actual.voices[2].startTime==original.time-(5+shift));
 }
 for(int invalid=0;invalid<4;++invalid){
  SPAudioCheckpointStage before{};before.time=1000;before.pausetime=500;before.active[73]=true;before.voices[73].paused=true;
  if(invalid==0)before.time=-1;if(invalid==1)before.pausetime=-1;if(invalid==2)before.pausetime=1001;
  if(invalid==3)before.voices[73].soundFileInfo.endtime=std::numeric_limits<int>::max();
  auto actual=before;check(!SP_UnpauseSavedAudio(actual));check(!memcmp(&before,&actual,sizeof(actual)));
 }
 for(int used:{0,1,73,147,148})for(unsigned counter:{0u,1u,148u,100000u}){
  SPAudioCheckpointStage stage{};stage.time=123;stage.used=used;stage.counter=counter;
  for(int i=0;i<148;++i)stage.playbacks[i].id=i<used?i+1:-1;
  auto before=stage;
  check(SP_ValidateSavedAudioRegistry(stage,true)==(counter>=static_cast<unsigned>(used)));
  check(SP_ValidateSavedAudioRegistry(stage,false)==(counter>=static_cast<unsigned>(used)&&counter&&used<148));
  check(!memcmp(&stage,&before,sizeof(stage)));
 }
 for(int fault=0;fault<8;++fault){
  SPAudioCheckpointStage stage{};stage.time=100;stage.counter=148;stage.used=2;
  for(auto&p:stage.playbacks)p.id=-1;stage.playbacks[0].id=1;stage.playbacks[147].id=148;
  if(fault==0)stage.time=-1;if(fault==1)stage.pausetime=1;if(fault==2)stage.counter=0x7FFFFFFEu;
  if(fault==3)stage.used=149;if(fault==4)stage.used=1;if(fault==5)stage.playbacks[147].id=1;
  if(fault==6)stage.playbacks[147].id=149;if(fault==7)stage.playbacks[147].id=0;
  auto before=stage;check(!SP_ValidateSavedAudioRegistry(stage,true));check(!memcmp(&stage,&before,sizeof(stage)));
 }
 printf("actual audio clock/unpause/registry %u checks PASS, canonical pointer %zu bytes\n",checks,sizeof(void*));
}
'''
a.out.mkdir(parents=True,exist_ok=False);(a.out/'fixture.cpp').write_text(fixture.replace('STAGE',stage).replace('REBASE',rebase),encoding='utf-8')
(a.out/'source-manifest.json').write_text(json.dumps([{'file':str(f),'sha256':hashlib.sha256(f.read_bytes()).hexdigest()} for f in [pub,owners,root/'src/game_sp/g_sp_audio_mixer_checkpoint.inl',root/'src/game_sp/g_sp_audio_owners_checkpoint.inl',root/'src/game_sp/g_sp_audio_occlusion_checkpoint.inl']],indent=2),encoding='utf-8')
