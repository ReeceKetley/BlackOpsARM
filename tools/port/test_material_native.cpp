#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <vector>
#include <string>
#include <algorithm>
using _BYTE=unsigned char;using _WORD=unsigned short;using _QWORD=unsigned long long;
#define LODWORD(x) (*reinterpret_cast<unsigned*>(&x))
struct IDirect3DVertexDeclaration9;struct IDirect3DVertexShader9;struct IDirect3DPixelShader9;
struct MaterialTextureDef;struct MaterialConstantDef;
// PRODUCTION_TYPES
// PRODUCTION_DECLARATIONS
static void require(bool ok,const char *why) {if(!ok){fprintf(stderr,"FAIL: %s\n",why);exit(1);}}
bool Assert_MyHandler(const char*,int,int,const char*,...) {require(false,"production assertion");return true;}
enum {ERR_DROP,ERR_FATAL};
void Com_Error(int,const char*,...) {require(false,"production fatal error");}
unsigned scriptErrors;
void Com_ScriptError(const char*,...) {++scriptErrors;}
void Com_PrintError(int,const char*,...) {}
void Com_PrintWarning(int,const char*,...) {}
void Com_Printf(int,const char*,...) {}
int Com_sprintf(char *dest,unsigned size,const char *format,...) {va_list ap;va_start(ap,format);int n=vsnprintf(dest,size,format,ap);va_end(ap);return n;}
void I_strncpyz(char *dest,const char *src,int size) {strncpy_s(dest,size,src,_TRUNCATE);}
void Com_BeginParseSession(const char*) {}
void Com_EndParseSession() {}
void Com_SetScriptWarningPrefix(const char*) {}
void Com_SetSpaceDelimited(int) {}
void Com_SetKeepStringQuotes(int) {}
struct Token {char token[1024];};
Token *Com_Parse(const char **text) {
    static Token result;auto *p=*text;while(*p && static_cast<unsigned char>(*p)<=32)++p;unsigned n=0;
    if(*p=='"') {result.token[n++]=*p++;while(*p && *p!='"')result.token[n++]=*p++;if(*p)result.token[n++]=*p++;}
    else if(strchr("{}:;",*p) && *p)result.token[n++]=*p++;
    else while(*p && static_cast<unsigned char>(*p)>32 && !strchr("{}:;",*p))result.token[n++]=*p++;
    result.token[n]=0;*text=p;return &result;
}
bool Material_MatchToken(const char **text,const char *expected) {return strcmp(Com_Parse(text)->token,expected)==0;}
std::string fileText;
int FS_ReadFile(const char*,void **file) {*file=fileText.data();return int(fileText.size());}
void FS_FreeFile(void*) {}
bool gCheckedForPimp=true,gIsPIMPEnabled=false,gIsSW2Material=false;
struct Allocation {unsigned char *pointer;size_t size;};
std::vector<Allocation> allocations;
unsigned char *Material_Alloc(unsigned size) {
    static uintptr_t next=sizeof(void*)==8?uintptr_t(0x180000000ull):0;
    size_t aligned=(size+15)&~size_t(15),pages=(aligned+4095)&~size_t(4095);
    auto *base=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>(next),pages+8192,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    require(base!=nullptr,"guarded allocation");if(sizeof(void*)==8){require(uintptr_t(base)>0x100000000ull,"native objects above4GB");next+=0x100000;}
    DWORD old;require(VirtualProtect(base,4096,PAGE_NOACCESS,&old)!=0,"front guard");require(VirtualProtect(base+4096+pages,4096,PAGE_NOACCESS,&old)!=0,"end guard");
    auto *p=base+4096+pages-aligned;memset(p,0,aligned);memset(p+size,0xa5,aligned-size);allocations.push_back({p,size});return p;
}
static void checkGuards() {for(auto a:allocations)for(size_t i=a.size;i<((a.size+15)&~size_t(15));++i)require(a.pointer[i]==0xa5,"allocation end canary");}
template<class T> T *object() {return reinterpret_cast<T*>(Material_Alloc(sizeof(T)));}
float *literal;MaterialVertexDeclaration *declaration;MaterialVertexShader *vertexShader;MaterialPixelShader *pixelShader;
MaterialStateMap *stateMap;MaterialTechnique *registeredTechnique;
std::vector<MaterialShaderArgument> passInputs;
bool Material_IgnoreTechnique(const char*) {return false;}
unsigned Material_TechniqueTypeForName(const char *token) {return unsigned(atoi(token+1));}
bool Material_UsingTechnique(unsigned) {return true;}
MaterialTechnique *Material_RegisterTechnique(char*) {return registeredTechnique;}
bool Material_ParseStateMap(char **,MaterialStateMap *map) {for(auto &p:map->ruleSet)p=stateMap->ruleSet[0];return true;}
bool Material_LoadPassStateMap(const char**,MaterialStateMap **map) {*map=stateMap;return true;}
bool Material_LoadPassVertexShader(const char**,unsigned short*,ShaderParameterSet *params,MaterialPass *pass,unsigned,unsigned *count,MaterialShaderArgument *args) {
    memset(params,0,sizeof(*params));pass->vertexShader=vertexShader;*count=unsigned(passInputs.size());std::copy(passInputs.begin(),passInputs.end(),args);return true;
}
bool Material_LoadPassPixelShader(const char**,unsigned short*,ShaderParameterSet *params,MaterialPass *pass,unsigned,unsigned*,MaterialShaderArgument*) {
    memset(params,0,sizeof(*params));pass->pixelShader=pixelShader;return true;
}
bool Material_ValidateShaderLinkage(const ShaderVaryingDef*,unsigned,const ShaderVaryingDef*,unsigned) {return true;}
char Material_LoadPassVertexDecl(const char**,const ShaderVaryingDef*,unsigned,MaterialPass *pass) {pass->vertexDecl=declaration;return 1;}
char Material_LoadDeclTypes(const char**,MaterialPass*) {return 1;}
unsigned Material_ElemCountForParamName(const char*,const ShaderUniformDef*,unsigned,const char*,ShaderParamType*) {require(false,"unused explicit-argument parse");return 0;}
bool Material_ParseIndexRange(const char**,unsigned,ShaderIndexRange*) {require(false,"unused explicit range");return false;}
bool Material_ParseArgumentSource(MaterialShaderType,const char**,const char*,ShaderParamType,ShaderArgumentSource*) {require(false,"unused explicit source");return false;}
void Com_SkipRestOfLine(const char**) {require(false,"unused explicit skip");}
char CodeConstIsOneOf(unsigned short,const unsigned short*,int) {require(false,"unused code const default");return 0;}
bool Material_DefaultArgumentSource(MaterialShaderType,const char*,ShaderParamType,const ShaderIndexRange*,ShaderArgumentSource *source) {source->type=2;return true;}
bool Material_AddShaderArgument(const char*,MaterialShaderType,ShaderArgumentSource*,const ShaderArgumentDest *dest,ShaderUniformDef *params,unsigned count,unsigned *used,MaterialShaderArgument *args,char (*)[64]) {
    require(*used<count,"scratch capacity");params[dest->indexRange.first].isAssigned=true;
    auto &a=args[(*used)++];a.type=2;a.dest=static_cast<unsigned short>(dest->indexRange.first);a.u.nameHash=0x12340000+dest->indexRange.first;return true;
}
unsigned stateFlagUpdates,techniqueFlagUpdates;
void Material_UpdateStateFlags(Material*) {++stateFlagUpdates;}
void Material_UpdateTechniqueFlags(Material*) {++techniqueFlagUpdates;}
// PRODUCTION_FUNCTIONS
static void initialize() {
    literal=reinterpret_cast<float*>(Material_Alloc(16));for(unsigned i=0;i<4;++i)literal[i]=i+.25f;
    declaration=object<MaterialVertexDeclaration>();declaration->hasOptionalSource=true;
    vertexShader=object<MaterialVertexShader>();pixelShader=object<MaterialPixelShader>();stateMap=object<MaterialStateMap>();
    auto *rule=object<MaterialStateMapRuleSet>();rule->ruleCount=1;
    for(auto &p:stateMap->ruleSet)p=rule;
    unsigned prim=0,obj=0;while(s_codeConstUpdateFreq[prim]!=MTL_UPDATE_PER_PRIM)++prim;while(s_codeSamplerUpdateFreq[obj]!=MTL_UPDATE_PER_OBJECT)++obj;
    MaterialShaderArgument a{};a.type=1;a.dest=10;a.u.literalConst=literal;passInputs.push_back(a);
    a={};a.type=3;a.dest=0;a.u.codeConst={static_cast<unsigned short>(prim),0,1};passInputs.push_back(a);
    a={};a.type=4;a.dest=15;a.u.codeSampler=27;passInputs.push_back(a);
    a={};a.type=4;a.dest=1;a.u.codeSampler=obj;passInputs.push_back(a);
    a={};a.type=0;a.dest=40;a.u.nameHash=123;passInputs.push_back(a);
}
static void checkPass(const MaterialPass &pass) {
    require(pass.vertexDecl==declaration && pass.vertexShader==vertexShader && pass.pixelShader==pixelShader,"full native pass pointers");
    require(pass.perPrimArgCount==1 && pass.perObjArgCount==1 && pass.stableArgCount==2 && pass.customSamplerFlags==1,"production argument frequencies/custom filtering");
    require(pass.localArgs[0].type==3 && pass.localArgs[1].type==4 && pass.localArgs[2].type==0,"sorted native argument slots");
    require(pass.localArgs[3].type==1 && pass.localArgs[3].dest==10 && pass.localArgs[3].u.literalConst==literal,"copied literal retains full pointer");
    require(pass.localArgs[3].u.literalConst[3]==3.25f,"copied literal remains readable");
}
static void techniqueCase() {
    for(unsigned n=1;n<=4;++n) {
        fileText.clear();for(unsigned i=0;i<n;++i)fileText+="{ } ";char name[]="zprepass";
        auto *tech=Material_LoadTechnique(name);require(tech && tech->passCount==n && tech->flags==12,"technique flags/pass count");
        require(strcmp(tech->name,name)==0,"owned technique name");
        auto *maps=reinterpret_cast<MaterialStateMap**>(tech->passArray+n);
        for(unsigned i=0;i<n;++i){checkPass(tech->passArray[i]);require(maps[i]==stateMap,"state map follows native passes");}
        require(tech->name==reinterpret_cast<const char*>(maps+n),"name follows native pointer array");registeredTechnique=tech;
    }
    fileText="";char name[]="empty";require(Material_LoadTechnique(name)==nullptr,"no-pass technique rejected");
}
static void techsetCase() {
    registeredTechnique=object<MaterialTechnique>();fileText="\"0\" : test ; \"129\" : test ;";char name[]="native_set";
    auto *set=Material_LoadTechniqueSet(name);require(set && !strcmp(set->name,name),"native technique-set name");
    require(set->techniques[0]==registeredTechnique && set->techniques[129]==registeredTechnique && set->techniques[128]==nullptr,"first/last native technique slots");
}
static void statemapCase() {
    char name[]="native_map";fileText="";auto *map=Material_LoadStateMap(name);require(map && !strcmp(map->name,name),"native state-map name");
    for(auto p:map->ruleSet)require(p==stateMap->ruleSet[0],"all ten full rule-set pointers");
}
static void literalCase() {
    auto *usage=reinterpret_cast<char(*)[64]>(Material_Alloc(256*64));auto *arg=object<MaterialShaderArgument>();ShaderUniformDef dest{};dest.resourceDest=9;
    require(Material_AddShaderArgumentFromLiteral("vs","literal",1,literal,&dest,arg,usage)==1 && arg->u.literalConst==literal,"literal setter keeps native pointer");
    require(arg->u.literalConst[2]==2.25f,"literal payload readable");
    require(Material_AddShaderArgumentFromLiteral("vs","duplicate",1,literal,&dest,arg,usage)==0,"register collision rejected");
    dest.resourceDest=255;require(Material_AddShaderArgumentFromLiteral("ps","literal",7,literal,&dest,arg,usage)==1 && arg->u.literalConst==literal,"pixel literal pointer");
}
static void combineCase() {
    auto *args=reinterpret_cast<MaterialShaderArgument*>(Material_Alloc(6*sizeof(MaterialShaderArgument)));
    args[0].type=args[1].type=3;args[0].dest=0;args[1].dest=1;args[0].u.codeConst={197,0,1};args[1].u.codeConst={197,1,1};
    args[2].type=1;args[2].dest=10;args[2].u.literalConst=literal;args[3].type=7;args[3].dest=20;args[3].u.literalConst=literal+2;
    require(Material_CombineShaderArguments(4,args)==3,"matrix rows merge");
    require(args[0].u.codeConst.rowCount==2 && args[1].u.literalConst==literal && args[2].u.literalConst==literal+2,"compaction preserves following pointers");
    auto *output=reinterpret_cast<MaterialShaderArgument*>(Material_Alloc(8*sizeof(MaterialShaderArgument)));unsigned count=2;output[0].dest=234;output[1].dest=235;
    require(Material_SetShaderArguments(3,args,8,&count,output)==1 && count==5,"argument append/count");
    require(output[0].dest==234 && output[1].dest==235,"existing prefix preserved");bool found=false;
    for(unsigned i=2;i<count;++i)if(output[i].type==7){require(output[i].u.literalConst==literal+2,"appended pointer");found=true;}
    require(found,"appended pixel literal");unsigned before=count;require(Material_SetShaderArguments(3,args,5,&count,output)==0 && count==before,"argument limit rejects without count change");
}
static void parseCase() {
    auto *params=reinterpret_cast<ShaderUniformDef*>(Material_Alloc(512*sizeof(ShaderUniformDef)));auto *args=reinterpret_cast<MaterialShaderArgument*>(Material_Alloc(512*sizeof(MaterialShaderArgument)));
    for(unsigned i=0;i<512;++i){params[i].name="sampler";params[i].index=static_cast<unsigned short>(i);params[i].type=SHADER_PARAM_SAMPLER_2D;}
    char textBuffer[]="{ }";char *text=textBuffer;unsigned count=0;unsigned short flags=0;char shader[]="parser_capacity";
    require(Material_ParseShaderArguments(&text,shader,MTL_VERTEX_SHADER,params,512,&flags,512,&count,args)==1 && count==512,"production parser full scratch/output capacity");
    for(unsigned i=0;i<512;++i)require(args[i].type==2 && args[i].dest==i && args[i].u.nameHash==0x12340000+i,"complete parsed native argument slots");
}
static MaterialTechnique *stateTechnique(unsigned first) {
    constexpr unsigned n=4;auto *tech=reinterpret_cast<MaterialTechnique*>(Material_Alloc(offsetof(MaterialTechnique,passArray)+n*sizeof(MaterialPass)+n*sizeof(MaterialStateMap*)));
    tech->passCount=n;auto **maps=reinterpret_cast<MaterialStateMap**>(tech->passArray+n);
    for(unsigned i=0;i<n;++i) {
        auto *map=object<MaterialStateMap>();map->name="state";
        for(unsigned j=0;j<10;++j){auto *rule=object<MaterialStateMapRuleSet>();rule->ruleCount=1;rule->rules[0].stateBitsClear[0]=rule->rules[0].stateBitsClear[1]=~0u;map->ruleSet[j]=rule;}
        map->ruleSet[0]->rules[0].stateBitsClear[0]=0;map->ruleSet[0]->rules[0].stateBitsSet[0]=first+i;
        maps[i]=map;
    }
    return tech;
}
static void stateCase() {
    auto *material=object<Material>();material->info.name="state_material";material->localTechniqueSet=object<MaterialTechniqueSet>();
    auto *a=stateTechnique(1),*b=stateTechnique(3);for(unsigned i=0;i<130;++i)material->localTechniqueSet->techniques[i]=i%2?b:a;
    material->localTechniqueSet->techniques[129]=nullptr;unsigned ref[2]={~0u,0x10};
    Material_BuildStateBitsTable(material,0,ref);require(material->stateBitsCount==6,"overlapping four-pass state sequences deduplicate");
    const unsigned expected[6]={1,2,3,4,5,6};
    for(unsigned i=0;i<6;++i)require(material->stateBitsTable[i].loadBits[0]==expected[i] && material->stateBitsTable[i].loadBits[1]==0,"actual remap rules/table output");
    for(unsigned i=0;i<129;++i)require(material->stateBitsEntry[i]==(i%2?2:0),"every technique selects correct state sequence");
    require(material->stateBitsEntry[129]==255 && stateFlagUpdates==1 && techniqueFlagUpdates==1,"absent technique sentinel and flag updates");
}
int main(int argc,char **argv) {
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);require(argc==2,"case argument");initialize();
    std::string c=argv[1];
    if(c=="technique")techniqueCase();else if(c=="techset")techsetCase();else if(c=="statemap")statemapCase();
    else if(c=="literal")literalCase();else if(c=="combine")combineCase();else if(c=="parse")parseCase();else if(c=="state")stateCase();else require(false,"known case");
    checkGuards();printf("PASS %s: production native layout/behavior, pointer width%zu\n",argv[1],sizeof(void*));
}
