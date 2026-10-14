#pragma once
#include <qcommon/thread_context.h>

namespace bo1::android {
// Bounded process-lifetime startup roles. A role is claimed once, never recycled.
// Retirement is required before a worker exits; main is process lifetime.
// Stream and occlusion roles are admitted for their actual native owners.
// Queue/occlusion reuse requires completed retirement and a main-thread join.
// Server is admitted for its actual process-lifetime callback owner; no restart.
void RegisterThreadRole(ThreadContext_t role);
void RetireThreadRole();
// Caller must have joined this retired worker before admitting a new owner.
void ResetJoinedWorkerRole(ThreadContext_t role);
}
void __cdecl Sys_InitMainThread();
void __cdecl Sys_InitThread(int threadContext);
int __cdecl Sys_GetThreadContext();
bool __cdecl Sys_IsMainThread();
bool __cdecl Sys_IsRenderThread();
bool __cdecl Sys_IsDatabaseThread();
bool __cdecl Sys_IsStreamThread();
const char *__cdecl Sys_GetCurrentThreadName();
void __cdecl Sys_SetValue(int valueIndex, void *data);
void *__cdecl Sys_GetValue(int valueIndex);
