// Fixture-only missing VM diagnostic endpoints; actual SL/MT bodies are linked.
#include <platform/script_pool_services.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
void Com_Memset(void*d,int v,size_t n){std::memset(d,v,n);}
[[noreturn]] static void unavailable(const char*name){std::fprintf(stderr,"Explicit unavailable default-resolution VM diagnostic: %s\n",name);std::abort();}
void __cdecl Scr_DumpScriptThreads(scriptInstance_t){unavailable("threads");}
void __cdecl Scr_DumpScriptVariablesDefault(scriptInstance_t){unavailable("variables");}
void __cdecl Scr_TerminalError(const char*,scriptInstance_t){unavailable("terminal recovery");}
