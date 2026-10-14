#pragma once
void Sys_CreateEvent(bool manualReset, bool initialState, void **event);
void Sys_SetEvent(void **event);
void Sys_ResetEvent(void **event);
void Sys_DestroyEvent(void **event); // Owner must first quiesce all users.
void Sys_PulseManualEvent(void **event);
void Sys_WaitForSingleObject(void **event);
bool Sys_WaitForSingleObjectTimeout(void **event, unsigned int milliseconds);
char Sys_SpawnStreamThread(void (*function)(unsigned int));
void Stream_Thread(unsigned int threadContext);
int Sys_StreamSleep();
void Sys_SetSndInitializedEvent();
void Sys_WakeStream();
extern void *sndInitializedEvent;
extern void *streamCompletedEvent;
extern void *streamEvent;
