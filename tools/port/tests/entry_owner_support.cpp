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
