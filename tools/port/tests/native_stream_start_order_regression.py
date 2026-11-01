"""Production streamed-voice factory with isolated I/O/device providers; verifies first-update state."""
from pathlib import Path
import argparse,json,hashlib
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);p.add_argument('--source',type=Path,default=Path('src/sound/snd_driver_xaudio2.cpp'));a=p.parse_args();a.out.mkdir(exist_ok=False)
s=a.source.read_text(encoding='utf-8');i=s.index('int __cdecl SND_StartAliasStream(');j=s.index('{',i)+1;n=1
while n:n+=(s[j]=='{')-(s[j]=='}');j+=1
src=r'''
#include <sound/snd.h>
#include <sound/snd_db.h>
#include <sound/snd_stream.h>
#include <sound/snd_driver_api.h>
#include <qcommon/common.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
snd_local_t g_snd{};
static int step;static unsigned checks;static void check(bool b){if(!b)std::abort();++checks;}
unsigned int __cdecl iSND_GetStreamChannel(unsigned int i){check(i<10);return i;}
void __cdecl SND_AliasGetFileName(const snd_alias_t*,char*f,int n){check(n>=8);strcpy(f,"fixture");}
void __cdecl Snd_StreamOpen(unsigned int i,char*,bool,unsigned int,char*){check(i<10&&step==0);step=1;}
void __cdecl SND_SetSoundFileVoiceInfo(unsigned int i,int channels,int rate,unsigned int length,int start,SndFileLoadingState loading){check(step==1&&channels==0&&rate==0&&length==0&&start==0&&loading==SFLS_LOADING);g_snd.voice[i].soundFileInfo={loading,channels,rate,g_snd.time,static_cast<int>(length)};step=2;}
void __cdecl SND_SetVoiceStartInfo(unsigned int i,SndStartAliasInfo*info){check(step==2);const auto&f=g_snd.voice[i].soundFileInfo;check(f.loadingState==SFLS_LOADING&&f.totalMsec==0&&f.srcChannelCount==0&&f.baserate==0);g_snd.voice[i].alias=info->alias;step=3;}
void __cdecl SD_UpdateVoice(unsigned int i){check(step==3&&g_snd.voice[i].alias);step=4;}
int __cdecl SND_SetPlaybackIdNotPlayed(unsigned int){check(step==0);return -1;}
void __cdecl Com_PrintError(int,const char*,...){}
FACTORY
int main(){
 for(unsigned index=0;index<10;++index)for(int flags:{0,1,0xC000,0xC001})for(int exists:{0,1}){
  memset(&g_snd.voice[index],255,sizeof(snd_voice_t));g_snd.voice[index].alias=nullptr;g_snd.voice[index].soundFileInfo.loadingState=SFLS_UNLOADED;g_snd.time=1000;step=0;
  StreamedSound stream{};SoundFile file{};file.type=2;file.exists=exists;file.u.streamSnd=&stream;snd_alias_t alias{};alias.flags=flags;alias.name="fixture";alias.soundFile=&file;SndStartAliasInfo info{};info.alias=&alias;info.playbackId=123;
  check(SND_StartAliasStream(&info,index)==(exists?123:-1));check(step==(exists?4:0));
 }
 printf("production stream start/order %u checks PASS pointer %zu\n",checks,sizeof(void*));
}
'''.replace('FACTORY',s[i:j]);(a.out/'fixture.cpp').write_text(src,encoding='utf-8');(a.out/'source-manifest.json').write_text(json.dumps({'source':str(a.source),'sha256':hashlib.sha256(a.source.read_bytes()).hexdigest()},indent=2))
