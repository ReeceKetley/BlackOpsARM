#pragma once

#include <Windows.h>
#if defined(__ANDROID__) || defined(BO1_NATIVE_FAUDIO_HOST_TEST)
namespace bo1::android { class VoiceCapture; class VoicePlayback; }
#else
#include <dsound.h>
#endif

struct __declspec(align(4)) dsound_sample_t // sizeof=0x48
{                                       // XREF: .data:s_recordingSamples/r
#if defined(__ANDROID__) || defined(BO1_NATIVE_FAUDIO_HOST_TEST)
    bo1::android::VoiceCapture *DSCB;
    bo1::android::VoicePlayback *DSB;
#else
    IDirectSoundCaptureBuffer *DSCB;
    IDirectSoundBuffer *DSB;
#endif
    unsigned int dwBufferSize;
    unsigned int dwCaptureOffset;
    unsigned int currentOffset;
    unsigned int lastOffset;
    unsigned int currentBufferLength;
    int stopPosition;
    unsigned int lastPlayPos;
    int bytesBuffered;
    int mode;
    int frequency;
    int volume;
    int pan;
    int channels;
    bool playing;
    // padding byte
    // padding byte
    // padding byte
    int channel;
    unsigned __int8 playMode;
    // padding byte
    // padding byte
    // padding byte
};
#if defined(__ANDROID__) || defined(BO1_NATIVE_FAUDIO_HOST_TEST)
static_assert(sizeof(dsound_sample_t)==80 && alignof(dsound_sample_t)>=alignof(void *), "native voice sample pointer alignment/layout");
#endif
