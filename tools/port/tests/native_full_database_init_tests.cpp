// Calls actual full DB_Init. This is pool initialization, not Com_Init/loading.
#include <platform/startup_services_api.h>
#include <platform/android/command_line_services.h>
#include <database/db_registry.h>
#include <database/db_asset_entry_types.h>
#include <clientscript/cscr_stringlist.h>
#if defined(BO1_NATIVE_ENUM_LOCALIZATION_PROBE)
#include <universal/com_memory.h>
#include <gfx_d3d/r_material_globals.h>
#include <gfx_d3d/r_image_globals.h>
#include <gfx_d3d/r_init.h>
#include <stringed/stringed_hooks.h>
#include <stringed/stringed_ingame.h>
#include <platform/android/zone_heap.h>
#endif
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
extern void *DB_XAssetPool[43];
extern void (__cdecl *DB_InitPoolHeaderHandler[43])(void *, int);
// Retail capacities captured independently from BlackOps64 db_registry.cpp.
// Source hash and macro names: native_registry_closure/pool-reference-counts.json.
constexpr int expectedPoolCounts[43]={64,64,64,64,6000,1000,4096,2048,4608,32,16,
    1,1,1,1,1,2,1,32,0,16,164,1024,16384,2048,0,0,1,500,4,0,0,0,0,0,0,
    1024,576,16,1,24,1,4};
