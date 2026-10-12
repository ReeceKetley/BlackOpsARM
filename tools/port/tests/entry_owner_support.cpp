// Standalone genuine-loader inputs/error endpoints only, not Dvar/LoadObj startup.
#include <database/db_entry_alloc_api.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#if defined(BO1_STARTUP_SERVICES_ZONE_INTEGRATION)
void InitializeEntryOwnerInput(){if(!useFastFile||useFastFile!=Dvar_FindVar("useFastFile")||!useFastFile->current.enabled){std::fputs("Real startup FastFile setting unavailable\n",stderr);std::abort();}}
#else
static dvar_t fastFile{};
const dvar_t*useFastFile=&fastFile;
void InitializeEntryOwnerInput(){fastFile.current.enabled=true;}
#endif
#if !defined(__ANDROID__) && !defined(BO1_STARTUP_SERVICES_ZONE_INTEGRATION)
char *va(const char*format,...){static thread_local char text[1024];va_list args;va_start(args,format);std::vsnprintf(text,sizeof(text),format,args);va_end(args);return text;}
#endif

#if defined(BO1_REGISTRY_ADD_GATE)
#include <database/db_registry_add_api.h>
#if defined(_WIN32)
#include <crtdbg.h>
#endif
#if defined(BO1_STARTUP_SERVICES_ZONE_INTEGRATION)
void InitializeRegistryAddInputs(){db_xassetdebug=_Dvar_RegisterBool("db_xassetdebug",false,0,"debug assets loading");db_xassetdebugtype=_Dvar_RegisterInt("db_xassetdebugtype",-1,-2,43,0,"debug assets loading type: -1 all; indexes start at 0");g_zoneIndex=1;}
#else
static dvar_t assetDebug{},assetDebugType{};
void InitializeRegistryAddInputs(){db_xassetdebug=&assetDebug;db_xassetdebugtype=&assetDebugType;assetDebug.current.enabled=false;assetDebugType.current.integer=-1;g_zoneIndex=1;}
#endif
[[noreturn]] static void registrationUnavailable(const char *name){
#if defined(_WIN32)
_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
#endif
std::fprintf(stderr,"Explicit unsupported standalone registration dependency: %s\n",name);std::fflush(stderr);std::abort();}
#if !defined(BO1_STARTUP_SERVICES_ZONE_INTEGRATION)
const char *Dvar_GetString(const char*){registrationUnavailable("Dvar_GetString");}
#endif
#if !defined(BO1_REGISTRY_DEFAULT_GATE)
XAssetEntry *DB_CreateDefaultEntry(XAssetType type,const char*){
#if defined(BO1_TECHSET_GATE)
extern void ReportTechniqueSetDefaultUnavailable();
if(type==ASSET_TYPE_TECHNIQUE_SET)ReportTechniqueSetDefaultUnavailable();
#endif
registrationUnavailable("DB_CreateDefaultEntry");}
#endif
bool DB_OverrideAsset(unsigned int,unsigned int){registrationUnavailable("DB_OverrideAsset");}
void DB_DelayedCloneXAsset(XAssetEntry*){registrationUnavailable("DB_DelayedCloneXAsset");}
void DB_SwapXAsset(XAsset*,XAsset*){registrationUnavailable("DB_SwapXAsset");}
void DB_CloneXAssetEntry(const XAssetEntry*,XAssetEntry*,DBCloneMethod){registrationUnavailable("DB_CloneXAssetEntry");}
void Mark_XAsset(){registrationUnavailable("Mark_XAsset");}
void NET_Sleep(unsigned int){registrationUnavailable("NET_Sleep");}
XAssetHeader DB_ModDefer(const XAssetEntry*){registrationUnavailable("DB_ModDefer");}
void DB_ModLinkModelMaterials(XModel*){registrationUnavailable("DB_ModLinkModelMaterials");}
#endif
void __cdecl DB_EnumXAssets(XAssetType,void(__cdecl *)(XAssetHeader,void*),void*,bool){std::fprintf(stderr,"Explicit unsupported standalone LoadObj diagnostic\n");std::abort();}

#if defined(BO1_REGISTRY_LOOKUP_GATE)
#include <database/db_registry_lookup_api.h>
#if defined(_WIN32)
#include <crtdbg.h>
#endif
XAssetEntryPoolEntry *DB_ModLinkDeferredByName(XAssetType,const char*){
#if defined(_WIN32)
_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
#endif
std::fprintf(stderr,"Explicit unsupported deferred mod-zone linking in standalone lookup gate\n");std::fflush(stderr);std::abort();}
#endif
