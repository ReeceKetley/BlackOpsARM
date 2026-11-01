"""Canonical glass checkpoint helpers; isolated memfile and map-asset providers."""
from pathlib import Path
import argparse,hashlib,json
root=Path(__file__).resolve().parents[3]
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);args=p.parse_args()
code=r"""
#include <qcommon/common.h>
#include <universal/memfile.h>
#include <Glass/glass_server.h>
#include <Glass/glass_load_obj.h>
#include <vector>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <limits>
static int checks;
#define CHECK(v) do {if(!(v)){fprintf(stderr,"failed line %i check %i\n",__LINE__,checks);abort();}++checks;} while(0)
bool Assert_MyHandler(const char *,int,int,const char *,...){abort();}
void Com_Error(errorParm_t,const char *,...){throw std::runtime_error("rejected");}
void Com_Printf(int,const char *,...){}
void MemFile_ReadData(MemoryFile *f,int n,unsigned char *out){if(n<0||n>f->bufferSize-f->bytesUsed)Com_Error(ERR_DROP,"truncated");memcpy(out,f->buffer+f->bytesUsed,n);f->bytesUsed+=n;}
void MemFile_WriteDataInternal(MemoryFile *,unsigned,unsigned char *){Com_Error(ERR_DROP,"unexpected flush");}
void MemFile_WriteInt(MemoryFile *f,int v){MemFile_WriteData(f,4,(unsigned char *)&v);}
char *MemFile_ReadCString(MemoryFile *f){static char s[256];for(int i=0;i<256;++i){MemFile_ReadData(f,1,(unsigned char *)&s[i]);if(!s[i])return s;}Com_Error(ERR_DROP,"long name");return nullptr;}
GlassesServer svGlasses{};
static Glasses asset{};static Glass panes[3]{};static GlassDef def{};
Glasses *GetGlasses(){return &asset;}
#include <game_sp/g_sp_glass_checkpoint.inl>
static std::vector<unsigned char> encode(){static MemoryFile writer;writer={};SP_WriteGlass(&writer);return {writer.cacheBuffer,writer.cacheBuffer+writer.cacheBufferUsed};}
static bool decode(const std::vector<unsigned char>&bytes){MemoryFile r{};r.buffer=const_cast<unsigned char*>(bytes.data());r.bufferSize=(int)bytes.size();try{SP_ReadGlass(&r);CHECK(r.bytesUsed==r.bufferSize);return true;}catch(...){return false;}}
int main(){
 CHECK(sizeof(void*)==8);CHECK(sizeof(SPGlassState)==40);
 asset.numGlasses=3;asset.glasses=panes;def.name="fixture_glass";def.maxHealth=100;
 svGlasses.numGlasses=3;svGlasses.nextShatterTime=2000;svGlasses.shatterIntervalsMS=77;
 SPGlassState original[3];
 for(int i=0;i<3;++i){panes[i].glassDef=&def;panes[i].index=i;panes[i].brushModel=10+i;panes[i].origin[0]=float(i)*3;
 auto &g=svGlasses.glasses[i];g.glass=&panes[i];g.state.val.i=(i<<4)|i;g.stateChangeTime=500+i;g.health=100-i*55.5f;g.crackedDamageRate=float(i)*.125f;
 for(int k=0;k<3;++k){g.hitPos[k]=float(i*4+k);g.hitDir[k]=float(k)*.25f;}original[i]=SP_GetGlassState(g);CHECK(SP_ValidGlassState(original[i],i));}
 SP_CaptureGlass();auto wire=encode();CHECK(decode(wire));CHECK(encode()==wire);
 for(auto &g:svGlasses.glasses){g.health=999;g.state.val.i=0;}
 svGlasses.nextShatterTime=0;svGlasses.shatterIntervalsMS=0;SP_PublishGlass();
 CHECK(svGlasses.nextShatterTime==2000&&svGlasses.shatterIntervalsMS==77);
 for(int i=0;i<3;++i){auto got=SP_GetGlassState(svGlasses.glasses[i]);CHECK(!memcmp(&got,&original[i],40));CHECK(svGlasses.glasses[i].glass==&panes[i]);}
 for(size_t n=0;n<wire.size();++n){auto cut=wire;cut.resize(n);CHECK(!decode(cut));}
 CHECK(decode(wire));auto bad=wire;bad[0]^=1;CHECK(!decode(bad));bad=wire;int invalid=1000;memcpy(bad.data()+4,&invalid,4);CHECK(!decode(bad));
 bad=wire;invalid=0;memcpy(bad.data()+12,&invalid,4);CHECK(!decode(bad));bad=wire;bad[16]^=1;CHECK(!decode(bad));
 def.name="changed";CHECK(!decode(wire));def.name="fixture_glass";CHECK(decode(wire));
 const size_t valueOffset=16+4+strlen(def.name)+1+12;
 for(float invalidFloat:{std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){bad=wire;memcpy(bad.data()+valueOffset+8,&invalidFloat,4);CHECK(!decode(bad));}
 bad=wire;invalid=3;memcpy(bad.data()+valueOffset,&invalid,4);CHECK(!decode(bad));CHECK(decode(wire));
 // Validate every publication identity before touching the first pane.
 svGlasses.glasses[0].health=333;const Glass *keep=svGlasses.glasses[2].glass;svGlasses.glasses[2].glass=&panes[0];bool rejected=false;try{SP_PublishGlass();}catch(...){rejected=true;}CHECK(rejected);CHECK(svGlasses.glasses[0].health==333);svGlasses.glasses[2].glass=const_cast<Glass*>(keep);SP_PublishGlass();CHECK(svGlasses.glasses[0].health==100);
 s_savedGlassPresent=false;svGlasses.glasses[0].health=222;SP_PublishGlass();CHECK(svGlasses.glasses[0].health==222);
 asset.numGlasses=0;svGlasses.numGlasses=0;SP_CaptureGlass();wire=encode();CHECK(wire.size()==16);CHECK(decode(wire));SP_PublishGlass();
 printf("production glass checkpoint %i checks PASS; pane record %zu bytes; pointer %zu bytes\n",checks,sizeof(SPGlassState),sizeof(void*));
}
"""
args.out.mkdir(parents=True,exist_ok=False)
(args.out/'fixture.cpp').write_text(code,encoding='utf-8')
paths=['src/game_sp/g_sp_glass_checkpoint.inl','src/Glass/glass.h','src/Glass/glass_server.h']
(args.out/'source-manifest.json').write_text(json.dumps({x:hashlib.sha256((root/x).read_bytes()).hexdigest() for x in paths},indent=2),encoding='utf-8')
