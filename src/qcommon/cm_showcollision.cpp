#include "cm_showcollision.h"
#include <xanim/xanim.h>
#include "common.h"
#include <common/brush_edges.h>
#include "cm_load.h"

#if defined(__ANDROID__)
#include <cmath>
#include <cstdlib>
namespace {
void CollisionRequire(bool valid,const char *reason) {
    if(valid)return;
    Assert_MyHandler(__FILE__,__LINE__,0,"%s",reason);
    std::abort(); // Failed prerequisites must never permit later writes.
}
void CollisionBrushBounds(const cbrush_t *brush) {
    CollisionRequire(brush && brush->numsides<=32761,"collision brush side indices fit signed 16-bit");
    CollisionRequire(!brush->numsides || brush->sides,"collision brush side ownership");
    for(int axis=0;axis<3;++axis)
        CollisionRequire(std::isfinite(brush->mins[axis]) && std::isfinite(brush->maxs[axis])
            && brush->mins[axis]<=brush->maxs[axis],"finite ordered collision brush bounds");
    for(unsigned side=0;side<brush->numsides;++side) {
        const cplane_s *plane=brush->sides[side].plane;
        CollisionRequire(plane && std::isfinite(plane->dist),"collision side plane ownership");
        float length=0;
        for(int axis=0;axis<3;++axis) {
            CollisionRequire(std::isfinite(plane->normal[axis]),"finite collision side normal");
            length+=plane->normal[axis]*plane->normal[axis];
        }
        CollisionRequire(std::isfinite(length) && length>0,"nonzero collision side normal");
    }
}
float *CollisionPoint(CMShowCollisionWinding *w,int index) {
    CollisionRequire(w && w->numpoints>=0 && w->numpoints<=1024,"owned collision winding count");
    CollisionRequire(index>=0 && index<1024,"owned collision winding point index");
    return w->p[index];
}
const float *CollisionPoint(const CMShowCollisionWinding *w,int index) {
    CollisionRequire(w && w->numpoints>=0 && w->numpoints<=1024,"owned collision winding count");
    CollisionRequire(index>=0 && index<1024,"owned collision winding point index");
    return w->p[index];
}
}
#define CM_COLLISION_POINT(w,index) CollisionPoint(w,index)
#else
#define CM_COLLISION_POINT(w,index) ((w)->p[index])
unsigned __int8 windingPool[12292];
#endif


