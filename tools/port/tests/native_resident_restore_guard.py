"""Actual resident restoration preflight with canonical sound types and malformed extents."""
from pathlib import Path
import argparse,hashlib,json
root=Path(__file__).resolve().parents[3]
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);a=p.parse_args()
source=root/'src/sound/snd_driver_xaudio2.cpp';s=source.read_text(encoding='utf-8')
def extract(key):
 start=s.index(key);end=s.index('{',start)+1;depth=1
 while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
 return s[start:end]
actual=extract('static bool SD_CanRestoreResidentInfo(')+'\n'+extract('bool SD_CanRestoreResidentVoice(')
fixture=r'''
#include <sound/snd.h>
#include <sound/snd_driver_api.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <initializer_list>
snd_local_t g_snd;
static int checks;
static void check(bool value){if(!value)std::abort();++checks;}
bool SND_IsVoiceFree(int index){if(index<0 || index>=74)std::abort();return !g_snd.voiceAliasHash[index];}
ACTUAL_GUARD
int main(){
 LoadedSound loaded{};SoundFile file{};file.type=1;file.u.loadSnd=&loaded;file.exists=1;
 snd_alias_t alias{};alias.soundFile=&file;
 auto &voice=g_snd.voice[10];voice.alias=&alias;voice.soundFileInfo.loadingState=SFLS_LOADED;g_snd.voiceAliasHash[10]=1;
 char data[2048]{};
 auto &asset=loaded.sound;asset.data=data;asset.frame_count=1024;asset.frame_rate=48000;asset.channel_count=1;asset.data_size=2048;asset.format=SND_ASSET_FORMAT_PCMS16;
 for(int index=-100;index<=100;++index)check(SD_CanRestoreResidentVoice(index,0)==(index==10));
 for(__int64 cursor:{0LL,1LL,511LL,512LL,1023LL})check(SD_CanRestoreResidentVoice(10,cursor));
 for(__int64 cursor:{-1LL,-100LL,1024LL,1025LL,std::numeric_limits<__int64>::max()})check(!SD_CanRestoreResidentVoice(10,cursor));
 alias.flags=1;for(__int64 cursor:{0LL,1024LL,1025LL,999999999999LL})check(SD_CanRestoreResidentVoice(10,cursor));
 asset.data_size=2050;check(!SD_CanRestoreResidentVoice(10,1025));asset.data_size=2046;check(!SD_CanRestoreResidentVoice(10,0));asset.data_size=2048;
 for(unsigned channels:{0u,3u,0xFFFFFFFFu}){asset.channel_count=channels;check(!SD_CanRestoreResidentVoice(10,0));}asset.channel_count=1;
 asset.format=SND_ASSET_FORMAT_MSADPCM;asset.data_size=524;
 for(__int64 cursor:{0LL,512LL,1024LL,1536LL})check(SD_CanRestoreResidentVoice(10,cursor));
 for(__int64 cursor:{1LL,511LL,513LL,1023LL,1537LL})check(SD_CanRestoreResidentVoice(10,cursor));
 asset.channel_count=2;asset.data_size=1048;check(SD_CanRestoreResidentVoice(10,512));
 asset.data_size=1049;check(!SD_CanRestoreResidentVoice(10,512));asset.data_size=1048;
 asset.frame_count=1023;check(!SD_CanRestoreResidentVoice(10,512));asset.frame_count=1024;
 asset.format=SND_ASSET_FORMAT_WMA;check(SD_CanRestoreResidentVoice(10,123456789));
 for(auto format:{SND_ASSET_FORMAT_PCMS24,SND_ASSET_FORMAT_PCMS32,SND_ASSET_FORMAT_IEEE,SND_ASSET_FORMAT_XMA4,SND_ASSET_FORMAT_MP3}){asset.format=format;check(!SD_CanRestoreResidentVoice(10,0));}
 asset.format=SND_ASSET_FORMAT_WMA;voice.soundFileInfo.loadingState=SFLS_LOADING;check(!SD_CanRestoreResidentVoice(10,0));voice.soundFileInfo.loadingState=SFLS_LOADED;
 file.type=2;check(!SD_CanRestoreResidentVoice(10,0));file.type=1;
 voice.alias=nullptr;check(!SD_CanRestoreResidentVoice(10,0));voice.alias=&alias;
 asset.frame_count=0;check(!SD_CanRestoreResidentVoice(10,0));asset.frame_count=1024;
 asset.frame_rate=0;check(!SD_CanRestoreResidentVoice(10,0));
 for(unsigned rate:{999u,100001u,0xFFFFFFFFu}){asset.frame_rate=rate;check(!SD_CanRestoreResidentVoice(10,0));}asset.frame_rate=48000;
 asset.data=nullptr;check(!SD_CanRestoreResidentVoice(10,0));asset.data=data;
 asset.format=SND_ASSET_FORMAT_MSADPCM;asset.channel_count=2;asset.frame_count=0xFFFFFE00;asset.data_size=(asset.frame_count/512)*524;
 check(!SD_CanRestoreResidentVoice(10,0));
 std::printf("actual resident checkpoint guard %i checks PASS, pointer %zu bytes\n",checks,sizeof(void*));
}
'''
a.out.mkdir(parents=True,exist_ok=False)
(a.out/'fixture.cpp').write_text(fixture.replace('ACTUAL_GUARD',actual),encoding='utf-8')
(a.out/'source-manifest.json').write_text(json.dumps({'file':str(source),'sha256':hashlib.sha256(source.read_bytes()).hexdigest()},indent=2),encoding='utf-8')
