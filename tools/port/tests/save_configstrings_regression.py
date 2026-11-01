"""Actual configstring codec/setter; canonical types, isolated assets/string/IO/network providers."""
from pathlib import Path
import argparse,json,hashlib
root=Path(__file__).resolve().parents[3]
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);a=p.parse_args()
s=(root/'src/server_mp/sv_init_mp.cpp').read_text(encoding='utf-8')
def function(signature):
 start=s.index(signature);b=s.index('{',start);e=b+1;depth=1
 while depth:depth+=(s[e]=='{')-(s[e]=='}');e+=1
 return s[start:e]
actual='\n'.join(function(sig) for sig in ['static void SV_SetConfigstringInternal(', 'void __cdecl SV_SetConfigstring(', 'void SV_RestoreConfigstring('])
fixture=r"""
#include <server_mp/sv_init_mp.h>
#include <server_mp/sv_main_mp.h>
#include <server/server.h>
#include <server/sv_game.h>
#include <game_mp/g_main_mp.h>
#include <game_mp/g_utils_mp.h>
#include <clientscript/cscr_stringlist.h>
#include <qcommon/common.h>
#include <universal/memfile.h>
#include <xanim/xmodel.h>
#include <map>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
server_t sv;serverStatic_t svs;level_locals_t level;
const dvar_t *com_maxclients;
XModel *cached_models[512];
static XModel modelA,modelB;
static unsigned checks,messages;static bool gate=true,badAsset;
static bool continuous,wrongAsset;
static std::vector<std::string> strings={"INVALID"};static std::vector<int> refs={0};
static std::vector<unsigned char> wire;
static void check(bool v){if(!v)std::abort();++checks;}
void Com_Error(errorParm_t,const char*,...){throw std::runtime_error("expected diagnostic error");}
void Com_Printf(int,const char*,...){}
bool Assert_MyHandler(const char*,int,int,const char*,...){std::abort();}
bool Dvar_GetBool(const char*){return gate;}
bool G_SP_CheckpointOption(const char*){return continuous || gate;}
bool G_SP_IsContinuousZombieResume(){return continuous;}
bool SV_Loaded(){return sv.state==SS_GAME;}
void SV_SendServerCommand(client_t*,svscmd_type,const char*,...){++messages;}
void I_strncpyz(char *d,const char*s,int n){if(n<=0)std::abort();std::snprintf(d,n,"%s",s);}
unsigned int SL_GetString_(scriptInstance_t,char *text,unsigned int,int){
 for(unsigned i=1;i<strings.size();++i)if(strings[i]==text){++refs[i];return i;}
 strings.emplace_back(text);refs.push_back(1);return (unsigned)strings.size()-1;
}
unsigned int SL_GetLowercaseString_(const char *s,unsigned int user,int type,scriptInstance_t inst){std::string text=s;for(char &c:text)c=(char)std::tolower((unsigned char)c);return SL_GetString_(inst,text.c_str(),user,type);}
void SL_RemoveRefToString(scriptInstance_t,unsigned int id){check(id>0 && id<refs.size() && refs[id]>0);--refs[id];}
char *SL_ConvertToString(unsigned int id,scriptInstance_t){check(id>0 && id<strings.size() && refs[id]>0);return strings[id].data();}
unsigned int SV_GetConfigstringConst(unsigned int i){check(i<MAX_CONFIGSTRINGS && sv.configstrings[i]);return sv.configstrings[i];}
int I_stricmp(const char *a,const char *b){while(*a && std::tolower((unsigned char)*a)==std::tolower((unsigned char)*b)){++a;++b;}return std::tolower((unsigned char)*a)-std::tolower((unsigned char)*b);}
XModel *SV_XModelGet(char *name){if(badAsset)return nullptr;if(wrongAsset)return &modelB;if(!std::strcmp(name,"fixture_a"))return &modelA;if(!std::strcmp(name,"fixture_b"))return &modelB;return nullptr;}
XModel *G_GetModel(int i){check(i>0 && i<512);return cached_models[i];}
static void flush(MemoryFile *f){wire.insert(wire.end(),f->cacheBuffer,f->cacheBuffer+f->cacheBufferUsed);f->cacheBufferUsed=0;}
void MemFile_WriteDataInternal(MemoryFile *f,unsigned int n,unsigned char*p){flush(f);wire.insert(wire.end(),p,p+n);}
void MemFile_WriteInt(MemoryFile *f,int n){MemFile_WriteData(f,4,(unsigned char*)&n);}
void MemFile_ReadData(MemoryFile*f,int n,unsigned char*p){if(n<0 || (size_t)f->bytesUsed+n>wire.size())Com_Error(ERR_DROP,"bounds");std::memcpy(p,wire.data()+f->bytesUsed,n);f->bytesUsed+=n;}
ACTUAL_SETTER
#include <game_sp/g_sp_save_configstrings.inl>
static void clear(){for(auto &id:sv.configstrings)if(id){SL_RemoveRefToString(SCRIPTINSTANCE_SERVER,id);id=0;}}
static void init(bool reversed){
 clear();for(auto &id:sv.configstrings)id=SL_GetString_(SCRIPTINSTANCE_SERVER,"",0,19);
 gate=true;messages=0;level.initializing=1;sv.state=SS_GAME;sv.restarting=1;
 SV_RestoreConfigstring(CS_MODELS+1,(char*)(reversed?"fixture_b":"fixture_a"));
 SV_RestoreConfigstring(CS_MODELS+2,(char*)(reversed?"fixture_a":"fixture_b"));
 cached_models[1]=reversed?&modelB:&modelA;cached_models[2]=reversed?&modelA:&modelB;
}
static bool loadRejected(){static MemoryFile file;file={};try{G_LoadSaveConfigstrings(&file);}catch(const std::runtime_error&){return true;}return false;}
int main(){
 static client_t client;static dvar_s maxClients;maxClients.current.integer=1;com_maxclients=&maxClients;svs.clients=&client;client.header.state=CS_ACTIVE;
 // Canonical CS_MODELS lowercases, DB assets keep original precache spelling.
 // Shangri-La requests P_ztem_bullseye but saves p_ztem_bullseye.
 modelA.name="Fixture_A";modelB.name="fixture_b";
 static MemoryFile f;init(false);
 // Both methods use real interned ownership/case rules; restore sends no deltas.
 unsigned empty=sv.configstrings[4];int emptyRefs=refs[empty];
 SV_RestoreConfigstring(4,(char*)"Saved Mixed Case");check(refs[empty]==emptyRefs-1 && messages==0);
 check(!std::strcmp(SL_ConvertToString(sv.configstrings[4],SCRIPTINSTANCE_SERVER),"Saved Mixed Case"));
 SV_SetConfigstring(4,(char*)"Normal change");check(messages==1);
 SV_SetConfigstring(4,(char*)"Normal change");check(messages==1);
 SV_RestoreConfigstring(CS_AMBIENT,(char*)"MIXED");check(!std::strcmp(SL_ConvertToString(sv.configstrings[CS_AMBIENT],SCRIPTINSTANCE_SERVER),"mixed"));
 gate=false;bool denied=false;try{SV_RestoreConfigstring(4,(char*)"bad");}catch(const std::runtime_error&){denied=true;}check(denied);gate=true;
 std::string longText(5000,'x');SV_RestoreConfigstring(4,longText.data());check(messages==1);
 wire.clear();f={};G_WriteSaveConfigstrings(&f);flush(&f);const auto captured=wire;
 init(true);SV_RestoreConfigstring(0,(char*)"fresh protocol");SV_RestoreConfigstring(1,(char*)"fresh system");SV_RestoreConfigstring(2,(char*)"fresh build");SV_RestoreConfigstring(3,(char*)"fresh server id");
 const unsigned fresh[]={sv.configstrings[0],sv.configstrings[1],sv.configstrings[2],sv.configstrings[3]};
 f={};G_LoadSaveConfigstrings(&f);check(cached_models[1]==&modelA && cached_models[2]==&modelB);check(messages==0);
 for(int i=0;i<4;++i)check(sv.configstrings[i]==fresh[i]);
 wire.clear();f={};G_WriteSaveConfigstrings(&f);flush(&f);check(wire==captured);
 // Refuse invalid lengths/magic/counts/embedded NUL before replacing anything.
 for(int offset : {0,4,8}){wire=captured;int bad=-1;std::memcpy(wire.data()+offset,&bad,4);check(loadRejected());}
 wire=captured;wire[12]=0;check(loadRejected());
 wire=captured;wire.resize(30);check(loadRejected());
 // Missing assets must not partially publish table/cache replacements.
 wire=captured;init(true);unsigned old=sv.configstrings[CS_MODELS+1];badAsset=true;check(loadRejected());badAsset=false;
 check(sv.configstrings[CS_MODELS+1]==old && cached_models[1]==&modelB && messages==0);
 // Case folding permits only the same identity; unrelated names still fail
 // before publishing any configstring or model cache replacement.
 wrongAsset=true;check(loadRejected());wrongAsset=false;
 check(sv.configstrings[CS_MODELS+1]==old && cached_models[1]==&modelB && messages==0);
 // Normal continuous restore keeps the same setter/asset/reference rules,
 // without enabling the old developer quit gate or emitting stale deltas.
 continuous=true;gate=false;wire=captured;f={};G_LoadSaveConfigstrings(&f);
 check(cached_models[1]==&modelA && cached_models[2]==&modelB && messages==0);
 check(!Dvar_GetBool("bo1_restore_verify_quit"));
 denied=false;try{SV_RestoreConfigstring(MAX_CONFIGSTRINGS,(char*)"bad");}catch(...){denied=true;}
 check(denied);continuous=false;gate=true;
 clear();for(unsigned i=1;i<refs.size();++i)check(refs[i]==0);
 std::printf("save configstrings %u checks PASS\n",checks);
}
"""
a.out.mkdir(parents=True,exist_ok=True);(a.out/'fixture.cpp').write_text(fixture.replace('ACTUAL_SETTER',actual),encoding='utf-8')
(a.out/'source-manifest.json').write_text(json.dumps({f:hashlib.sha256((root/f).read_bytes()).hexdigest() for f in ['src/server_mp/sv_init_mp.cpp','src/server_mp/sv_init_mp.h','src/game_sp/g_sp_save_configstrings.inl']},indent=2)+'\n',encoding='utf-8')
