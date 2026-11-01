"""Exercise the actual checkpoint mixer boundary; backend scheduling is synthetic.

The installed APK separately verifies actual FAudio/FAPO parameter publication.
"""
from pathlib import Path
import argparse, hashlib, json

p = argparse.ArgumentParser()
p.add_argument('--out', type=Path, required=True)
a = p.parse_args()
a.out.mkdir(parents=True, exist_ok=False)
root = Path(__file__).resolve().parents[3]
source = root/'src/sound/snd_driver_xaudio2.cpp'
text = source.read_text()
body = text[text.index('bool SD_RefreshCheckpointMix()'):text.index('static bool SD_CanRestoreResidentInfo')]
fixture = r'''
#include <cstdio>
#include <cstring>
#include <cstdlib>
constexpr int S_OK=0;
struct Params { float value; };
using snd_rv_params=Params;
using snd_dsp_master_params=Params;
using snd_dsp_master_no_voice_params=Params;
struct XAUDIO2_VOICE_STATE { unsigned BuffersQueued; };
struct Voice {
 unsigned queued=0; Params actual{},pending{};
 void GetState(XAUDIO2_VOICE_STATE *s){s->BuffersQueued=queued;}
 Voice *NativeVoice(){return this;}
};
struct Engine {
 int starts=0,stops=0,error=0;bool startFailure=false,active=false;
 int Failure(){return error;}
 int StartEngine(){++starts;if(startFailure)return 1;active=true;return S_OK;}
 void StopEngine(){++stops;active=false;}
};
struct State {
 Engine *xa2;Voice *radverbBus,*masterBus,*novoiceBus,*voices[2];
 Params radverbParams,masterParams,novoiceParams;
} g_sd;
int delays=0,applyAfter=3,failAfter=0,preUpdates=0;
void SD_PreUpdate(){
 ++preUpdates;g_sd.radverbBus->pending=g_sd.radverbParams;
 g_sd.masterBus->pending=g_sd.masterParams;g_sd.novoiceBus->pending=g_sd.novoiceParams;
}
void SDL_Delay(int){
 ++delays;if(failAfter&&delays==failAfter)g_sd.xa2->error=1;
 if(g_sd.xa2->active&&delays>=applyAfter)
  for(auto *v:{g_sd.radverbBus,g_sd.masterBus,g_sd.novoiceBus})v->actual=v->pending;
}
int FAudioVoice_GetEffectParameters(Voice *v,int,void *out,unsigned size){memcpy(out,&v->actual,size);return 0;}
'''
fixture = '#include <initializer_list>\n'+fixture+body+r'''
void check(bool ok){if(!ok)abort();}
int main(){
 for(int mode=0;mode<7;++mode){
  Engine engine;Voice r,m,n,s;
  g_sd={&engine,&r,&m,&n,{&s,nullptr},{1},{2},{3}};
  delays=preUpdates=0;applyAfter=3;failAfter=0;
  if(mode==1)s.queued=1;
  if(mode==2)engine.startFailure=true;
  if(mode==3)applyAfter=101;
  if(mode==4)failAfter=2;
  if(mode==5)g_sd.masterBus=nullptr;
  if(mode==6)engine.error=1;
  bool result=SD_RefreshCheckpointMix();
  check(result==(mode==0));check(!engine.active);
  if(mode==0)check(delays==3&&engine.starts==1&&engine.stops==1);
  if(mode==1||mode==5||mode==6)check(!preUpdates&&!engine.starts&&!delays);
  if(mode==2)check(preUpdates==1&&!engine.stops&&!delays);
  if(mode==3)check(delays==100&&engine.stops==1);
  if(mode==4)check(delays==2&&engine.stops==1);
 }
 puts("actual checkpoint mixer boundary 7 scheduling/error/retirement cases PASS");
}
'''
(a.out/'fixture.cpp').write_text(fixture, encoding='utf-8')
(a.out/'source-manifest.json').write_text(json.dumps([{'file':str(source),'sha256':hashlib.sha256(source.read_bytes()).hexdigest()}],indent=2),encoding='utf-8')
