"""Actual saved-dvar stage/retirement helper; isolated typed dvar/lock providers.

Installed APK acceptance separately exercises the canonical dvar implementation.
"""
from pathlib import Path
import argparse,json,hashlib
root=Path(__file__).resolve().parents[3]
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);a=p.parse_args()
source='src/game_sp/g_sp_saved_dvar_checkpoint.inl'
fixture=r'''
#include <universal/dvar.h>
#include <qcommon/common.h>
#include <string>
#include <map>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <stdexcept>
static bool s_continuousZombieResume=true,mainThread=true;
static unsigned checks,writes;
static std::map<std::string,dvar_s> vars;
static std::map<std::string,std::string> strings;
void Com_Error(errorParm_t,const char*,...){throw std::runtime_error("rejected");}
void Com_Printf(int,const char*,...){}
bool Sys_IsMainThread(){return mainThread;}
int I_stricmp(const char*a,const char*b){return strcasecmp(a,b);}
const dvar_s*Dvar_FindVar(const char*n){auto i=vars.find(n);return i==vars.end()?nullptr:&i->second;}
dvar_s*Dvar_FindMalleableVar(const char*n){return const_cast<dvar_s*>(Dvar_FindVar(n));}
DvarValue*Dvar_StringToValue(DvarValue*v,dvarType_t t,DvarLimits,const char*s){
 *v={};if(t==DVAR_TYPE_STRING)v->string=s;else if(t==DVAR_TYPE_BOOL)v->enabled=atoi(s)!=0;
 else if(t==DVAR_TYPE_INT)v->integer=atoi(s);else if(t==DVAR_TYPE_FLOAT)v->value=strtof(s,nullptr);else std::abort();return v;
}
char Dvar_ValueInDomain(dvarType_t t,DvarValue v,DvarLimits d){
 if(t==DVAR_TYPE_INT)return v.integer>=d.integer.min&&v.integer<=d.integer.max;
 if(t==DVAR_TYPE_FLOAT)return std::isfinite(v.value)&&v.value>=d.value.min&&v.value<=d.value.max;
 return true;
}
bool Dvar_ValuesEqual(dvarType_t t,DvarValue a,DvarValue b){
 if(t==DVAR_TYPE_STRING)return !strcmp(a.string,b.string);if(t==DVAR_TYPE_BOOL)return a.enabled==b.enabled;
 if(t==DVAR_TYPE_INT)return a.integer==b.integer;return a.value==b.value;
}
void Dvar_SetVariant(dvar_s*d,DvarValue v,DvarSetSource s){
 if(s!=DVAR_SOURCE_INTERNAL)std::abort();++writes;
 if(d->type==DVAR_TYPE_STRING){strings[d->name]=v.string;d->current.string=strings[d->name].c_str();}else d->current=v;
}
#include <game_sp/g_sp_saved_dvar_checkpoint.inl>
static void check(bool b){if(!b){fprintf(stderr,"failed %u\n",checks);std::abort();}++checks;}
template<class F>static void rejects(F f){bool rejected=false;try{f();}catch(const std::runtime_error&){rejected=true;}check(rejected);}
static dvar_s& make(const std::string&n,dvarType_t type){
 auto &d=vars[n];d={};d.name=vars.find(n)->first.c_str();d.type=type;d.loadedFromSaveGame=true;
 d.saved.integer=77;d.domain.integer.min=-100;d.domain.integer.max=100;return d;
}
int main(){
 auto &hide=make("ammoCounterHide",DVAR_TYPE_BOOL);hide.current.enabled=true;
 auto &count=make("count",DVAR_TYPE_INT);count.current.integer=99;
 auto &text=make("text",DVAR_TYPE_STRING);text.current.string="temporary";text.saved.string="original-backup";
 auto &scale=make("scale",DVAR_TYPE_FLOAT);scale.domain.value.min=0;scale.domain.value.max=10;scale.current.value=9;
 char name[64]="text",value[64]="saved-value";
 SP_StagePublicSavedDvar("ammoCounterHide","0");SP_StagePublicSavedDvar("count","12");
 SP_StagePublicSavedDvar(name,value);SP_StagePublicSavedDvar("scale","1.5");
 memset(name,'x',sizeof(name));memset(value,'y',sizeof(value));
 rejects([]{SP_RestorePublicSavedDvars("early");});check(writes==0);
 s_publicSavedDvarsReady=true;SP_RestorePublicSavedDvars("retirement");
 check(!hide.current.enabled&&count.current.integer==12&&scale.current.value==1.5f&&!strcmp(text.current.string,"saved-value"));
 check(hide.saved.integer==77&&text.loadedFromSaveGame&&!strcmp(text.saved.string,"original-backup"));
 auto previousWrites=writes;SP_RestorePublicSavedDvars("handoff");check(writes==previousWrites);
 // A late fault must reject before changing even the first valid owner.
 for(int fault=0;fault<3;++fault){
  hide.current.enabled=true;auto original=count;auto node=vars.extract("count");
  if(fault!=0){vars.insert(std::move(node));if(fault==1)count.type=DVAR_TYPE_FLOAT;else count.domain.integer.max=5;}
  previousWrites=writes;rejects([]{SP_RestorePublicSavedDvars("bad-owner");});
  check(hide.current.enabled&&writes==previousWrites);
  if(fault==0)vars.insert(std::move(node));count=original;
 }
 mainThread=false;rejects([]{SP_RestorePublicSavedDvars("wrong-thread");});mainThread=true;
 s_continuousZombieResume=false;previousWrites=writes;SP_RestorePublicSavedDvars("ordinary");
 SP_StagePublicSavedDvar("not-registered","1");check(writes==previousWrites&&s_publicSavedDvars.size()==4);
 s_continuousZombieResume=true;rejects([]{SP_StagePublicSavedDvar("ammoCounterHide","1");});
 SP_StagePublicSavedDvar("sv_restoreDvars","1");check(s_publicSavedDvars.size()==4);
 SP_ClearPublicSavedDvars();check(!s_publicSavedDvarsReady&&s_publicSavedDvars.empty());
 rejects([]{SP_RestorePublicSavedDvars("cleared");});
 // An intentionally hidden saved HUD must remain hidden too: no forced zero.
 hide.current.enabled=false;SP_StagePublicSavedDvar("ammoCounterHide","1");s_publicSavedDvarsReady=true;
 SP_RestorePublicSavedDvars("saved-hidden");check(hide.current.enabled&&hide.saved.integer==77);
 SP_ClearPublicSavedDvars();
 // Names/values survive scratch reuse and vector reallocation.
 for(int i=0;i<256;++i){std::string n="text-"+std::to_string(i),v(600,'a'+i%26);auto&d=make(n,DVAR_TYPE_STRING);d.current.string="fresh";d.saved.string="backup";SP_StagePublicSavedDvar(n.c_str(),v.c_str());}
 s_publicSavedDvarsReady=true;SP_RestorePublicSavedDvars("owned-strings");
 for(int i=0;i<256;++i){auto*d=Dvar_FindVar(("text-"+std::to_string(i)).c_str());check(strlen(d->current.string)==600&&d->current.string[0]=='a'+i%26&&!strcmp(d->saved.string,"backup"));}
 SP_ClearPublicSavedDvars();s_publicSavedDvars.resize(4096);rejects([]{SP_StagePublicSavedDvar("count","1");});SP_ClearPublicSavedDvars();
 printf("actual saved dvar checkpoint %u checks PASS, typed stage owns strings and preserves recovery backups\n",checks);
}
'''
a.out.mkdir(parents=True,exist_ok=False)
(a.out/'fixture.cpp').write_text(fixture,encoding='utf-8')
(a.out/'source-manifest.json').write_text(json.dumps({source:hashlib.sha256((root/source).read_bytes()).hexdigest()},indent=2),encoding='utf-8')