#if defined(BO1_NATIVE_ENUM_LOCALIZATION_PROBE)
extern const dvar_t *useFastFile;
extern fileData_s *com_fileDataHashTable[1024];
extern CStringEdPackage *TheStringPackage;
extern GfxImage g_imageProgs[29];
struct CallbackState { void *expected;unsigned count; };
void enumerate(XAssetHeader header,void *context) {
    auto &state=*static_cast<CallbackState *>(context);void *pointer{};
    std::memcpy(&pointer,&header,sizeof(pointer));
    if(pointer!=state.expected)std::abort();++state.count;
}
#endif
extern "C" void __cyg_profile_func_enter(void *, void *) {}
extern "C" void __cyg_profile_func_exit(void *, void *) {}
void check(bool good,const char *message) {
    if(!good){std::fprintf(stderr,"FAIL %s\n",message);std::exit(1);}
}
int main(int argc,char **argv) {
    if(argc!=1)return 2;
    std::setvbuf(stdout,nullptr,_IONBF,0);
    bo1::android::InstallEngineCommandLine(argv[0], "+set bo1_headless 1 +set dedicated 1");
    Sys_InitializeCriticalSections();Sys_InitMainThread();Com_InitParse();Dvar_Init();
    SL_Init(SCRIPTINSTANCE_SERVER);SL_Init(SCRIPTINSTANCE_CLIENT);
    DB_Init();
    int linkedPools=0,singletons=0,absent=0;
    for(int type=0;type<43;++type) {
        const int expected=expectedPoolCounts[type];
        void *pool=DB_XAssetPool[type];
        if(!pool) {
            check(expected==0 && !DB_InitPoolHeaderHandler[type],"reference absent pool");++absent;
            std::printf("POOL %d absent 0\n",type);continue;
        }
        if(DB_InitPoolHeaderHandler[type]==DB_InitSingleton) {
            check(expected==1,"reference singleton capacity");++singletons;
            std::printf("POOL %d singleton 1\n",type);continue;
        }
        void *node;std::memcpy(&node,pool,sizeof(node));
        std::uintptr_t stride=0;int count=0;
        while(node) {
            check(count<expected,"free list bounded by independent reference capacity");
            void *next;std::memcpy(&next,node,sizeof(next));
            if(next) {
                auto current=reinterpret_cast<std::uintptr_t>(node),following=reinterpret_cast<std::uintptr_t>(next);
                check(following>current,"native next pointer advances");
                if(!stride)stride=following-current;
                check(following-current==stride,"native entries retain exact stride");
            }
            ++count;node=next;
        }
        check(count==expected,"full pool capacity matches BlackOps64 reference");++linkedPools;
        std::printf("POOL %d linked %d stride %llu\n",type,count,static_cast<unsigned long long>(stride));
    }
    check(g_freeAssetEntryHead==&g_assetEntryPool[16],"reserved first sixteen entries");
    unsigned entries=0;
    for(auto *node=g_freeAssetEntryHead;node;node=node->next) {
        check(entries<32752 && node==&g_assetEntryPool[16+entries],"native asset entry order and bounds");++entries;
    }
    check(entries==32752,"actual retail asset entry capacity");
    auto *debug=Dvar_FindVar("db_xassetdebug"),*debugType=Dvar_FindVar("db_xassetdebugtype");
    check(debug&&!debug->current.enabled&&debugType&&debugType->current.integer==-1
          &&debugType->domain.integer.min==-2&&debugType->domain.integer.max==43,"actual DB dvar registration");
    std::printf("PASS full DB_Init %d linked pools %d singletons %d absent types entries %u\n",
                linkedPools,singletons,absent,entries);
#if defined(BO1_NATIVE_ENUM_LOCALIZATION_PROBE)
    useFastFile=_Dvar_RegisterBool("useFastFile",true,0x10u,"Enables loading data from fast files.");
    CallbackState state{nullptr,0};
    DB_EnumXAssets(ASSET_TYPE_XMODEL,enumerate,&state,false);
    check(state.count==0,"actual empty FastFile enumeration");
    Dvar_SetBool(const_cast<dvar_t *>(useFastFile),false);
    XModel model{};Material material{};MaterialTechniqueSet technique{};GfxImage image{};
    fileData_s node{};node.data=&model;node.type=5;
    com_fileDataHashTable[0]=&node;
    state={&model,0};DB_EnumXAssets(ASSET_TYPE_XMODEL,enumerate,&state,false);
    check(state.count==1,"typed LoadObj XModel callback and original hash traversal");com_fileDataHashTable[0]=nullptr;
    rg.materialHashTable[1]=&material;
    state={&material,0};DB_EnumXAssets(ASSET_TYPE_MATERIAL,enumerate,&state,false);
    check(state.count==1,"typed original material callback");rg.materialHashTable[1]=nullptr;
    materialGlobals.techniqueSetHashTable[2]=&technique;
    state={&technique,0};DB_EnumXAssets(ASSET_TYPE_TECHNIQUE_SET,enumerate,&state,false);
    check(state.count==1,"typed original technique callback");materialGlobals.techniqueSetHashTable[2]=nullptr;
    imageGlobals.imageHashTable[3]=&image;imageGlobals.imageHashTable[4]=&g_imageProgs[0];
    state={&image,0};DB_EnumXAssets(ASSET_TYPE_IMAGE,enumerate,&state,false);
    check(state.count==1,"typed original image callback and program-image exclusion");
    imageGlobals.imageHashTable[3]=nullptr;imageGlobals.imageHashTable[4]=nullptr;
    Dvar_SetBool(const_cast<dvar_t *>(useFastFile),true);
    const auto heapBaseline=bo1::android::ZoneAllocationCount();
    SEH_InitLanguage();SEH_Init_StringEd();
    check(TheStringPackage && SEH_GetCurrentLanguage()==0,"actual localization initialization and language dvars");
    TheStringPackage->SetupNewFileParse("native_probe.str");
    char version[]="VERSION \"1\"",reference[]="REFERENCE \"GREETING\"";
    char english[]="LANG_ENGLISH \"Hello world\"",end[]="ENDMARKER";
    check(!TheStringPackage->ParseLine(version,false) && !TheStringPackage->ParseLine(reference,false)
        && !TheStringPackage->ParseLine(english,false) && !TheStringPackage->ParseLine(end,false),"original localization parser accepts complete entry");
    const auto *localized=SE_GetString_LoadObj("NATIVE_PROBE_GREETING");
    check(localized && !std::strcmp(localized,"Hello world") && TheStringPackage->m_bEndMarkerFound_ParseOnly,"actual parsed string lookup and end marker");
    SEH_Shutdown_StringEd();
    check(!TheStringPackage && bo1::android::ZoneAllocationCount()==heapBaseline,"actual localization package deletion and heap baseline");
    std::puts("PASS typed asset enumeration and original localization parser; fixture membership restored");
#endif
    Dvar_Shutdown();SL_Shutdown(SCRIPTINSTANCE_CLIENT);SL_Shutdown(SCRIPTINSTANCE_SERVER);
}
