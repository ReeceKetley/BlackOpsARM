// Actual canonical registry owners included once to exercise their private
// native adapters without retaining unrelated material/image renderer services.
#include <database/db_registry.cpp>
#include <ui/ui_main.h>
#include <type_traits>
#include <vector>
#include <new>
#include <cstdint>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <csignal>
static unsigned checks;
static uiInfo_s info;
static bool nullInfo;
static int *fatalMarker;
static void Check(bool ok,const char *why){if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);std::abort();}++checks;}
// An isolated real UI roster provider; no application/UI-ready claim.
uiInfo_s *UI_GetInfo(int local){Check(local==0,"real menu owner requests local0");return nullInfo?nullptr:&info;}
void Com_Error(errorParm_t,const char *format,...){if(fatalMarker)*fatalMarker=1;va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);std::fputc('\n',stderr);} // Intentional return tests fail-closed UI prerequisite guards.
bool Assert_MyHandler(const char *file,int line,int,const char *format,...){std::fprintf(stderr,"assert %s:%d ",file,line);va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);std::abort();}
using Handler=void (__cdecl *)(void *,XAssetHeader);
static_assert(std::is_same_v<Handler,std::remove_reference_t<decltype(DB_FreeXAssetHeaderHandler[0])>>);
static_assert(std::is_same_v<Handler,decltype(&DB_NativeFreeXAssetHeader<DB_FreeMaterial>)>);
static_assert(std::is_same_v<Handler,decltype(&DB_NativeFreeXAssetHeader<DB_FreeGfxImage>)>);
static_assert(std::is_same_v<Handler,decltype(&DB_NativeFreeXAssetHeader<DB_FreeMenu>)>);
static_assert(std::is_same_v<Handler,decltype(&DB_NativeFreeXAssetHeader<DB_FreeXGlobals>)>);
static menuDef_t others[600];
void MenuCase(int which){
    struct Storage{unsigned char before[16];XAssetPoolEntry<menuDef_t> entries[2];unsigned char after[16];} pool{};
    std::memset(pool.before,0xA5,sizeof(pool.before));std::memset(pool.after,0xA5,sizeof(pool.after));
    auto *released=&pool.entries[0];new(&released->entry)menuDef_t{};
    auto *previous=&pool.entries[1];XAssetPoolEntry<menuDef_t> *head=previous;
    XAssetHeader header{};header.menu=&released->entry;
    info={};const int count=which==5?600:5;info.uiDC.menuCount=count;
    for(int i=0;i<600;++i)info.uiDC.Menus[i]=&others[i];
    if(which==0)info.uiDC.Menus[0]=header.menu;
    if(which==1)info.uiDC.Menus[2]=header.menu;
    if(which==2)info.uiDC.Menus[4]=header.menu;
    if(which==3){info.uiDC.Menus[1]=header.menu;info.uiDC.Menus[2]=header.menu;info.uiDC.Menus[4]=header.menu;}
    if(which==5)info.uiDC.Menus[599]=header.menu;
    std::vector<menuDef_t *> expected;for(int i=0;i<count;++i)if(info.uiDC.Menus[i]!=header.menu)expected.push_back(info.uiDC.Menus[i]);
    const uiInfo_s snapshot=info;unsigned char releasedBefore[sizeof(*released)];std::memcpy(releasedBefore,released,sizeof(*released));
    volatile Handler dispatch=&DB_NativeFreeXAssetHeader<DB_FreeMenu>;
    dispatch(&head,header); // Actual exact-signature indirect call, UBSan enabled.
    Check(head==released && released->next==previous,"actual typed menu free publishes native pool links");
    Check(info.uiDC.menuCount==static_cast<int>(expected.size()),"actual roster retires every target occurrence");
    for(std::size_t i=0;i<expected.size();++i)Check(info.uiDC.Menus[i]==expected[i],"actual roster preserves survivor order and native pointer bits");
    const auto *after=reinterpret_cast<const unsigned char *>(&info);const auto *before=reinterpret_cast<const unsigned char *>(&snapshot);
    const std::size_t menus=offsetof(uiInfo_s,uiDC)+offsetof(UiContext,Menus),number=offsetof(uiInfo_s,uiDC)+offsetof(UiContext,menuCount);
    bool untouched=true;for(std::size_t i=0;i<sizeof(info);++i){const bool mutableByte=(i>=menus && i<menus+count*sizeof(void *)) || (i>=number && i<number+sizeof(int));if(!mutableByte && after[i]!=before[i])untouched=false;}
    Check(untouched,"actual removal preserves UI neighbors/unused roster tail canaries");
    Check(!std::memcmp(reinterpret_cast<unsigned char *>(released)+sizeof(void *),releasedBefore+sizeof(void *),sizeof(*released)-sizeof(void *)),"actual free changes only native first pointer slot");
    for(auto byte:pool.before)Check(byte==0xA5,"menu pool prefix canary");for(auto byte:pool.after)Check(byte==0xA5,"menu pool suffix canary");
    Check(reinterpret_cast<std::uintptr_t>(released)>UINT32_MAX,"actual native released pointer exceeds fixed32range");
}
void GlobalsCase(){
    XAssetPoolEntry<XGlobals> entries[2];auto *released=&entries[0];new(&released->entry)XGlobals{};
    auto *previous=&entries[1];XAssetPoolEntry<XGlobals> *head=previous;XAssetHeader header{};header.xGlobals=&released->entry;
    volatile Handler dispatch=&DB_NativeFreeXAssetHeader<DB_FreeXGlobals>;dispatch(&head,header);
    Check(head==released && released->next==previous,"actual typed XGlobals free retains native pointer owner bits");
}
int main(){
    alarm(15);for(int which=0;which<6;++which)MenuCase(which);GlobalsCase();
    fatalMarker=static_cast<int *>(mmap(nullptr,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0));Check(fatalMarker!=MAP_FAILED,"actual shared fatal marker");
    for(int which=0;which<3;++which){*fatalMarker=0;pid_t child=fork();Check(child>=0,"real malformed owner child");if(!child){info.uiDC.menuCount=which==0?-1:601;nullInfo=which==2;XAssetPoolEntry<menuDef_t> released;new(&released.entry)menuDef_t{};XAssetPoolEntry<menuDef_t>*head=nullptr;XAssetHeader header{};header.menu=&released.entry;volatile Handler dispatch=&DB_NativeFreeXAssetHeader<DB_FreeMenu>;dispatch(&head,header);_exit(94);}int status{};Check(waitpid(child,&status,0)==child && WIFSIGNALED(status) && WTERMSIG(status)==SIGABRT && *fatalMarker,"actual bad roster prerequisites fail after returning reporter");}
    std::printf("PASS: %u actual adapter/menu roster/pool publication observations\n",checks);
}
