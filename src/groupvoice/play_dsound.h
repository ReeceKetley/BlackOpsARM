#pragma once

#include "directsound.h"
#if defined(__ANDROID__) || defined(BO1_NATIVE_FAUDIO_HOST_TEST)
struct HWND__; // Original opaque parameter; actual native routing uses the live canonical graph.
#endif

int __cdecl DSound_GetBytesLeft(dsound_sample_t *sample);
unsigned int __cdecl DSound_UpdateSample(dsound_sample_t *sample, char *data, signed int data_len);
void __cdecl DSound_AdjustSamplePlayback(dsound_sample_t *sample, int bytesLeft);
bool __cdecl DSound_BufferUnderrunOccurred(dsound_sample_t *sample);
void __cdecl DSound_HandleBufferUnderrun(dsound_sample_t *sample);
void __cdecl DSound_SampleFrame(dsound_sample_t *sample);
dsound_sample_t *__cdecl DSound_NewSample();
#if !(defined(__ANDROID__) || defined(BO1_NATIVE_FAUDIO_HOST_TEST))
HRESULT __cdecl CreateBasicBuffer(
                IDirectSound8 *lpDirectSound,
                IDirectSoundBuffer **ppDsb,
                unsigned int sampleRate,
                unsigned __int16 channels,
                unsigned int bufferSize);
#endif
char __cdecl DSound_StopSample(dsound_sample_t *sample);
int __cdecl DSound_Init(bool callDsoundInit, HWND__ *handle);
void __cdecl DSound_Shutdown();
