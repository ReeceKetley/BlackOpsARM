// Finite callback verifies the real thread/event backend, not DB_Thread loading.
#include <platform/android/database_worker.h>
#include <platform/android/thread_services.h>
#include <universal/thread_data.h>
#include <cstdio>
#include <cstdlib>

namespace {
int payload;
int observed;
bool tlsMatched;
void check(bool value, const char *name) {
    if (!value) { std::fprintf(stderr, "FAIL %s\n", name); std::exit(1); }
    std::printf("PASS %s\n", name);
}
void finiteDatabase(unsigned int role) {
    tlsMatched = role == THREAD_CONTEXT_DATABASE && Sys_IsDatabaseThread()
        && Sys_GetValue(2) == g_com_error[THREAD_CONTEXT_DATABASE]
        && Sys_GetValue(1) == &va_info[THREAD_CONTEXT_DATABASE];
    Sys_WaitStartDatabase();
    Sys_WaitDiscReads();
    observed = payload;
    Sys_DatabaseCompleted();
}
}
// Compiler instrumentation observers are deliberately disabled in this probe.
extern "C" void __cyg_profile_func_enter(void *, void *) {}
extern "C" void __cyg_profile_func_exit(void *, void *) {}
// Test-owned assertion failure: never recover or report success on a violation.
bool Assert_MyHandler(const char *file, int line, int, const char *format, ...) {
    std::fprintf(stderr, "FAIL production assertion %s:%d %s\n", file, line, format);
    std::abort();
}
int main() {
    Sys_InitMainThread();
    check(Sys_SpawnDatabaseThread(finiteDatabase) == 1, "native database thread created");
    check(Sys_WaitForSingleObjectTimeout(&databaseCompletedEvent, 0)
          && Sys_WaitForSingleObjectTimeout(&databaseCompletedEvent2, 0)
          && Sys_WaitForSingleObjectTimeout(&resumedDatabaseEvent, 0), "original initial completion/resume states");
    check(!Sys_WaitForSingleObjectTimeout(&wakeDatabaseEvent, 0), "wake initially nonsignaled");
    Sys_WakeDatabase2();
    Sys_WakeDatabase();
    check(!Sys_WaitForSingleObjectTimeout(&databaseCompletedEvent, 0)
          && !Sys_WaitForSingleObjectTimeout(&databaseCompletedEvent2, 0), "both completion phases reset");
    Sys_SuspendDiscReads(THREAD_OWNER_DATABASE);
    check(Sys_HaveSuspendedDiscReads(THREAD_OWNER_DATABASE), "actual disc owner recorded");
    payload = 891;
    Sys_NotifyDatabase();
    check(!Sys_WaitForSingleObjectTimeout(&databaseCompletedEvent, 20), "disc suspension prevents completion");
    Sys_ResumeDiscReads(THREAD_OWNER_DATABASE);
    check(Sys_HaveSuspendedDiscReads(THREAD_OWNER_NONE), "actual disc owner released");
    check(Sys_WaitForSingleObjectTimeout(&databaseCompletedEvent, 2000), "worker completion signaled");
    bo1::android::JoinRetiredDatabaseThread();
    check(Sys_IsDatabaseReady() && !Sys_IsDatabaseReady2(), "actual readiness retains distinct phases");
    check(tlsMatched && observed == payload, "canonical TLS and published payload");
    check(Sys_DatabaseStopsServer()
          && !Sys_WaitForSingleObjectTimeout(&databaseCompletedEvent2, 0), "load completion does not imply postload completion");
    Sys_DatabaseCompleted2();
    check(!Sys_DatabaseStopsServer()
          && Sys_WaitForSingleObjectTimeout(&databaseCompletedEvent2, 0), "postload completes independently");
    check(Sys_GetThreadContext() == THREAD_CONTEXT_MAIN, "main identity preserved after genuine join");
}
