"""Canonical HUD checkpoint and Show/Hide/refresh, isolated menu/file providers."""
from pathlib import Path
import argparse,hashlib,json
root=Path(__file__).resolve().parents[3]
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);a=p.parse_args()
def extract(path,key):
 s=(root/path).read_text();start=s.index(key);end=s.index('{',start)+1;depth=1
 while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
 return s[start:end]
source='src/game_sp/g_sp_client_hud_checkpoint.inl'
code=r"""
#include <cgame_mp/cg_main_mp.h>
#include <universal/memfile.h>
#include <ui/ui_utils.h>
#include <client_mp/cl_scrn_mp.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <stdexcept>
#include <limits>
#include <type_traits>
static int checks;
#define CHECK(x) do{if(!(x)){fprintf(stderr,"check %i failed line %i\n",checks,__LINE__);abort();}++checks;}while(0)
bool Assert_MyHandler(const char *,int,int,const char *,...){throw std::runtime_error("assert");}
void Com_Error(errorParm_t,const char *,...){throw std::runtime_error("rejected");}
void Com_Printf(int,const char *,...){}
void MemFile_ReadData(MemoryFile *f,int n,unsigned char *out){if(n<0||n>f->bufferSize-f->bytesUsed)Com_Error(ERR_DROP,"truncated");memcpy(out,f->buffer+f->bytesUsed,n);f->bytesUsed+=n;}
void MemFile_WriteDataInternal(MemoryFile *,unsigned,unsigned char *){Com_Error(ERR_DROP,"flush");}
char *MemFile_ReadCString(MemoryFile *f){static char s[256];for(int i=0;i<256;++i){MemFile_ReadData(f,1,(unsigned char *)&s[i]);if(!s[i])return s;}Com_Error(ERR_DROP,"long");return nullptr;}
static cg_s fixtureCg;
cg_s *cgArray=&fixtureCg;
UiContext cgDC[1];
menuDef_t *Menus_FindByName(const UiContext *dc,const char *name){for(int i=0;i<dc->menuCount;++i)if(!strcmp(dc->Menus[i]->window.name,name))return dc->Menus[i];return nullptr;}
int Window_GetDynamicFlags(int i,const windowDef_t *w){CHECK(i==0);return w->dynamicFlags[i];}
bool Dvar_GetBool(const char *name){CHECK(!strcmp(name,"zombiemode"));return true;}
CANONICAL_UI
CODEC;
PRODUCTION
CANONICAL_REFRESH
static std::vector<unsigned char> encode(SPClientHudState value){MemoryFile w{};SPNativeCheckpointCodec c{nullptr,&w};SP_ClientHudCodec(c,value);return {w.cacheBuffer,w.cacheBuffer+w.cacheBufferUsed};}
static bool decode(const std::vector<unsigned char> &bytes){MemoryFile f{},w{};f.buffer=const_cast<unsigned char*>(bytes.data());f.bufferSize=bytes.size();SPNativeCheckpointCodec c{&f,&w};try{SP_DecodeClientHud(c);CHECK(f.bytesUsed==f.bufferSize);return true;}catch(...){return false;}}
static void same(const SPClientHudState &a,const SPClientHudState &b){CHECK(encode(a)==encode(b));}
int main(){
 CHECK(sizeof(void*)==8);fixtureCg.time=fixtureCg.physicsTime=27758;
 fixtureCg.ammoFadeTime=27758;fixtureCg.healthFadeTime=27000;fixtureCg.compassFadeTime=26000;fixtureCg.stanceFadeTime=25000;
 fixtureCg.sprintFadeTime=0;fixtureCg.offhandFadeTime=-1;fixtureCg.offhandFlashTime=std::numeric_limits<int>::min();fixtureCg.waypointFadeTime=std::numeric_limits<int>::max();fixtureCg.vehicleControlsFadeTime=50000;fixtureCg.scoreFadeTime=123;
 static menuDef_t menus[3]{};const char *names[]={"Health","weaponinfozombies","objectiveinfo"};
 for(int i=0;i<3;++i){menus[i].window.name=names[i];menus[i].window.dynamicFlags[0]=0x10000|(i==1?4:0);cgDC[0].Menus[i]=&menus[i];}cgDC[0].menuCount=3;
 SPClientHudState saved;SP_ReadClientHud(saved);auto wire=encode(saved);CHECK(decode(wire));same(s_clientHud,saved);
 // Reproduce a later fresh-map clock, and prove legacy does not invent history.
 fixtureCg.ammoFadeTime=45150;s_clientHudPresent=false;SP_PublishClientHud();CHECK(fixtureCg.ammoFadeTime==45150);
 CHECK(decode(wire));fixtureCg.healthFadeTime=99999;menus[0].window.dynamicFlags[0]|=4;menus[1].window.dynamicFlags[0]&=~4;
 SP_PublishClientHud();SPClientHudState actual;SP_ReadClientHud(actual);same(actual,saved);CHECK(fixtureCg.time==27758&&fixtureCg.physicsTime==27758);
 CHECK(fixtureCg.ammoFadeTime==27758&&fixtureCg.vehicleControlsFadeTime==50000);CHECK(menus[0].window.dynamicFlags[0]==0x10000);CHECK(menus[1].window.dynamicFlags[0]==0x10004);
 SP_PublishClientHud();SP_ReadClientHud(actual);same(actual,saved);
 // Canonical paused HUD refresh still owns subsequent ordinary transitions.
 CG_SP_RefreshAllHudFades(0);CHECK(fixtureCg.ammoFadeTime==27758&&fixtureCg.healthFadeTime==27758&&fixtureCg.compassFadeTime==27758);CHECK(fixtureCg.vehicleControlsFadeTime==50000);
 CHECK((menus[0].window.dynamicFlags[0]&4)!=0);
 for(size_t n=0;n<wire.size();++n){auto cut=wire;cut.resize(n);CHECK(!decode(cut));}
 auto bad=wire;bad[0]^=1;CHECK(!decode(bad));bad=wire;int badCount=600;memcpy(bad.data()+44,&badCount,4);CHECK(!decode(bad));
 // Each boolean on disk must be canonical, and names must retain identity/order.
 size_t cursor=48;
 for(unsigned i=0;i<ARRAY_COUNT(s_hudGroupNames);++i){const auto n=strlen(s_hudGroupNames[i]);bad=wire;bad[cursor]^=1;CHECK(!decode(bad));cursor+=n+1;
  for(int field=0;field<2;++field){bad=wire;bad[cursor+field]=2;CHECK(!decode(bad));}cursor+=2;}
 CHECK(decode(wire));SP_PublishClientHud();fixtureCg.ammoFadeTime=88888;
 // All definition identities must validate before the first clock/menu mutation.
 cgDC[0].menuCount=2;bool failed=false;try{SP_PublishClientHud();}catch(...){failed=true;}CHECK(failed&&fixtureCg.ammoFadeTime==88888);cgDC[0].menuCount=3;
 cgDC[0].contextIndex=1;failed=false;try{SP_PublishClientHud();}catch(...){failed=true;}CHECK(failed&&fixtureCg.ammoFadeTime==88888);cgDC[0].contextIndex=0;
 auto invalid=saved;invalid.groups[1].present=false;invalid.groups[1].visible=true;bool invalidRejected=false;try{encode(invalid);}catch(...){invalidRejected=true;}CHECK(invalidRejected);
 CHECK(decode(wire));SP_PublishClientHud();SP_ReadClientHud(actual);same(actual,saved);
 printf("production HUF1 HUD %i checks PASS; pointer %zu; deadlines preserved with canonical Show/Hide/paused refresh\n",checks,sizeof(void*));
}
"""
ui='\n'.join(extract('src/ui/ui_utils.cpp',k) for k in ['void __cdecl Window_SetDynamicFlags(','void __cdecl Window_AddDynamicFlags(','void __cdecl Window_RemoveDynamicFlags('])
ui+='\n'+'\n'.join(extract('src/ui/ui_shared.cpp',k) for k in ['void __cdecl Menus_HideByName(','void __cdecl Menus_ShowByName('])
code=code.replace('CANONICAL_UI',ui).replace('CODEC',extract('src/game_sp/g_sp_native_owners_checkpoint.inl','struct SPNativeCheckpointCodec')).replace('PRODUCTION',(root/source).read_text()).replace('CANONICAL_REFRESH',extract('src/cgame/cg_sp_hud.cpp','void CG_SP_RefreshAllHudFades('))
a.out.mkdir(parents=True,exist_ok=False);(a.out/'fixture.cpp').write_text(code,encoding='utf-8')
paths=[source,'src/game_sp/g_sp_client_checkpoint.inl','src/game_sp/g_sp_client_models_checkpoint.inl','src/game_sp/g_sp_native_owners_checkpoint.inl','src/cgame/cg_sp_hud.cpp','src/ui/ui_shared.cpp','src/ui/ui_utils.cpp','src/cgame_mp/cg_local_mp.h']
(a.out/'source-manifest.json').write_text(json.dumps({x:hashlib.sha256((root/x).read_bytes()).hexdigest() for x in paths},indent=2),encoding='utf-8')
