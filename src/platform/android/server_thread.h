#pragma once
#include <qcommon/thread_context.h>
#include <platform/android/stream_worker.h>
#include <universal/assertive.h>

char Sys_SpawnServerThread(void (*function)(unsigned int));
unsigned int Sys_GetCurrentThreadId();
extern void *wakeServerEvent;
extern void *serverCompletedEvent;
extern void *allowServerNetworkEvent;
extern void *serverNetworkCompletedEvent;
extern unsigned int g_networkOverrideThread;
void Sys_InitServerEvents();
void Sys_SleepServer();
bool Sys_WaitServer(unsigned int timeout);
void Sys_WakeServer();
void Sys_ServerCompleted();
bool Sys_WaitStartServer(unsigned int timeout);
bool Sys_IsServerThread();
void Sys_SetServerAllowNetworkEvent();
void Sys_ResetServerAllowNetworkEvent();
void Sys_SetServerNetworkCompletedEvent();
void Sys_ResetServerNetworkCompletedEvent();
void Sys_WaitAllowServerNetworkLoop();
void Sys_WaitServerNetworkCompleted();
namespace bo1::android {
// Call on main after genuine Com_SyncThreads, before script/error-lock cleanup.
// An endless server acknowledges only at its next completed iteration boundary.
void ParkServerForTerminalShutdown();
void ServerTerminalCheckpoint();
bool ServerTerminalShutdownEstablished();
}
