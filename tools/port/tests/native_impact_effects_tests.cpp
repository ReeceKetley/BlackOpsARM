// Actual canonical raw impact loader/tokenizer/surface names. Isolated real
// directory/file storage, tracked temporary/permanent allocations, and typed
// immutable test FX assets expose native pointer width and lifetime boundaries.
#include <cgame/cg_effects_load_obj.cpp>
#include <array>
#include <vector>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <pthread.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

static unsigned checks,opens,closes,registrations;
static bool shortRead,brokenList;
static FILE *file;
static const pthread_t owner=pthread_self();
static void check(bool ok,const char *name){if(!ok){fprintf(stderr,"FAIL %s\n",name);exit(1);}++checks;printf("PASS %s\n",name);}
bool Sys_IsMainThread(){return pthread_equal(owner,pthread_self());}
bool Sys_IsRenderThread(){return false;}
bool Sys_IsServerThread(){return false;}
bool Sys_IsDatabaseThread(){return false;}
bool Assert_MyHandler(const char *,int,int,const char *fmt,...){va_list a;va_start(a,fmt);vfprintf(stderr,fmt,a);va_end(a);abort();}
void Com_Error(errorParm_t,const char *fmt,...){va_list a;va_start(a,fmt);vfprintf(stderr,fmt,a);va_end(a);abort();}
void Com_Printf(int,const char *fmt,...){va_list a;va_start(a,fmt);vfprintf(stderr,fmt,a);va_end(a);}
void Com_PrintError(int,const char *fmt,...){va_list a;va_start(a,fmt);vfprintf(stderr,fmt,a);va_end(a);}
void Com_PrintWarning(int,const char *fmt,...){va_list a;va_start(a,fmt);vfprintf(stderr,fmt,a);va_end(a);}
int I_stricmp(const char *a,const char *b){return strcasecmp(a,b);}
int Com_sprintf(char *out,unsigned size,const char *fmt,...){va_list a;va_start(a,fmt);int n=vsnprintf(out,size,fmt,a);va_end(a);return n;}
char *va(const char *fmt,...){static thread_local char text[4096];va_list a;va_start(a,fmt);vsnprintf(text,sizeof(text),fmt,a);va_end(a);return text;}

