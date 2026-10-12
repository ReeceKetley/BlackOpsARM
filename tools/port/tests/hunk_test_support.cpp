// Standalone gate only. Missing engine services abort; never linked into the APK.
#include <universal/mem_userhunk.h>
#include <universal/mem_firstfit.h>
#include <universal/mem_fixed.h>
#include <universal/com_errors.h>
#include <universal/memory_services.h>
#include <clientscript/cscr_evaluate.h>
#include <clientscript/cscr_vm.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
[[noreturn]] static void unsupported(const char *name) { std::fprintf(stderr, "Unsupported gate service: %s\n", name); std::abort(); }
#if !defined(BO1_CHECKED_ZONE_GATE)
bool Assert_MyHandler(const char *file, int line, int, const char *, ...) { std::fprintf(stderr,"Engine assertion %s:%d\n",file,line); std::abort(); }
void Com_Error(errorParm_t, const char *fmt, ...) { va_list a; va_start(a,fmt); std::vfprintf(stderr,fmt,a); va_end(a); std::abort(); }
#endif
#if !defined(BO1_FRONTEND_INTERN_GATE)
char *va(const char *fmt, ...) { static thread_local char b[512]; va_list a; va_start(a,fmt); std::vsnprintf(b,sizeof(b),fmt,a); va_end(a); return b; }
#endif
#if !defined(BO1_ZONE_GATE)
void Bo1_Diag(const char *fmt, ...) { va_list a; va_start(a,fmt); std::vfprintf(stderr,fmt,a); va_end(a); }
void Bo1_DiagStack(const char *) { unsupported("diagnostic stack"); }
#endif
#if !defined(BO1_REAL_FIRSTFIT_GATE)
bool Sys_IsMainThread() { unsupported("main-thread registration"); }
#endif
void track_userhunk_freerange(void *, unsigned int) { unsupported("tracking freerange"); }
void track_userhunk_alloc(int, int, const char *, int) { unsupported("tracking alloc"); }
void track_userhunk_free(void *) { unsupported("tracking free"); }
#if !defined(BO1_DEBUGGER_GATE)
unsigned int *Z_Malloc(int, const char *, int) { unsupported("debugger Z_Malloc"); }
void Z_Free(char *, int) { unsupported("debugger Z_Free"); }
void Scr_ClearDebugExprValue(scriptInstance_t, sval_u) { unsupported("debugger evaluator clear"); }
void Scr_FreeDebugExprValue(scriptInstance_t, sval_u) { unsupported("debugger evaluator free"); }
#endif
scrVmDebugPub_t gScrVmDebugPub[2];
#if !defined(BO1_REAL_FIRSTFIT_GATE)
HunkUser *Hunk_FirstFitInit(void *, unsigned int, HU_ALLOCATION_SCHEME, unsigned int, void *, const char *, int) { unsupported("first fit init"); }
void Hunk_FirstFitReset(HunkUser *) { unsupported("first fit reset"); }
void Hunk_FirstFitDestroy(HunkUser *) { unsupported("first fit destroy"); }
void *Hunk_FirstFitAlloc(HunkUser *, int, int) { unsupported("first fit alloc"); }
void Hunk_FirstFitFree(HunkUser *, void *) { unsupported("first fit free"); }
#endif
HunkUser *Hunk_FixedInit(void *, int, HU_ALLOCATION_SCHEME, unsigned int, void *, const char *, int) { unsupported("fixed init"); }
void Hunk_FixedReset(HunkUser *) { unsupported("fixed reset"); }
void Hunk_FixedDestroy(HunkUser *) { unsupported("fixed destroy"); }
void *Hunk_FixedAlloc(HunkUser *, int, int, const char *) { unsupported("fixed alloc"); }
void Hunk_FixedFree(HunkUser *, void *) { unsupported("fixed free"); }

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
