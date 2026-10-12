#pragma once
#include <qcommon/thread_context.h>

namespace bo1::android {
// Bounded process-lifetime startup roles. A role is claimed once, never recycled.
// Retirement is required before a worker exits; main is process lifetime.
// Server/worker/event integration is deliberately not provided by this boundary.
void RegisterThreadRole(ThreadContext_t role);
void RetireThreadRole();
}
void __cdecl Sys_InitMainThread();
void __cdecl Sys_InitThread(int threadContext);
int __cdecl Sys_GetThreadContext();
bool __cdecl Sys_IsMainThread();
bool __cdecl Sys_IsRenderThread();
bool __cdecl Sys_IsDatabaseThread();
void __cdecl Sys_SetValue(int valueIndex, void *data);
void *__cdecl Sys_GetValue(int valueIndex);
