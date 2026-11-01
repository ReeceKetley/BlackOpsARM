"""Signed canonical playback clocks and checkpoint validation; no audio device fixture."""
from pathlib import Path
import argparse,json,hashlib
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);a=p.parse_args();a.out.mkdir(exist_ok=False)
def extract(path,key):
 s=Path(path).read_text(encoding='utf-8');i=s.index(key);j=s.index('{',i)+1;depth=1
 while depth:depth+=(s[j]=='{')-(s[j]=='}');j+=1
 return s[i:j]
files=['src/game_sp/g_sp_native_owners_checkpoint.inl','src/sound/snd_local.cpp','src/sound/snd.h']
src='''#include <sound/snd.h>
#include <cmath>
#include <limits>
#include <cstdio>
#include <cstdlib>
snd_local_t g_snd{};
static snd_voice_t voice{};
snd_voice_t * __cdecl SND_GetPlaybackVoice(int id){return id==7?&voice:nullptr;}
'''+extract(files[1],'int __cdecl SNDL_GetPlaybackTime(')+'\n'+extract(files[0],'static bool SP_ValidAudioPlayback(')+'''
static unsigned checks;
static void check(bool b){if(!b)std::abort();++checks;}
int main(){
 for(int time:{100,1000,100000})for(int pause:{0,50,100,1000})for(int start:{0,50,100,1000,100000}){
  g_snd.time=time;g_snd.pausetime=pause;voice.startTime=start;
  int expected=(pause?pause:time)-start;
  check(SNDL_GetPlaybackTime(7)==expected);check(SNDL_GetPlaybackTime(99)==0);
  snd_playback playback{7,1.f,1234,SNDL_GetPlaybackTime(7)};
  auto before=playback;check(SP_ValidAudioPlayback(playback));check(!memcmp(&before,&playback,sizeof(playback)));
 }
 for(int elapsed:{std::numeric_limits<int>::min(),-1000,-1,0,1,std::numeric_limits<int>::max()}){
  snd_playback playback{7,.5f,0,elapsed};check(SP_ValidAudioPlayback(playback));
  playback.lengthMs=-1;check(!SP_ValidAudioPlayback(playback));
  playback.lengthMs=100;for(float bad:{std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity()}){playback.attenuation=bad;check(!SP_ValidAudioPlayback(playback));}
 }
 printf("canonical signed playback/validator %u checks PASS pointer %zu\\n",checks,sizeof(void*));
}
'''
src=src.replace('#include <cmath>','#include <cmath>\n#include <cstring>\n#include <initializer_list>');(a.out/'fixture.cpp').write_text(src,encoding='utf-8');(a.out/'source-manifest.json').write_text(json.dumps({f:hashlib.sha256(Path(f).read_bytes()).hexdigest() for f in files},indent=2))
