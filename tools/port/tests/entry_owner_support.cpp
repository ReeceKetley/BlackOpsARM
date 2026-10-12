// Standalone genuine-loader inputs/error endpoints only, not Dvar/LoadObj startup.
#include <database/db_entry_alloc_api.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
static dvar_t fastFile{};
const dvar_t*useFastFile=&fastFile;
void InitializeEntryOwnerInput(){fastFile.current.enabled=true;}
#if !defined(__ANDROID__)
char *va(const char*format,...){static thread_local char text[1024];va_list args;va_start(args,format);std::vsnprintf(text,sizeof(text),format,args);va_end(args);return text;}
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