#if defined(__ANDROID__)
#define CM_AXIAL(planes,row,col) ((planes)[row][col])
#define CM_XYZ_ROWS(xyz) (xyz)
#define CM_LOCAL_XYZ(xyz,row) ((xyz)[row])
#define CM_LOCAL_COORD(xyz,index) ((xyz)[(index)/3][(index)%3])
#define CM_XYZ_ROW(points,row) ((points)[row])
#define CM_VIEWER_WINDING (&ownedWinding)
#define CM_VIEWER_POINTS brushPts
#define CM_DRAW_WINDING(w,color) drawCollisionPoly((w)->numpoints,(w)->p,color)
#else
#define CM_AXIAL(planes,row,col) ((*planes)[4*(row)+(col)])
#define CM_XYZ_ROWS(xyz) ((float (*)[3])xyz)
#define CM_LOCAL_XYZ(xyz,row) (&xyz[3*(row)])
#define CM_LOCAL_COORD(xyz,index) (xyz[index])
#define CM_XYZ_ROW(points,row) (&(*points)[3*(row)])
#define CM_VIEWER_WINDING ((CMShowCollisionWinding *)windingPool)
#define CM_VIEWER_POINTS (&brushPts)
#define CM_DRAW_WINDING(w,color) drawCollisionPoly(*(_DWORD *)windingPool,(float (*)[3]) &windingPool[4],color)
#endif
void __cdecl CM_GetPlaneVec4Form(
                const cbrushside_t *sides,
                const float (*axialPlanes)[4],
                int index,
                float *expandedPlane)
{
    cplane_s *plane; // [esp+0h] [ebp-8h]
    float *v5; // [esp+4h] [ebp-4h]

#if defined(__ANDROID__)
    CollisionRequire(index>=0 && expandedPlane && (index>=6 ? sides && sides[index-6].plane : axialPlanes!=nullptr),"collision plane input ownership");
#endif
    if ( index >= 6 )
    {
        if ( !sides
            && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\qcommon\\cm_showcollision.cpp", 107, 0, "%s", "sides") )
        {
            __debugbreak();
        }
        plane = sides[index - 6].plane;
        *expandedPlane = plane->normal[0];
        expandedPlane[1] = plane->normal[1];
        expandedPlane[2] = plane->normal[2];
        expandedPlane[3] = sides[index - 6].plane->dist;
    }
    else
    {
        if ( !axialPlanes
            && !Assert_MyHandler(
                        "C:\\projects_pc\\cod\\codsrc\\src\\qcommon\\cm_showcollision.cpp",
                        102,
                        0,
                        "%s",
                        "axialPlanes") )
        {
            __debugbreak();
        }
        #if defined(__ANDROID__)
        v5 = const_cast<float *>(axialPlanes[index]);
#else
        v5 = (float *)&(*axialPlanes)[4 * index];
#endif
        *expandedPlane = *v5;
        expandedPlane[1] = v5[1];
        expandedPlane[2] = v5[2];
        expandedPlane[3] = v5[3];
    }
}

void __cdecl CM_ShowSingleBrushCollision(
    const cbrush_t *brush,
    const float *color,
    void(__cdecl *drawCollisionPoly)(int, float (*)[3], const float *))
{
    int ptCount; // [esp+30h] [ebp-506Ch]
#if defined(__ANDROID__)
    ShowCollisionBrushPt brushPts[1024];
    CMShowCollisionWinding ownedWinding{};
#else
    ShowCollisionBrushPt brushPts; // [esp+34h] [ebp-5068h] BYREF
#endif
    int sideIndex; // [esp+5038h] [ebp-64h]
#if defined(__ANDROID__)
    float axialPlanes[6][4];
    CollisionBrushBounds(brush);
    CollisionRequire(color && drawCollisionPoly,"collision draw callback/color ownership");
#else
    float axialPlanes[4][4]; // [esp+503Ch] [ebp-60h] BYREF

    if (!brush
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\qcommon\\cm_showcollision.cpp", 403, 0, "%s", "brush"))
    {
        __debugbreak();
    }
    if (!color
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\qcommon\\cm_showcollision.cpp", 404, 0, "%s", "color"))
    {
        __debugbreak();
    }
#endif
    CM_BuildAxialPlanes(brush, axialPlanes);
    ptCount = CM_ForEachBrushPlaneIntersection(brush, axialPlanes, CM_VIEWER_POINTS);
    if (ptCount >= 4)
    {
        for (sideIndex = 0; (unsigned int)sideIndex < 6; ++sideIndex)
        {
            if (CM_BuildBrushWindingForSide(CM_VIEWER_WINDING, axialPlanes[sideIndex], sideIndex, CM_VIEWER_POINTS, ptCount))
                CM_DRAW_WINDING(CM_VIEWER_WINDING,color);
        }
        for (sideIndex = 6; sideIndex < brush->numsides + 6; ++sideIndex)
        {
            if (CM_BuildBrushWindingForSide(
                CM_VIEWER_WINDING,
                brush->sides[sideIndex - 6].plane->normal,
                sideIndex,
                CM_VIEWER_POINTS,
                ptCount))
            {
                CM_DRAW_WINDING(CM_VIEWER_WINDING,color);
            }
        }
    }
}

void __cdecl CM_BuildAxialPlanes(const cbrush_t *brush, float (*axialPlanes)[4])
{
    float v2; // [esp+4h] [ebp-28h]
    float v3; // [esp+Ch] [ebp-20h]
    float v4; // [esp+14h] [ebp-18h]
    float v5; // [esp+1Ch] [ebp-10h]
    float v6; // [esp+24h] [ebp-8h]
    float v7; // [esp+28h] [ebp-4h]

#if defined(__ANDROID__)
    CollisionBrushBounds(brush);
    CollisionRequire(axialPlanes,"six-row axial plane ownership");
#endif
    v7 = -brush->mins[0];
    CM_AXIAL(axialPlanes,0,0) = -1.0f;
    CM_AXIAL(axialPlanes,0,1) = 0.0f;
    CM_AXIAL(axialPlanes,0,2) = 0.0f;
    CM_AXIAL(axialPlanes,0,3) = v7;
    v6 = brush->maxs[0];
    CM_AXIAL(axialPlanes,1,0) = 1.0f;
    CM_AXIAL(axialPlanes,1,1) = 0.0f;
    CM_AXIAL(axialPlanes,1,2) = 0.0f;
    CM_AXIAL(axialPlanes,1,3) = v6;
    v5 = -brush->mins[1];
    CM_AXIAL(axialPlanes,2,0) = 0.0f;
    CM_AXIAL(axialPlanes,2,1) = -1.0f;
    CM_AXIAL(axialPlanes,2,2) = 0.0f;
    CM_AXIAL(axialPlanes,2,3) = v5;
    v4 = brush->maxs[1];
    CM_AXIAL(axialPlanes,3,0) = 0.0f;
    CM_AXIAL(axialPlanes,3,1) = 1.0f;
    CM_AXIAL(axialPlanes,3,2) = 0.0f;
    CM_AXIAL(axialPlanes,3,3) = v4;
    v3 = -brush->mins[2];
    CM_AXIAL(axialPlanes,4,0) = 0.0f;
    CM_AXIAL(axialPlanes,4,1) = 0.0f;
    CM_AXIAL(axialPlanes,4,2) = -1.0f;
    CM_AXIAL(axialPlanes,4,3) = v3;
    v2 = brush->maxs[2];
    CM_AXIAL(axialPlanes,5,0) = 0.0f;
    CM_AXIAL(axialPlanes,5,1) = 0.0f;
    CM_AXIAL(axialPlanes,5,2) = 1.0f;
    CM_AXIAL(axialPlanes,5,3) = v2;
}

int __cdecl CM_ForEachBrushPlaneIntersection(
                const cbrush_t *brush,
                const float (*axialPlanes)[4],
                ShowCollisionBrushPt *brushPts)
{
    const float *plane[3]; // [esp+Ch] [ebp-60h] BYREF
    int sideCount; // [esp+18h] [ebp-54h]
    int ptCount; // [esp+1Ch] [ebp-50h]
    float xyz[3]; // [esp+20h] [ebp-4Ch] BYREF
    float expandedPlane[3][4]; // [esp+2Ch] [ebp-40h] BYREF
    __int16 sideIndex[4]; // [esp+5Ch] [ebp-10h] BYREF
    const cbrushside_t *sides; // [esp+68h] [ebp-4h]

    if ( !brush
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\qcommon\\cm_showcollision.cpp", 125, 0, "%s", "brush") )
    {
        __debugbreak();
    }
    if ( !brushPts
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\qcommon\\cm_showcollision.cpp", 126, 0, "%s", "brushPts") )
    {
        __debugbreak();
    }
#if defined(__ANDROID__)
    CollisionBrushBounds(brush);
    CollisionRequire(axialPlanes && brushPts,"owned collision intersection output");
#endif
    ptCount = 0;
    plane[0] = expandedPlane[0];
    plane[1] = expandedPlane[1];
    plane[2] = expandedPlane[2];
    sideCount = brush->numsides + 6;
    sides = brush->sides;
    for ( sideIndex[0] = 0; sideIndex[0] < sideCount - 2; ++sideIndex[0] )
    {
        CM_GetPlaneVec4Form(sides, axialPlanes, sideIndex[0], expandedPlane[0]);
        for ( sideIndex[1] = sideIndex[0] + 1; sideIndex[1] < sideCount - 1; ++sideIndex[1] )
        {
            if ( sideIndex[0] < 6 || sideIndex[1] < 6 || sides[sideIndex[0] - 6].plane != sides[sideIndex[1] - 6].plane )
            {
                CM_GetPlaneVec4Form(sides, axialPlanes, sideIndex[1], expandedPlane[1]);
                for ( sideIndex[2] = sideIndex[1] + 1; sideIndex[2] < sideCount; ++sideIndex[2] )
                {
                    if ( (sideIndex[0] < 6 || sideIndex[2] < 6 || sides[sideIndex[0] - 6].plane != sides[sideIndex[2] - 6].plane)
                        && (sideIndex[1] < 6 || sideIndex[2] < 6 || sides[sideIndex[1] - 6].plane != sides[sideIndex[2] - 6].plane) )
                    {
                        CM_GetPlaneVec4Form(sides, axialPlanes, sideIndex[2], expandedPlane[2]);
                        if ( IntersectPlanes(plane, xyz) )
                        {
                            SnapPointToIntersectingPlanes(plane, xyz, 0.25, 0.0099999998);
                            ptCount = CM_AddSimpleBrushPoint(brush, axialPlanes, sideIndex, xyz, ptCount, brushPts);
                        }
                    }
                }
            }
        }
    }
    return ptCount;
}

int __cdecl CM_AddSimpleBrushPoint(
                const cbrush_t *brush,
                const float (*axialPlanes)[4],
                const __int16 *sideIndices,
                const float *xyz,
                int ptCount,
                ShowCollisionBrushPt *brushPts)
{
    ShowCollisionBrushPt *v7; // [esp+0h] [ebp-14h]
    cplane_s *plane; // [esp+8h] [ebp-Ch]
    unsigned int sideIndex; // [esp+10h] [ebp-4h]
    unsigned int sideIndexa; // [esp+10h] [ebp-4h]

    if ( !brush
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\qcommon\\cm_showcollision.cpp", 67, 0, "%s", "brush") )
    {
        __debugbreak();
    }
    if ( !brushPts
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\qcommon\\cm_showcollision.cpp", 68, 0, "%s", "brushPts") )
    {
        __debugbreak();
    }
#if defined(__ANDROID__)
    CollisionBrushBounds(brush);
    CollisionRequire(axialPlanes && sideIndices && xyz && brushPts && ptCount>=0 && ptCount<=1024,"collision intersection ownership/count");
    for(int n=0;n<3;++n)CollisionRequire(sideIndices[n]>=0 && sideIndices[n]<brush->numsides+6 && std::isfinite(xyz[n]),"collision intersection indices/coordinates");
#endif
    for ( sideIndex = 0; sideIndex < 6; ++sideIndex )
    {
        if ( (float)((float)((float)((float)((float)CM_AXIAL(axialPlanes,sideIndex,0) * *xyz)
                                                             + (float)((float)CM_AXIAL(axialPlanes,sideIndex,1) * xyz[1]))
                                             + (float)((float)CM_AXIAL(axialPlanes,sideIndex,2) * xyz[2]))
                             - (float)CM_AXIAL(axialPlanes,sideIndex,3)) > 0.1 )
            return ptCount;
    }
    for ( sideIndexa = 0; sideIndexa < brush->numsides; ++sideIndexa )
    {
        plane = brush->sides[sideIndexa].plane;
        if (
#if defined(__ANDROID__)
            (sideIndices[0]<6 || plane != brush->sides[sideIndices[0]-6].plane)
            && (sideIndices[1]<6 || plane != brush->sides[sideIndices[1]-6].plane)
            && (sideIndices[2]<6 || plane != brush->sides[sideIndices[2]-6].plane)
#else
            plane != brush->sides[*sideIndices - 6].plane
            && plane != brush->sides[sideIndices[1] - 6].plane
            && plane != brush->sides[sideIndices[2] - 6].plane
#endif
            && (float)((float)((float)((float)(plane->normal[0] * *xyz) + (float)(plane->normal[1] * xyz[1]))
                                             + (float)(plane->normal[2] * xyz[2]))
                             - plane->dist) > 0.1 )
        {
            return ptCount;
        }
    }
    if ( ptCount >= 1024 ) {
        Com_Error(ERR_DROP, "More than %i points from plane intersections on %i-sided brush\n", ptCount, brush->numsides);
#if defined(__ANDROID__)
        std::abort();
#endif
    }
    v7 = &brushPts[ptCount];
    v7->xyz[0] = *xyz;
    v7->xyz[1] = xyz[1];
    v7->xyz[2] = xyz[2];
    v7->sideIndex[0] = *sideIndices;
    v7->sideIndex[1] = sideIndices[1];
    v7->sideIndex[2] = sideIndices[2];
    return ptCount + 1;
}

char __cdecl CM_BuildBrushWindingForSide(
                CMShowCollisionWinding *winding,
                float *planeNormal,
                int sideIndex,
                const ShowCollisionBrushPt *pts,
                int ptCount)
{
    // kcod4
    int j; // [esp+Ch] [ebp-302Ch] BYREF
    int i2; // [esp+10h] [ebp-3028h] BYREF
    int XyzList; // [esp+14h] [ebp-3024h]
#if defined(__ANDROID__)
    float xyz[1024][3];
#else
    float xyz[3072]; // [esp+18h] [ebp-3020h] BYREF
#endif
    int i0; // [esp+3018h] [ebp-20h] BYREF
    int i1; // [esp+301Ch] [ebp-1Ch] BYREF
    int k; // [esp+3020h] [ebp-18h]
    int i; // [esp+3024h] [ebp-14h] BYREF
    float plane[4]; // [esp+3028h] [ebp-10h] BYREF

#if defined(__ANDROID__)
    CollisionRequire(winding && planeNormal && pts && sideIndex>=0 && ptCount>=0 && ptCount<=1024,"owned collision winding builder inputs");
    winding->numpoints=0;
#endif
    iassert(winding);
    iassert(planeNormal);
    iassert(pts);

    XyzList = CM_GetXyzList(sideIndex, pts, ptCount, CM_XYZ_ROWS(xyz), 1024);
    if (XyzList < 3)
        return 0;

    CM_PickProjectionAxes(planeNormal, &i, &j);

    CM_COLLISION_POINT(winding,0)[0] = CM_LOCAL_COORD(xyz,0);
    CM_COLLISION_POINT(winding,0)[1] = CM_LOCAL_COORD(xyz,1);
    CM_COLLISION_POINT(winding,0)[2] = CM_LOCAL_COORD(xyz,2);
    CM_COLLISION_POINT(winding,1)[0] = CM_LOCAL_COORD(xyz,3);
    CM_COLLISION_POINT(winding,1)[1] = CM_LOCAL_COORD(xyz,4);
    CM_COLLISION_POINT(winding,1)[2] = CM_LOCAL_COORD(xyz,5);
    winding->numpoints = 2;

    for (k = 2; k < XyzList; ++k)
        CM_AddExteriorPointToWindingProjected(winding, CM_LOCAL_XYZ(xyz,k), i, j);

    if (CM_RepresentativeTriangleFromWinding(winding, planeNormal, &i0, &i1, &i2) < EQUAL_EPSILON)
        return 0;

    PlaneFromPoints(plane, CM_COLLISION_POINT(winding,i0), CM_COLLISION_POINT(winding,i1), CM_COLLISION_POINT(winding,i2));

    if (Vec3Dot(plane, planeNormal) > 0.0)
        CM_ReverseWinding(winding);

    return 1;
}

int __cdecl CM_GetXyzList(int sideIndex, const ShowCollisionBrushPt *pts, int ptCount, float (*xyz)[3], int xyzLimit)
{
    float *v6; // [esp+0h] [ebp-10h]
    const ShowCollisionBrushPt *v7; // [esp+4h] [ebp-Ch]
    int xyzCount; // [esp+8h] [ebp-8h]
    int ptIndex; // [esp+Ch] [ebp-4h]

    if ( !pts
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\qcommon\\cm_showcollision.cpp", 185, 0, "%s", "pts") )
    {
        __debugbreak();
    }
    if ( !xyz
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\qcommon\\cm_showcollision.cpp", 186, 0, "%s", "xyz") )
    {
        __debugbreak();
    }
#if defined(__ANDROID__)
    CollisionRequire(pts && xyz && ptCount>=0 && ptCount<=1024 && xyzLimit>=0 && xyzLimit<=1024,"collision face point list bounds");
#endif
    xyzCount = 0;
    for ( ptIndex = 0; ptIndex < ptCount; ++ptIndex )
    {
        if ( (sideIndex == pts[ptIndex].sideIndex[0]
             || sideIndex == pts[ptIndex].sideIndex[1]
             || sideIndex == pts[ptIndex].sideIndex[2])
            && !CM_PointInList(pts[ptIndex].xyz, xyz, xyzCount) )
        {
            if ( xyzCount == xyzLimit ) {
                Com_Error(ERR_DROP, "Winding point limit (%i) exceeded on brush face", xyzLimit);
#if defined(__ANDROID__)
                std::abort();
#endif
            }
            v6 = CM_XYZ_ROW(xyz,xyzCount);
            v7 = &pts[ptIndex];
            *v6 = v7->xyz[0];
            v6[1] = v7->xyz[1];
            v6[2] = v7->xyz[2];
            ++xyzCount;
        }
    }
    return xyzCount;
}

int __cdecl CM_PointInList(const float *point, const float (*xyzList)[3], int xyzCount)
{
    int xyzIndex; // [esp+8h] [ebp-4h]

#if defined(__ANDROID__)
    CollisionRequire(point && xyzList && xyzCount>=0 && xyzCount<=1024,"collision point comparison ownership/count");
#endif
    for ( xyzIndex = 0; xyzIndex < xyzCount; ++xyzIndex )
    {
        if ( VecNCompareCustomEpsilon(CM_XYZ_ROW(xyzList,xyzIndex), point, 0.1, 3) )
            return 1;
    }
    return 0;
}

void __cdecl CM_PickProjectionAxes(const float *normal, int *i, int *j)
{
    int k; // [esp+18h] [ebp-4h]

#if defined(__ANDROID__)
    CollisionRequire(normal && i && j && std::isfinite(normal[0]) && std::isfinite(normal[1]) && std::isfinite(normal[2]),"finite projection normal and axes");
    k = fabs(normal[1]) > fabs(*normal);
    if (fabs(normal[2]) > fabs(normal[k])) k=2;
#else
    k = fabs((unsigned int)normal[1]) > fabs(*normal);
    if ( fabs((unsigned int)normal[2]) > fabs(normal[k]) )
        LOBYTE(k) = 2;
#endif
    *i = (k & 1) == 0;
    *j = ~(_BYTE)k & 2;
}

void __cdecl CM_AddExteriorPointToWindingProjected(CMShowCollisionWinding *w, float *pt, int i, int j)
{
    float *v4; // [esp+0h] [ebp-18h]
    int indexPrev; // [esp+4h] [ebp-14h]
    int bestIndex; // [esp+8h] [ebp-10h]
    int index; // [esp+Ch] [ebp-Ch]
    float signedArea; // [esp+10h] [ebp-8h]
    float bestSignedArea; // [esp+14h] [ebp-4h]

#if defined(__ANDROID__)
    CollisionRequire(w && w->numpoints>=2 && w->numpoints<=1024 && pt && i>=0 && i<3 && j>=0 && j<3 && i!=j,"projected winding ownership/count/axes");
#endif
    bestIndex = -1;
    bestSignedArea = FLT_MAX;
    indexPrev = w->numpoints - 1;
    for ( index = 0; index < w->numpoints; ++index )
    {
        signedArea = CM_SignedAreaForPointsProjected(CM_COLLISION_POINT(w,indexPrev), pt, CM_COLLISION_POINT(w,index), i, j);
        if ( bestSignedArea > signedArea )
        {
            bestSignedArea = signedArea;
            bestIndex = index;
        }
        indexPrev = index;
    }
    if ( bestIndex < 0
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\qcommon\\cm_showcollision.cpp",
                    286,
                    0,
                    "%s",
                    "bestIndex >= 0") )
    {
        __debugbreak();
    }
    if ( bestSignedArea < -0.001 )
    {
#if defined(__ANDROID__)
        CollisionRequire(w->numpoints<1024,"collision winding growth within owned capacity");
#endif
        memmove((unsigned __int8 *)CM_COLLISION_POINT(w,bestIndex + 1), (unsigned __int8 *)CM_COLLISION_POINT(w,bestIndex), 12 * (w->numpoints - bestIndex));
        v4 = CM_COLLISION_POINT(w,bestIndex);
        *v4 = *pt;
        v4[1] = pt[1];
        v4[2] = pt[2];
        ++w->numpoints;
    }
    else if ( bestSignedArea <= 0.001 )
    {
        CM_AddColinearExteriorPointToWindingProjected(w, pt, i, j, (bestIndex + w->numpoints - 1) % w->numpoints, bestIndex);
    }
}

double __cdecl CM_SignedAreaForPointsProjected(const float *pt0, const float *pt1, const float *pt2, int i, int j)
{
#if defined(__ANDROID__)
    CollisionRequire(pt0 && pt1 && pt2 && i>=0 && i<3 && j>=0 && j<3 && i!=j,"projected triangle points/axes");
#endif
    return (pt2[j] - pt1[j]) * pt0[i] + (pt0[j] - pt2[j]) * pt1[i] + (pt1[j] - pt0[j]) * pt2[i];
}

void __cdecl CM_AddColinearExteriorPointToWindingProjected(
                CMShowCollisionWinding *w,
                float *pt,
                int i,
                int j,
                int index0,
                int index1)
{
    float *v6; // [esp+4h] [ebp-24h]
    float *v7; // [esp+8h] [ebp-20h]
    float *v8; // [esp+Ch] [ebp-1Ch]
    float *v9; // [esp+10h] [ebp-18h]
    float delta; // [esp+18h] [ebp-10h]
    int axis; // [esp+24h] [ebp-4h]

#if defined(__ANDROID__)
    CollisionRequire(w && w->numpoints>=2 && w->numpoints<=1024 && pt && index0>=0 && index0<w->numpoints && index1>=0 && index1<w->numpoints && i>=0 && i<3 && j>=0 && j<3 && i!=j,"colinear winding indices/axes");
#endif
    if ( CM_COLLISION_POINT(w,index0)[i] == CM_COLLISION_POINT(w,index1)[i]
        && CM_COLLISION_POINT(w,index0)[j] == CM_COLLISION_POINT(w,index1)[j]
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\qcommon\\cm_showcollision.cpp",
                    232,
                    0,
                    "%s",
                    "w->p[index0][i] != w->p[index1][i] || w->p[index0][j] != w->p[index1][j]") )
    {
        __debugbreak();
    }
    if ( fabs(CM_COLLISION_POINT(w,index1)[i] - CM_COLLISION_POINT(w,index0)[i]) < fabs(CM_COLLISION_POINT(w,index1)[j] - CM_COLLISION_POINT(w,index0)[j]) )
    {
        axis = j;
        delta = CM_COLLISION_POINT(w,index1)[j] - CM_COLLISION_POINT(w,index0)[j];
    }
    else
    {
        axis = i;
        delta = CM_COLLISION_POINT(w,index1)[i] - CM_COLLISION_POINT(w,index0)[i];
    }
    if ( delta <= 0.0 )
    {
        if ( CM_COLLISION_POINT(w,index0)[axis] <= CM_COLLISION_POINT(w,index1)[axis]
            && !Assert_MyHandler(
                        "C:\\projects_pc\\cod\\codsrc\\src\\qcommon\\cm_showcollision.cpp",
                        257,
                        0,
                        "%s",
                        "w->p[index0][axis] > w->p[index1][axis]") )
        {
            __debugbreak();
        }
        if ( pt[axis] <= CM_COLLISION_POINT(w,index0)[axis] )
        {
            if ( CM_COLLISION_POINT(w,index1)[axis] > pt[axis] )
            {
                v6 = CM_COLLISION_POINT(w,index1);
                *v6 = *pt;
                v6[1] = pt[1];
                v6[2] = pt[2];
            }
        }
        else
        {
            v7 = CM_COLLISION_POINT(w,index0);
            *v7 = *pt;
            v7[1] = pt[1];
            v7[2] = pt[2];
        }
    }
    else
    {
        if ( CM_COLLISION_POINT(w,index1)[axis] <= CM_COLLISION_POINT(w,index0)[axis]
            && !Assert_MyHandler(
                        "C:\\projects_pc\\cod\\codsrc\\src\\qcommon\\cm_showcollision.cpp",
                        249,
                        0,
                        "%s",
                        "w->p[index0][axis] < w->p[index1][axis]") )
        {
            __debugbreak();
        }
        if ( CM_COLLISION_POINT(w,index0)[axis] <= pt[axis] )
        {
            if ( pt[axis] > CM_COLLISION_POINT(w,index1)[axis] )
            {
                v8 = CM_COLLISION_POINT(w,index1);
                *v8 = *pt;
                v8[1] = pt[1];
                v8[2] = pt[2];
            }
        }
        else
        {
            v9 = CM_COLLISION_POINT(w,index0);
            *v9 = *pt;
            v9[1] = pt[1];
            v9[2] = pt[2];
        }
    }
}

double __cdecl CM_RepresentativeTriangleFromWinding(const CMShowCollisionWinding *w, const float *normal, int *i0, int *i1, int *i2)
{
    int j; // [esp+14h] [ebp-38h]
    float areaBest; // [esp+18h] [ebp-34h]
    float vb[3]; // [esp+20h] [ebp-2Ch] BYREF
    int k; // [esp+2Ch] [ebp-20h]
    float vc[3]; // [esp+30h] [ebp-1Ch] BYREF
    float va[3]; // [esp+3Ch] [ebp-10h] BYREF
    int i; // [esp+48h] [ebp-4h]

#if defined(__ANDROID__)
    CollisionRequire(w && w->numpoints>=2 && w->numpoints<=1024 && normal && i0 && i1 && i2,"representative winding ownership/count");
#endif
    *i0 = 0;
    *i1 = 1;
    *i2 = 2;
    areaBest = 0.0f;
    for ( k = 2; k < w->numpoints; ++k )
    {
        for ( j = 1; j < k; ++j )
        {
            vb[0] = CM_COLLISION_POINT(w,k)[0] - CM_COLLISION_POINT(w,j)[0];
            vb[1] = CM_COLLISION_POINT(w,k)[1] - CM_COLLISION_POINT(w,j)[1];
            vb[2] = CM_COLLISION_POINT(w,k)[2] - CM_COLLISION_POINT(w,j)[2];
            for ( i = 0; i < j; ++i )
            {
                va[0] = CM_COLLISION_POINT(w,i)[0] - CM_COLLISION_POINT(w,j)[0];
                va[1] = CM_COLLISION_POINT(w,i)[1] - CM_COLLISION_POINT(w,j)[1];
                va[2] = CM_COLLISION_POINT(w,i)[2] - CM_COLLISION_POINT(w,j)[2];
                Vec3Cross(va, vb, vc);
                if ( fabs((float)((float)(vc[0] * *normal) + (float)(vc[1] * normal[1])) + (float)(vc[2] * normal[2])) > areaBest )
                {
#if defined(__ANDROID__)
                    // Magnitude selects a representative; orientation is handled below.
                    areaBest = fabs((float)((float)(vc[0] * *normal) + (float)(vc[1] * normal[1])) + (float)(vc[2] * normal[2]));
#else
                    areaBest = -((float)((float)(vc[0] * *normal) + (float)(vc[1] * normal[1])) + (float)(vc[2] * normal[2]));
#endif
                    *i0 = i;
                    *i1 = j;
                    *i2 = k;
                }
            }
        }
    }
    return areaBest;
}

void __cdecl CM_ReverseWinding(CMShowCollisionWinding *w)
{
    float *v1; // ecx
    float *v2; // [esp+4h] [ebp-1Ch]
    float *v3; // [esp+8h] [ebp-18h]
    float pt; // [esp+10h] [ebp-10h]
    float pt_4; // [esp+14h] [ebp-Ch]
    float pt_8; // [esp+18h] [ebp-8h]
    int i; // [esp+1Ch] [ebp-4h]

#if defined(__ANDROID__)
    CollisionRequire(w && w->numpoints>=0 && w->numpoints<=1024,"reverse winding ownership/count");
#endif
    for ( i = 0; i < w->numpoints / 2; ++i )
    {
        pt = CM_COLLISION_POINT(w,i)[0];
        pt_4 = CM_COLLISION_POINT(w,i)[1];
        pt_8 = CM_COLLISION_POINT(w,i)[2];
        v2 = CM_COLLISION_POINT(w,i);
        v3 = CM_COLLISION_POINT(w,w->numpoints - 1 - i);
        *v2 = *v3;
        v2[1] = v3[1];
        v2[2] = v3[2];
        v1 = CM_COLLISION_POINT(w,w->numpoints - 1 - i);
        *v1 = pt;
        v1[1] = pt_4;
        v1[2] = pt_8;
    }
}

char __cdecl CM_BrushInView(const cbrush_t *brush, cplane_s *frustumPlanes, int frustumPlaneCount)
{
    int frustumPlaneIndex; // [esp+0h] [ebp-4h]

    if (!frustumPlanes
        && !Assert_MyHandler(
            "C:\\projects_pc\\cod\\codsrc\\src\\qcommon\\cm_showcollision.cpp",
            49,
            0,
            "%s",
            "frustumPlanes"))
    {
        __debugbreak();
    }
    for (frustumPlaneIndex = 0; frustumPlaneIndex < frustumPlaneCount; ++frustumPlaneIndex)
    {
        if ((BoxOnPlaneSide(brush->mins, brush->maxs, &frustumPlanes[frustumPlaneIndex]) & 1) == 0)
            return 0;
    }
    return 1;
}

void __cdecl CM_ShowBrushCollision(
                int contentMask,
                cplane_s *frustumPlanes,
                int frustumPlaneCount,
                void (__cdecl *drawCollisionPoly)(int, float (*)[3], const float *))
{
    float colorFloat[4]; // [esp+4h] [ebp-1Ch] BYREF
    int brushIndex; // [esp+14h] [ebp-Ch]
    cbrush_t *brush; // [esp+18h] [ebp-8h]
    int colorCounter; // [esp+1Ch] [ebp-4h]

    if ( !frustumPlanes
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\qcommon\\cm_showcollision.cpp",
                    435,
                    0,
                    "%s",
                    "frustumPlanes") )
    {
        __debugbreak();
    }
    colorCounter = 0;
    for ( brushIndex = 0; brushIndex < cm.numBrushes; ++brushIndex )
    {
        brush = &cm.brushes[brushIndex];
        if ( (contentMask & brush->contents) != 0 )
        {
            CM_GetShowCollisionColor(colorFloat, colorCounter++);
            colorCounter %= 8;
            if ( CM_BrushInView(brush, frustumPlanes, frustumPlaneCount) )
                CM_ShowSingleBrushCollision(brush, colorFloat, drawCollisionPoly);
        }
    }
}

void __cdecl CM_GetShowCollisionColor(float *colorFloat, char colorCounter)
{
    float v2; // [esp+0h] [ebp-Ch]
    float v3; // [esp+4h] [ebp-8h]
    float v4; // [esp+8h] [ebp-4h]

    if ( !colorFloat
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\qcommon\\cm_showcollision.cpp", 36, 0, "%s", "colorFloat") )
    {
        __debugbreak();
    }
    if ( (colorCounter & 1) != 0 )
        v4 = 1.0f;
    else
        v4 = 0.0f;
    *colorFloat = v4;
    if ( (colorCounter & 2) != 0 )
        v3 = 1.0f;
    else
        v3 = 0.0f;
    colorFloat[1] = v3;
    if ( (colorCounter & 4) != 0 )
        v2 = 1.0f;
    else
        v2 = 0.0f;
    colorFloat[2] = v2;
    colorFloat[3] = 0.5f;
}

