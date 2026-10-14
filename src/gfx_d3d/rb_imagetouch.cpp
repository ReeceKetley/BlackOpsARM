#include "rb_imagetouch.h"
#include "r_init.h"
#include "r_dvars.h"
#include "rb_logfile.h"
#include "rb_shade.h"
#include "r_image.h"
#include "rb_draw3d.h"
#include "r_state_utils.h"
#if defined(__ANDROID__)
#include <cstdlib>
#include <cstdint>
#endif

void __cdecl RB_TouchAllImages()
{
  const char *v0; // eax
  const char *v1; // eax
  int v3; // [esp+4h] [ebp-4224h]
  int hr; // [esp+Ch] [ebp-421Ch]
  unsigned int i; // [esp+14h] [ebp-4214h]
#if defined(__ANDROID__)
  uint64_t v9;
#else
  int v9; // [esp+1Ch] [ebp-420Ch]
#endif
  ImageList imageList; // [esp+20h] [ebp-4208h] BYREF

  if ( !dx.inScene )
  {
    if ( r_logFile && r_logFile->current.integer )
      RB_LogPrint("dx.device->BeginScene()\n");
    hr = dx.device->BeginScene();
    if ( hr < 0 )
    {
      ++g_disableRendering;
      v0 = R_ErrorDescription(hr);
      Com_Error(
        ERR_FATAL,
        "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\rb_imagetouch.cpp (%i) dx.device->BeginScene() failed: %s\n",
        62,
        v0);
#if defined(__ANDROID__)
      std::abort();
#endif
    }
  }
  if ( tess.indexCount )
    RB_EndTessSurface();
  R_GetImageList(&imageList);
#if defined(__ANDROID__)
  if (imageList.count > ARRAY_COUNT(imageList.image)) { Com_Error(ERR_FATAL,"Image touch list exceeds actual capacity"); std::abort(); }
  for(unsigned index=0;index<imageList.count;++index) if(!imageList.image[index]) { Com_Error(ERR_FATAL,"Image touch list contains null image"); std::abort(); }
  qsort(imageList.image,imageList.count,sizeof(imageList.image[0]),[](const void *a,const void *b) {
      return RB_CompareTouchImages(static_cast<GfxImage *const *>(a),static_cast<GfxImage *const *>(b));
  });
#else
  qsort(imageList.image, imageList.count, sizeof(*(imageList.image)), (_CoreCrtNonSecureSearchSortCompareFunction)RB_CompareTouchImages);
#endif
  v9 = 0;
  for ( i = 0; i < imageList.count && imageList.image[i]->semantic; ++i )
  {
    RB_TouchImage(imageList.image[i]);
    v9 += imageList.image[i]->cardMemory.platform[0];
  }
  R_SetCodeImageTexture(&gfxCmdBufSourceState, 8u, 0);
  if ( !dx.inScene )
  {
    if ( r_logFile && r_logFile->current.integer )
      RB_LogPrint("dx.device->EndScene()\n");
    //v3 = ((int (__thiscall *)(IDirect3DDevice9 *, IDirect3DDevice9 *))dx.device->EndScene)(dx.device, dx.device);
    v3 = dx.device->EndScene();
    if ( v3 < 0 )
    {
      ++g_disableRendering;
      v1 = R_ErrorDescription(v3);
      Com_Error(
        ERR_FATAL,
        "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\rb_imagetouch.cpp (%i) dx.device->EndScene() failed: %s\n",
        79,
        v1);
#if defined(__ANDROID__)
      std::abort();
#endif
    }
  }
}

void __cdecl RB_TouchImage(GfxImage *image)
{
  if ( image->mapType == 3 )
  {
    R_SetCodeImageTexture(&gfxCmdBufSourceState, 8u, image);
    R_Set2D(&gfxCmdBufSourceState);
    RB_DrawStretchPic(
      rgp.feedbackReplaceMaterial,
      0.0,
      0.0,
      8.0,
      8.0,
      0.0,
      0.0,
      1.0,
      1.0,
      0xFFFFFFFF,
      GFX_PRIM_STATS_CODE);
    RB_EndTessSurface();
  }
}

int __cdecl RB_CompareTouchImages(GfxImage *const *e0, GfxImage *const *e1)
{
    const GfxImage *image = *e0;
    const GfxImage *other = *e1;
#if defined(__ANDROID__)
    if (!other->semantic && !image->semantic) return 0;
#endif
    if (!other->semantic) return -1;
    if (!image->semantic) return 1;
    if (other->cardMemory.platform[0] != image->cardMemory.platform[0])
        return other->cardMemory.platform[0] > image->cardMemory.platform[0] ? 1 : -1;
    return static_cast<int>(image->semantic) - static_cast<int>(other->semantic);
}
