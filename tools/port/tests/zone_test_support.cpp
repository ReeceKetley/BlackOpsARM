// Standalone boundary only. Logging emits real output; unported services abort.
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
void Com_Printf(int, const char *format, ...) { va_list args; va_start(args,format); std::vprintf(format,args); va_end(args); }
[[noreturn]] static void unavailable(const char *name) { std::fprintf(stderr,"Unavailable zone gate service: %s\n",name); std::abort(); }
int PMem_GetOverAllocatedSize() { unavailable("legacy PMem over-allocation accounting"); }
unsigned long long DB_XFileBytesLoaded() { unavailable("decompressed file byte accounting"); }
int g_dbTopAssetType;
