// Standalone gate only. Missing runtime services fail explicitly. Windows
// tracking bodies are disabled in production; these empty hooks match that
// disabled tracking. Ownership measurements inspect actual g_mem directly.
#include <universal/com_errors.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#if defined(_WIN32)
#include <Windows.h>
#include <qcommon/msg_mp.h>
#endif
void Com_Printf(int, const char *, ...) {}
void Com_PrintError(int, const char *f, ...) { va_list a; va_start(a,f); std::vfprintf(stderr,f,a); va_end(a); }
void tlPrintf(const char *, ...) {}
bool Assert_MyHandler(const char *f, int l, int, const char *, ...) { std::fprintf(stderr,"Assertion %s:%d\n",f,l); std::abort(); }
void Com_Error(errorParm_t, const char *f, ...) { va_list a; va_start(a,f); std::vfprintf(stderr,f,a); va_end(a); std::abort(); }
unsigned int *Z_Malloc(int, const char *, int) { std::abort(); }
void Z_Free(char *, int) { std::abort(); }
#if defined(_WIN32)
const char *I_stristr(const char *a, const char *b) {
    for (; *a; ++a) if (!_strnicmp(a,b,std::strlen(b))) return a;
    return nullptr;
}
int I_stricmp(const char *a, const char *b) { return _stricmp(a,b); }
void track_flush_physical_alloc(const char *, unsigned char) {}
void track_physical_alloc(int, const char *, int, int) {}
void track_PrintInfo() { std::fputs("PMem tracker disabled in production/reference fixture\n",stderr); }
void Sys_OutOfMemErrorInternal(const char *f, int l) { std::fprintf(stderr,"Production PMem OOM %s:%d\n",f,l); std::exit(23); }
#else
int PMem_GetOverAllocatedSize() { std::fputs("Unavailable legacy over-allocation accounting\n",stderr); std::abort(); }
#endif

#if defined(_WIN32)
// Data-only network fixtures required by unused static header tables. The gate
// never calls networking; these zeroed arrays provide no runtime service.
NetField eventEntityStateFields[69]{};
NetField actorStateFields[69]{};
NetField vehicleEntityStateFields[71]{};
NetField planeStateFields[66]{};
NetField helicopterEntityStateFields[71]{};
NetField turretEntityStateFields[69]{};
NetField loopFxEntityStateFields[69]{};
NetField fxStateFields[68]{};
NetField soundBlendEntityStateFields[68]{};
NetField scriptMoverStateFields[72]{};
NetField missileEntityStateFields[69]{};
NetField itemEntityStateFields[69]{};
NetField corpseEntityStateFields[68]{};
NetField playerEntityStateFields[74]{};
NetField archivedEntityFields[8]{};
NetField entityStateFields[69]{};
NetField clientStateFields[50]{};
NetField playerStateFields[179]{};
NetField objectiveFields[9]{};
NetField matchStateFields[15]{};
#endif
