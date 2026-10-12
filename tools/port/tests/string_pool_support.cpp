// Standalone real memory/diagnostic providers. Full engine console/TLS/error
// recovery and unavailable VM diagnostics are not certified by this pool gate.
#include <platform/script_pool_services.h>
#include <universal/assertive.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
void Com_Memset(void*dst,const int value,const size_t count){std::memset(dst,value,count);}
#if !defined(BO1_FRONTEND_INTERN_GATE)
static void print(int channel,const char*format,va_list args){std::fprintf(stderr,"pool[%d] ",channel);std::vfprintf(stderr,format,args);std::fflush(stderr);}
void Com_Printf(int channel,const char*format,...){va_list args;va_start(args,format);print(channel,format,args);va_end(args);}
void Com_PrintError(int channel,const char*format,...){va_list args;va_start(args,format);print(channel,format,args);va_end(args);}
void Com_Error(errorParm_t code,const char*format,...){va_list args;va_start(args,format);print(static_cast<int>(code),format,args);va_end(args);std::fputs("Standalone engine error recovery unavailable\n",stderr);std::abort();}
#endif
char*va(const char*format,...){thread_local char buffers[4][4096];thread_local unsigned index;char*out=buffers[index++%4];va_list args;va_start(args,format);int n=std::vsnprintf(out,4096,format,args);va_end(args);if(n<0||n>=4096){std::fputs("Pool diagnostic formatter capacity exceeded\n",stderr);std::abort();}return out;}
#if !defined(BO1_FRONTEND_INTERN_GATE)
bool Assert_MyHandler(const char*file,int line,int kind,const char*format,...){std::fprintf(stderr,"Pool assertion %s:%d kind%d ",file,line,kind);va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);std::abort();}
#endif
void __cdecl Scr_DumpScriptThreads(scriptInstance_t){std::fputs("Unsupported standalone VM thread dump\n",stderr);std::abort();}
void __cdecl Scr_DumpScriptVariablesDefault(scriptInstance_t){std::fputs("Unsupported standalone VM variable dump\n",stderr);std::abort();}
void __cdecl Scr_TerminalError(const char*text,scriptInstance_t){std::fprintf(stderr,"Unsupported standalone VM terminal recovery: %s\n",text);std::abort();}
