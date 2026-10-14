#pragma once
#include "stream_worker.h"
extern void *occlusionEvent;
void Sys_SpawnOcclusion(void (*function)(unsigned int));
void Sys_WaitForOcclusion();
void Sys_SetOcclusion();
namespace bo1::android {
bool WaitForOcclusionThreadRetirement(unsigned milliseconds);
void JoinRetiredOcclusionThread();
}