struct Allocation{unsigned char *base,*payload;unsigned size;};
static std::vector<Allocation> permanent,low,high;
static unsigned char *allocate(unsigned size,unsigned alignment,std::vector<Allocation> &arena){
    void *base=nullptr;check(posix_memalign(&base,64,size+128)==0,"allocate aligned actual fixture storage");
    auto *bytes=static_cast<unsigned char *>(base);memset(bytes,0xa5,size+128);auto *payload=bytes+64;
    check(reinterpret_cast<uintptr_t>(payload)%alignment==0,"requested allocation alignment honored");
    arena.push_back({bytes,payload,size});return payload;
}
static bool guards(const Allocation &a){for(unsigned i=0;i<64;++i)if(a.base[i]!=0xa5||a.payload[a.size+i]!=0xa5)return false;return true;}
static void retire(std::vector<Allocation> &arena){for(auto a:arena){check(guards(a),"allocation surrounding canaries preserved");free(a.base);}arena.clear();}
unsigned char *Hunk_AllocAlign(unsigned size,int alignment,const char *,int){return allocate(size,alignment,permanent);}
unsigned *Hunk_AllocateTempMemory(int size,const char *){return reinterpret_cast<unsigned *>(allocate(size,4,low));}
void *Hunk_AllocateTempMemoryHigh(int size,const char *){return allocate(size,4,high);}
char Hunk_CheckTempMemoryClear(){check(low.empty(),"low temporary owner initially clear");return 1;}
char Hunk_CheckTempMemoryHighClear(){check(high.empty(),"high temporary owner initially clear");return 1;}
void Hunk_ClearTempMemory(){retire(low);}
void Hunk_ClearTempMemoryHigh(){retire(high);}
int FS_GetFileList(const char *path,char *,FsListBehavior_e,char *list,int capacity){
    DIR *dir=opendir(path);if(!dir)return 0;unsigned used=0,count=0;
    while(auto *entry=readdir(dir)){const size_t n=strlen(entry->d_name);if(n<4||strcmp(entry->d_name+n-4,".csv"))continue;check(used+n+1<=static_cast<unsigned>(capacity),"real directory names fit list");memcpy(list+used,entry->d_name,n+1);used+=n+1;++count;}
    closedir(dir);if(brokenList){memset(list,'x',capacity);return 1;}return count;
}
unsigned FS_FOpenFileByMode(char *path,int *handle,fsMode_t mode){check(mode==FS_READ&&!file,"open real CSV with sole owned handle");file=fopen(path,"rb+");++opens;if(!file){*handle=0;return static_cast<unsigned>(-1);}fseek(file,0,SEEK_END);long n=ftell(file);rewind(file);if(shortRead){check(ftruncate(fileno(file),n-1)==0,"shorten actual file after length measurement");shortRead=false;}*handle=1;return n;}
unsigned FS_Read(unsigned char *out,unsigned n,int handle){check(file&&handle==1,"read actual owned CSV");return fread(out,1,n,file);}
void FS_FCloseFile(int handle){check(file&&handle==1,"close actual CSV handle");fclose(file);file=nullptr;++closes;}
static FxEffectDef assets[22]{};
static char assetNames[22][32];
const FxEffectDef *FX_Register(char *name){for(unsigned i=0;i<22;++i)if(!strcmp(name,assetNames[i])){++registrations;return &assets[i];}fprintf(stderr,"Unknown fixture FX asset %s\n",name);abort();}
static void writeFiles(){
    FILE *out=fopen("fx/a.csv","wb");check(out!=nullptr,"create real impact CSV");
    for(unsigned type=0;type<21;++type)for(unsigned surface=0;surface<31;++surface)fprintf(out,"%s,%s,%s\n",g_TypeName[type],Com_SurfaceTypeToName(surface),assetNames[type]);fclose(out);
    out=fopen("fx/z.csv","wb");check(out!=nullptr,"create real sorted override CSV");fprintf(out,"%s,flesh,%s\n",g_TypeName[0],assetNames[21]);fclose(out);
}
int main(){
    check(mkdir("fx",0700)==0,"create isolated fixture directory");Com_InitParse();
    for(unsigned i=0;i<22;++i){snprintf(assetNames[i],sizeof(assetNames[i]),"fixture_impact_%u",i);assets[i].name=assetNames[i];}
    writeFiles();FxImpactTable *table=CG_RegisterImpactEffects_LoadObj(nullptr);
    check(table&&permanent.size()==2&&permanent[0].size==sizeof(FxImpactTable)&&permanent[1].size==21*sizeof(FxImpactEntry),"actual loader allocates complete native header and pointer rows");
    check(table->name&&!strcmp(table->name,"")&&low.empty()&&high.empty()&&opens==closes&&!file,"published table survives retired file and temporary owners");
    bool all=true;for(unsigned i=0;i<21;++i){for(unsigned s=0;s<31;++s)all&=table->table[i].nonflesh[s]==&assets[(i==0&&s==7)?21:i];for(unsigned f=0;f<4;++f)all&=table->table[i].flesh[f]==&assets[i?i:21];}
    check(all&&registrations==21*35,"all native effect pointers and original flesh fallback preserve sorted override");
    check(guards(permanent[0])&&guards(permanent[1]),"native impact header/table canaries intact after publication");
    check(Com_GetParseThreadInfo()->parseInfoNum==0,"actual CSV parser sessions balanced");
    const unsigned before=registrations;shortRead=true;writeFiles();
    std::array<char,65536> list{};EffectFile effects{};CG_RegisterImpactEffectsForDir(const_cast<char *>("fx"),&effects,list.data());
    check(registrations==before&&high.empty()&&low.empty()&&opens==closes&&!file,"real short CSV read rejects before parsing and retires owned memory/file");
    brokenList=true;CG_RegisterImpactEffectsForDir(const_cast<char *>("fx"),&effects,list.data());check(opens==closes&&!file,"malformed list cannot reach file open");
    retire(permanent);printf("IMPACT_EFFECTS PASS checks=%u\n",checks);
}
