#pragma once
#include <qcommon/thread_context.h>
#include <sched.h>
#include <sys/types.h>
namespace bo1::android {
// Registered by the actual TLS owner; retirement excludes stale/reused TIDs.
void RegisterSchedulingThread(ThreadContext_t role);
void RetireSchedulingThread(ThreadContext_t role);
struct SchedulingState {
    pid_t tid;
    int inheritedNice,currentNice,lastError;
    cpu_set_t inheritedAffinity,currentAffinity;
    bool affinityChanged,niceChanged;
};
bool ReadSchedulingState(ThreadContext_t role,SchedulingState *output);
}
void Sys_ModApplyPriority(int mode);
void Sys_ModApplyPCore(int mode);
int Sys_ModOnPCore();
void Win_LockThreadAffinity();
void Win_UnlockThreadAffinity();
