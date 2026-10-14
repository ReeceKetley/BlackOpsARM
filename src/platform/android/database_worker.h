#pragma once
#include "stream_worker.h"
#include <qcommon/thread_owner.h>
void Sys_SuspendDiscReads(ThreadOwner owner);
void Sys_ResumeDiscReads(ThreadOwner owner);
bool Sys_HaveSuspendedDiscReads(ThreadOwner owner);
bool Sys_IsDatabaseReady();
bool Sys_IsDatabaseReady2();
void Sys_NotifyDatabase();
void Sys_SyncDatabase();
char Sys_SpawnDatabaseThread(void (*function)(unsigned int));
void Sys_WaitStartDatabase();
void Sys_WakeDatabase();
void Sys_WakeDatabase2();
void Sys_DatabaseCompleted();
void Sys_DatabaseCompleted2();
void Sys_WaitDiscReads();
bool Sys_DatabaseStopsServer();
extern void *serverCompletedEvent;
extern void *wakeDatabaseEvent;
extern void *databaseCompletedEvent;
extern void *databaseCompletedEvent2;
extern void *resumedDatabaseEvent;
namespace bo1::android {
// Only after the callback returns. The original endless DB_Thread cannot be
// joined until an actual shutdown path exists; never fake its termination.
void JoinRetiredDatabaseThread();
}
