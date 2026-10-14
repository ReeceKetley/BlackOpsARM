#pragma once
#include <d3d9.h>
#include <cstdint>
#include <cstddef>
class IXAudio2;
class IXAudio2Voice;
// Native contract only: these fields are the canonical engine's consumed Bink surface,
// never a claim of binary compatibility with the proprietary Windows DLL.
struct BINKPLANE {int Allocate;void *Buffer;unsigned BufferPitch;};
struct BINKFRAMEPLANESET {BINKPLANE YPlane,cRPlane,cBPlane,APlane;};
struct BINKFRAMEBUFFERS {int TotalFrames;unsigned YABufferWidth,YABufferHeight,cRcBBufferWidth,cRcBBufferHeight,FrameNum;BINKFRAMEPLANESET Frames[2];};
struct BINKFRAMETEXTURES {unsigned Ysize,cRsize,cBsize,Asize;IDirect3DTexture9 *Ytexture,*cRtexture,*cBtexture,*Atexture;};
struct BINKTEXTURESET {BINKFRAMEBUFFERS bink_buffers;BINKFRAMETEXTURES textures[2],tex_draw;};
struct BINK {unsigned Width,Height,Frames,FrameNum,LastFrameNum,FrameRate,FrameRateDiv,OpenFlags;void *nativeOwner;};
using HBINK=BINK *;
struct BINKREALTIME {unsigned FrameNum,FrameRate,FrameRateDiv,Frames,FramesTime,FramesVideoDecompTime,FramesAudioDecompTime,FramesReadTime,FramesIdleReadTime,FramesThreadReadTime,FramesBlitTime,ReadBufferSize,ReadBufferUsed,FramesDataRate;};
namespace bo1::android {
HBINK OpenBinkMemory(const void *data,std::size_t length,unsigned flags,std::size_t budgetBytes);
HBINK OpenBinkFile(const char *path,unsigned flags,std::size_t budgetBytes);
bool BindBinkAudio(IXAudio2 *engine,IXAudio2Voice *output);
std::size_t BinkOwnedBytes(HBINK handle);
}
const char *BinkGetError();
void BinkClose(HBINK);
int BinkDoFrame(HBINK);
void BinkNextFrame(HBINK);
int BinkPause(HBINK,int);
int BinkWait(HBINK);
int BinkShouldSkip(HBINK);
void BinkGetFrameBuffersInfo(HBINK,BINKFRAMEBUFFERS *);
void BinkRegisterFrameBuffers(HBINK,BINKFRAMEBUFFERS *);
void BinkGetRealtime(HBINK,BINKREALTIME *,unsigned);
int BinkControlBackgroundIO(HBINK,unsigned);
void BinkSetSoundTrack(unsigned,unsigned *);
void BinkSetVolume(HBINK,unsigned,int);
