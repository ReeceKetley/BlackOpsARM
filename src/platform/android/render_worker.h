#pragma once
#include "stream_worker.h"
extern void *renderPausedEvent,*renderCompletedEvent,*resourcesFlushedEvent,*resourcesQueuedEvent;
extern void *rendererRunningEvent,*backendEvent[2],*updateSpotLightEffectEvent,*updateEffectsEvent;
extern void *d3dDeviceOKEvent,*d3dDeviceHardStartEvent,*d3dShutdownEvent,*d3dDeviceWinMessageEvent;
extern void *win32QuitEvent,*win32ScriptDebuggerDrawEvent,*rgRegisteredEvent,*renderEvent;
char Sys_SpawnRenderThread(void (*function)(unsigned int));
void Sys_WaitResourcesFlushedEvent();
void Sys_ClearResourcesFlushedEvent();
void Sys_SetResourcesFlushedEvent();
void Sys_WaitResourcesQueuedEvent(unsigned int);
void Sys_ClearResourcesQueuedEvent();
void Sys_SetResourcesQueuedEvent();
void Sys_SetRGRegisteredEvent();
void Sys_ClearRGRegisteredEvent();
bool Sys_QueryRGRegisteredEvent();
bool Sys_QueryD3DDeviceOKEvent();
int Sys_WaitRenderer();
namespace bo1::android {
// Actual one-shot worker ownership; does not certify graphics readiness.
bool HasSpawnedRenderThread();
bool WaitForRenderThreadRetirement(unsigned milliseconds);
// Only a finite callback that actually retired may be joined; no restart API.
void JoinRetiredRenderThread();
}

// These operate on real synchronization handles. Signaled does not certify GPU readiness.
void Sys_WaitD3DDeviceOKEvent();
void Sys_ClearD3DDeviceOKEvent();
void Sys_SetD3DDeviceOKEvent();
void Sys_SetD3DDeviceStartEvent();
void Sys_ClearD3DDeviceStartEvent();
void Sys_WaitRGRegisteredEvent();
bool Sys_QueryRenderEvent();
void Sys_SetD3DShutdownEvent();
void Sys_ClearD3DShutdownEvent();
bool Sys_QueryD3DShutdownEvent();
void Sys_InitDemoStreamingEvent();
void Sys_WaitForDemoStreamingEvent();
void Sys_SetDemoStreamingEvent();
extern void *demoStreamingReady;
