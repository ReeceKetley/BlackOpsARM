// Real Windows thread identity and recursive critical section for the host gate.
#include <Windows.h>
#include <win32/win_common.h>
#include "hunk_main_thread_test.h"
#include <cstdlib>
static DWORD testMainThread;
struct Lock {
    CRITICAL_SECTION section;
    Lock() { InitializeCriticalSection(&section); }
    ~Lock() { DeleteCriticalSection(&section); }
};
static Lock firstFitLock;
void RegisterHunkTestMainThread() { testMainThread = GetCurrentThreadId(); }
bool IsHunkTestMainThread() { return testMainThread == GetCurrentThreadId(); }
bool Sys_IsMainThread() { return IsHunkTestMainThread(); }
void Sys_EnterCriticalSection(CriticalSection s)
{
    if (s != CRITSECT_MEMFIRSTFIT) std::abort();
    EnterCriticalSection(&firstFitLock.section);
}
void Sys_LeaveCriticalSection(CriticalSection s)
{
    if (s != CRITSECT_MEMFIRSTFIT) std::abort();
    LeaveCriticalSection(&firstFitLock.section);
}
