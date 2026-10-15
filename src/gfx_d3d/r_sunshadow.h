#pragma once
#include "r_rendercmds.h"

#if defined(__ANDROID__) && defined(BO1_NATIVE_VULKAN_RESOURCES) && BO1_NATIVE_VULKAN_RESOURCES
// Native sun-shadow far-partition cache (r_nativeSunShadowCache). The far
// partition is locked to a world grid so the static casters' depth can be kept
// across frames; only dynamic casters are redrawn. Frontend: whether the frame
// being set up uses it. Backend: the cache key of a frame (0 = draw normally).
bool R_SunShadowCacheActive();
unsigned long long R_SunShadowCacheKey(const GfxBackEndData *data);
void R_RegisterSunShadowCacheDvar();
#endif

void __cdecl R_SunShadowMapBoundingPoly(
                const GfxSunShadowBoundingPoly *boundingPoly,
                float sampleSize,
                float (*polyInClipSpace)[9][2],
                int *pointIsNear,
                unsigned int partitionRes);
void __cdecl R_SetupSunShadowMaps(const GfxViewParms *viewParms, GfxSunShadow *sunShadow);
void __cdecl R_GetSceneExtentsAlongDir(const float *origin, const float *forward, float *nearCap, float *farCap);
void __cdecl R_CalcBoxVsCylinderRayMinBox(
                const float *box,
                const float *origin,
                const float *forward,
                float radius,
                float *outBox);
void __cdecl R_GetSunAxes(float (*sunAxis)[3][3]);
unsigned int __cdecl R_SunShadowMapClipSpaceClipPlanes(
                const GfxSunShadowBoundingPoly *boundingPoly,
                int partitionIndex,
                float sampleSize,
                float (*boundingPolyClipSpacePlanes)[9][4],
                unsigned int partitionRes);
void __cdecl R_SunShadowMapProjectionMatrix(
                const float *snappedViewOrgInClipSpace,
                float shadowSampleSize,
                float nearClip,
                float farClip,
                GfxViewParms *shadowViewParms,
                unsigned int partitionRes);
void __cdecl R_SetupSunShadowMapProjection(
                const GfxViewParms *viewParms,
                const float (*sunAxis)[3][3],
                GfxSunShadow *sunShadow,
                float (*snappedViewOrgInClipSpace)[2][2],
                float *partitionFraction,
                unsigned int partitionRes);
void __cdecl R_GetFrustumNearClipPoints(const GfxMatrix *invViewProjMtx, float (*frustumPoints)[3]);
void __cdecl R_ClipSpaceToWorldSpace(
                const GfxMatrix *invViewProjMtx,
                const float (*clipSpacePoints)[4][3],
                int pointCount,
                float (*worldSpacePoints)[4][3]);
void __cdecl R_SetupSunShadowBoundingPoly(
                float (*frustumPointsInSunProj)[2],
                const float *viewOrgInSunProj,
                const float *snappedViewOrgInSunProj,
                float maxSizeInSunProj,
                const float *snappedViewOrgInClipSpace,
                GfxSunShadowBoundingPoly *boundingPoly,
                unsigned int pointCount);
void __cdecl R_SetupSunShadowMapViewMatrix(
                const float *snappedViewOrgInSunProj,
                const float (*sunAxis)[3],
                GfxSunShadowProjection *sunProj);
void __cdecl R_SetupSunShadowMapPartitionFraction(
                const GfxViewParms *viewParms,
                float scaleToFitUsable,
                GfxSunShadowProjection *sunProj,
                float *partitionFraction);
void __cdecl R_GetSunShadowMapPartitionViewOrgInTextureSpace(
                const float *viewOrgInPixels,
                const float *viewOrgInSunProj,
                const float *snappedViewOrgInSunProj,
                float sampleSize,
                float *viewOrgInTexSpace,
                unsigned int partitionRes);
void __cdecl R_SetupNearRegionPlane(const float *partitionFraction);
void __cdecl R_GetSunShadowLookupMatrix(
                const GfxViewParms *shadowViewParms,
                const GfxSunShadowProjection *sunProj,
                const float *partitionFraction,
                GfxMatrix *lookupMatrix);
void __cdecl R_SunShadowMaps();
void __cdecl R_MergeAndEmitSunShadowMapsSurfs(GfxViewInfo *viewInfo);
