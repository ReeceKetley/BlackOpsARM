#include "r_model_skin.h"

#if defined(__ANDROID__) && defined(__aarch64__)
// ---------------------------------------------------------------------------
// NEON port of the SSE/MMX skinner below (same maths and output layout).
//
// A GfxPackedVertex is two 16-byte lanes:
//   [0] = xyz + binormalSign          (position, w = binormal sign)
//   [1] = color | texCoord | normal | tangent   (the last two are PackedUnitVec)
// A bone (DObjSkelMat) is four 16-byte rows: axis[0..2] (rotation) + origin.
// vertexBlend holds, per vertex: bone0, then (bone_i, weight_i) pairs (byte
// offsets into the bone array and uint16 weights), so the stride for N
// weights is 2*N + 1.
//
// Packing matches cvt_ps2pi + two packuswb passes: round to nearest even and
// clamp to [0, 255] (vcvtnq_s32_f32, vqmovun_s32, vqmovn_u16).
// ---------------------------------------------------------------------------
#include <universal/q_shared.h>
#include "r_model_skin.h"
#include <xanim/xanim.h>
#include <universal/profile.h>
#include <arm_neon.h>
#include <cstdint>
#include <cstring>

namespace {
const float32x4_t kEncodeShift = {127.0f, 127.0f, 127.0f, -192.0f};
const float32x4_t kEncodeScale = {127.0f, 127.0f, 127.0f, 255.0f};

inline const DObjSkelMat *BoneAt(const DObjSkelMat *boneMatrix, unsigned byteOffset)
{
    return (const DObjSkelMat *)((const char *)boneMatrix + byteOffset);
}

// [Black Ops] position (x, y, z, +/-1): keep only the sign bit of w, OR in 1.0
inline float32x4_t LoadSkinPosition(float32x4_t p)
{
    const uint32_t sign = vgetq_lane_u32(vreinterpretq_u32_f32(p), 3) & 0x80000000u;
    return vreinterpretq_f32_u32(vsetq_lane_u32(sign | 0x3F800000u, vreinterpretq_u32_f32(p), 3));
}

// affine transform (rotation + translation), summed as the SSE version
inline float32x4_t TransformPoint(const DObjSkelMat *m, float32x4_t p)
{
    const float32x4_t a = vaddq_f32(vmulq_laneq_f32(vld1q_f32(m->axis[0]), p, 0), vmulq_laneq_f32(vld1q_f32(m->axis[1]), p, 1));
    const float32x4_t b = vaddq_f32(vmulq_laneq_f32(vld1q_f32(m->axis[2]), p, 2), vld1q_f32(m->origin));
    return vaddq_f32(a, b);
}

// linear transform (rotation only)
inline float32x4_t TransformDir(const DObjSkelMat *m, float32x4_t d)
{
    const float32x4_t a = vaddq_f32(vmulq_laneq_f32(vld1q_f32(m->axis[0]), d, 0), vmulq_laneq_f32(vld1q_f32(m->axis[1]), d, 1));
    return vaddq_f32(a, vmulq_laneq_f32(vld1q_f32(m->axis[2]), d, 2));
}

// transformed xyz with w taken from wSource
inline float32x4_t PackXyzW(float32x4_t xyz, float32x4_t wSource)
{
    return vcopyq_laneq_f32(xyz, 3, wSource, 3);
}

// byte-packed unit vector -> direction scaled by its packed length
inline float32x4_t DecodeUnitVec(uint32_t packed)
{
    const uint16x4_t bytes = vget_low_u16(vmovl_u8(vcreate_u8(packed)));
    const float32x4_t d = vdivq_f32(vsubq_f32(vcvtq_f32_u32(vmovl_u16(bytes)), kEncodeShift), kEncodeScale);
    return vmulq_laneq_f32(d, d, 3);
}

// transform a packed unit vector by a bone's rotation and re-encode it
inline float32x4_t SkinUnitVec(const DObjSkelMat *m, uint32_t packed)
{
    const float32x4_t transformed = PackXyzW(TransformDir(m, DecodeUnitVec(packed)), vld1q_f32(m->origin));
    return vaddq_f32(vmulq_f32(transformed, kEncodeScale), kEncodeShift);
}

// weight (uint16) -> [0, 1)
inline float32x4_t DecodeWeight(uint16_t weight)
{
    return vdupq_n_f32(float(weight) * (1.0f / 65536.0f));
}

// encoded normal & tangent (float4 each) -> 8 bytes
inline uint64_t PackNormalTangent(float32x4_t encNormal, float32x4_t encTangent)
{
    const uint16x8_t words = vcombine_u16(vqmovun_s32(vcvtnq_s32_f32(encNormal)), vqmovun_s32(vcvtnq_s32_f32(encTangent)));
    return vget_lane_u64(vreinterpret_u64_u8(vqmovn_u16(words)), 0);
}

inline uint32_t Word(const GfxPackedVertex *v, unsigned index)
{
    uint32_t w;
    std::memcpy(&w, (const char *)v + 4 * index, 4);
    return w;
}

inline void StoreVertex(GfxPackedVertex *dst, float32x4_t position, const GfxPackedVertex *src, uint64_t normalTangent)
{
    vst1q_f32((float *)dst, position);
    std::memcpy((char *)dst + 16, (const char *)src + 16, 8); // color + texCoord
    std::memcpy((char *)dst + 24, &normalTangent, 8);
}

template <int numWeights>
inline float32x4_t BlendPosition(const GfxPackedVertex *src, const uint16_t *blend, const DObjSkelMat *boneMatrix,
    const DObjSkelMat **bone0Out)
{
    const float32x4_t position = LoadSkinPosition(vld1q_f32((const float *)src));
    const DObjSkelMat *bone0 = BoneAt(boneMatrix, blend[0]);
    const float32x4_t pos0 = PackXyzW(TransformPoint(bone0, position), position);
    float32x4_t outPos = pos0;
    for (int w = 1; w <= numWeights; ++w)
    {
        const DObjSkelMat *bone = BoneAt(boneMatrix, blend[2 * w - 1]);
        const float32x4_t bonePos = PackXyzW(TransformPoint(bone, position), position);
        outPos = vaddq_f32(outPos, vmulq_f32(DecodeWeight(blend[2 * w]), vsubq_f32(bonePos, pos0)));
    }
    *bone0Out = bone0;
    return outPos;
}

template <int numWeights>
void SkinWeightBlock(const GfxPackedVertex *srcVerts, const uint16_t *vertexBlend, int vertCount,
    const DObjSkelMat *boneMatrix, GfxPackedVertex *dstVerts, GfxPackedVertexNormal *dstVertNormals, int *pVertexIndex)
{
    int vertIndex = *pVertexIndex;
    for (int i = 0; i < vertCount; ++i)
    {
        const GfxPackedVertex *src = &srcVerts[vertIndex];
        const DObjSkelMat *bone0;
        const float32x4_t outPos = BlendPosition<numWeights>(src, vertexBlend, boneMatrix, &bone0);
        const uint64_t normalTangent = PackNormalTangent(SkinUnitVec(bone0, Word(src, 6)), SkinUnitVec(bone0, Word(src, 7)));
        StoreVertex(&dstVerts[vertIndex], outPos, src, normalTangent);
        if (dstVertNormals)
            std::memcpy(&dstVertNormals[vertIndex], &normalTangent, 8);
        vertexBlend += 2 * numWeights + 1;
        ++vertIndex;
    }
    *pVertexIndex = vertIndex;
}

template <int numWeights>
void SkinWeightBlockInOut(const GfxPackedVertex *srcVerts, const uint16_t *vertexBlend, int vertCount,
    const DObjSkelMat *boneMatrix, const GfxPackedVertexNormal *srcVertNormals,
    GfxPackedVertexNormal *dstVertNormals, GfxPackedVertex *dstVerts, int *pVertexIndex)
{
    int vertIndex = *pVertexIndex;
    for (int i = 0; i < vertCount; ++i)
    {
        const GfxPackedVertex *src = &srcVerts[vertIndex];
        const DObjSkelMat *bone0;
        const float32x4_t outPos = BlendPosition<numWeights>(src, vertexBlend, boneMatrix, &bone0);
        uint64_t normalTangent;
        std::memcpy(&normalTangent, &srcVertNormals[vertIndex], 8);
        StoreVertex(&dstVerts[vertIndex], outPos, src, normalTangent);
        std::memcpy(&dstVertNormals[vertIndex], &normalTangent, 8);
        vertexBlend += 2 * numWeights + 1;
        ++vertIndex;
    }
    *pVertexIndex = vertIndex;
}

void SkinWeight(const GfxPackedVertex *inVerts, const XSurfaceVertexInfo *vertexInfo, const DObjSkelMat *boneMatrix,
    GfxPackedVertex *outVerts, const GfxPackedVertexNormal *inNormals, GfxPackedVertexNormal *outNormals)
{
    PROF_SCOPED("SkinXSurfaceWeight");

    int vertIndex = 0;
    const uint16_t *blend = vertexInfo->vertsBlend;
    for (int weights = 0; weights < 4; ++weights)
    {
        const int count = vertexInfo->vertCount[weights];
        if (!count)
            continue;
        switch (weights)
        {
        case 0: inNormals ? SkinWeightBlockInOut<0>(inVerts, blend, count, boneMatrix, inNormals, outNormals, outVerts, &vertIndex)
                          : SkinWeightBlock<0>(inVerts, blend, count, boneMatrix, outVerts, outNormals, &vertIndex); break;
        case 1: inNormals ? SkinWeightBlockInOut<1>(inVerts, blend, count, boneMatrix, inNormals, outNormals, outVerts, &vertIndex)
                          : SkinWeightBlock<1>(inVerts, blend, count, boneMatrix, outVerts, outNormals, &vertIndex); break;
        case 2: inNormals ? SkinWeightBlockInOut<2>(inVerts, blend, count, boneMatrix, inNormals, outNormals, outVerts, &vertIndex)
                          : SkinWeightBlock<2>(inVerts, blend, count, boneMatrix, outVerts, outNormals, &vertIndex); break;
        default: inNormals ? SkinWeightBlockInOut<3>(inVerts, blend, count, boneMatrix, inNormals, outNormals, outVerts, &vertIndex)
                           : SkinWeightBlock<3>(inVerts, blend, count, boneMatrix, outVerts, outNormals, &vertIndex); break;
        }
        blend += (2 * weights + 1) * count;
    }
}

// Rigid: one bone per vertex list, no blend; raw binormal sign (as retail).
void SkinRigid(const XSurface *surf, const DObjSkelMat *boneMatrix, GfxPackedVertex *dstVerts,
    const GfxPackedVertexNormal *inNormals, GfxPackedVertexNormal *outNormals)
{
    PROF_SCOPED("SkinXSurfaceRigid");

    const GfxPackedVertex *src = (const GfxPackedVertex *)surf->verts0;
    GfxPackedVertex *dst = dstVerts;
    for (unsigned list = 0; list < surf->vertListCount; ++list)
    {
        const XRigidVertList *vertList = &surf->vertList[list];
        const DObjSkelMat *bone = BoneAt(boneMatrix, vertList->boneOffset);
        for (int i = 0; i < vertList->vertCount; ++i)
        {
            __builtin_prefetch(&src[4]);
            const float32x4_t srcPos = vld1q_f32((const float *)src);
            const float32x4_t outPos = PackXyzW(TransformPoint(bone, srcPos), srcPos);
            uint64_t normalTangent;
            if (inNormals)
                std::memcpy(&normalTangent, inNormals++, 8);
            else
                normalTangent = PackNormalTangent(SkinUnitVec(bone, Word(src, 6)), SkinUnitVec(bone, Word(src, 7)));
            StoreVertex(dst, outPos, src, normalTangent);
            if (outNormals)
                std::memcpy(outNormals++, &normalTangent, 8);
            ++src;
            ++dst;
        }
    }
    iassert(dst - dstVerts == surf->vertCount);
}
}

void __cdecl R_SkinXSurfaceSkinnedSse(const XSurface *xsurf, const DObjSkelMat *boneMatrix,
    GfxPackedVertexNormal *skinVertNormalIn, GfxPackedVertexNormal *skinVertNormalOut,
    GfxPackedVertex *skinVerticesOut)
{
    iassert(skinVertNormalOut || !skinVertNormalIn);
    if ((xsurf->flags & 0x80) != 0)
        SkinWeight(xsurf->verts0, &xsurf->vertInfo, boneMatrix, skinVerticesOut, skinVertNormalOut ? skinVertNormalIn : nullptr, skinVertNormalOut);
    else
        SkinRigid(xsurf, boneMatrix, skinVerticesOut, skinVertNormalOut ? skinVertNormalIn : nullptr, skinVertNormalOut);
}
#elif defined(BO1_PORT_X64) || defined(_M_X64) || defined(_WIN64)
// BO1_PORT_X64_SSE_STUB: MMX skinning is x86-only; fall back to scalar.
void __cdecl R_SkinXSurfaceSkinnedSse(
    const XSurface *xsurf,
    const DObjSkelMat *boneMatrix,
    GfxPackedVertexNormal *skinVertNormalIn,
    GfxPackedVertexNormal *skinVertNormalOut,
    GfxPackedVertex *skinVerticesOut)
{
    (void)skinVertNormalIn;
    (void)skinVertNormalOut;
    R_SkinXSurfaceSkinned(xsurf, boneMatrix, skinVerticesOut);
}
#else
// (Aislop) - (from BO1Cod4)
// shared between KCOD4 and KBLOPS.
// (COD4) Before ~(580us) -- After ~(25us).
// (BLOPS) Before ~(85us) -- After ~(45us).
// 
// BO1TODO: if someone wants to they could take some time, sit down, and make this even better with modern AVX

// This file uses MMX (__m64), which aliases the x87 register stack. EMMS is issued
// once per skinning batch (see _mm_empty() in the R_SkinXSurface* entry points), not
// per inline helper. Disabled before the intrinsic headers so it also covers the
// inline MMX intrinsics defined there (e.g. _mm_cvtpu16_ps in <xmmintrin.h>).
#pragma warning(disable : 4799)

#include <universal/q_shared.h>
#include "r_model_skin.h"

#include <xanim/xanim.h>
#include <universal/profile.h>

#include <xmmintrin.h>
#include <mmintrin.h>

// ---------------------------------------------------------------------------
// SSE / MMX vertex skinning.
//
// A GfxPackedVertex is two 16-byte lanes:
//   [0] = xyz + binormalSign          (position, w = binormal sign)
//   [1] = color | texCoord | normal | tangent   (the last two are PackedUnitVec)
// A bone (DObjSkelMat) is four 16-byte rows: axis[0..2] (rotation) + origin.
//
// vertexBlend holds, per vertex: bone0, then (bone_i, weight_i) pairs, so the
// stride for N weights is 2*N + 1.
//
// Black Ops upgrade (weighted path only): the output position's w is rebuilt to
// a clean +/-1 binormal sign via LoadSkinPosition() instead of trusting the raw
// packed value. See mask/one constants below. Rigid path matches retail (raw w).
// ---------------------------------------------------------------------------

// blend weights are uint16 in [0, 65535]; 1/65536 normalizes them to [0, 1)
static const __m128 sse_weightScale = { { 1.0f / 65536.0f, 1.0f / 65536.0f, 1.0f / 65536.0f, 1.0f / 65536.0f } };

// packed unit vectors decode as (packed - shift) / scale, encode as packed * scale + shift
static const __m128 sse_encodeShift = { { 127.0f, 127.0f, 127.0f, -192.0f } };
static const __m128 sse_encodeScale = { { 127.0f, 127.0f, 127.0f, 255.0f } };

// Black Ops binormal-sign normalization: keep xyz fully, keep only the sign bit of w, then OR in 1.0
static const __declspec(align(16)) unsigned int k_binormalSignMask[4] = { 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0x80000000u };
static const __m128 sse_wOne = { { 0.0f, 0.0f, 0.0f, 1.0f } };

// ---------------------------------------------------------------------------
// intrinsic helpers
// ---------------------------------------------------------------------------

static __forceinline __m128 SplatX(__m128 v) { return _mm_shuffle_ps(v, v, 0x00); }
static __forceinline __m128 SplatY(__m128 v) { return _mm_shuffle_ps(v, v, 0x55); }
static __forceinline __m128 SplatZ(__m128 v) { return _mm_shuffle_ps(v, v, 0xAA); }
static __forceinline __m128 SplatW(__m128 v) { return _mm_shuffle_ps(v, v, 0xFF); }

// vertexBlend entries are pre-scaled byte offsets into the bone-matrix array
static __forceinline const DObjSkelMat *BoneAt(const DObjSkelMat *boneMatrix, unsigned byteOffset)
{
    return (const DObjSkelMat *)((const char *)boneMatrix + byteOffset);
}

// [Black Ops] rebuild an affine position (x, y, z, +/-1) with a clean binormal sign
static __forceinline __m128 LoadSkinPosition(__m128 xyzAndBinormalSign)
{
    __m128 signMask = _mm_load_ps((const float *)k_binormalSignMask);
    __m128 hi = _mm_unpackhi_ps(xyzAndBinormalSign, xyzAndBinormalSign);   // (z, z, sign, sign)
    __m128 wOne = _mm_or_ps(_mm_and_ps(signMask, hi), sse_wOne);           // w = +/-1.0
    return _mm_shuffle_ps(xyzAndBinormalSign, wOne, 0xC4);                 // (x, y, z, +/-1)
}

// affine transform (rotation + translation)
static __forceinline __m128 TransformPoint(const DObjSkelMat *m, __m128 p)
{
    return _mm_add_ps(
        _mm_add_ps(_mm_mul_ps(_mm_load_ps(m->axis[0]), SplatX(p)),
            _mm_mul_ps(_mm_load_ps(m->axis[1]), SplatY(p))),
        _mm_add_ps(_mm_mul_ps(_mm_load_ps(m->axis[2]), SplatZ(p)),
            _mm_load_ps(m->origin)));
}

// linear transform (rotation only)
static __forceinline __m128 TransformDir(const DObjSkelMat *m, __m128 d)
{
    return _mm_add_ps(
        _mm_add_ps(_mm_mul_ps(_mm_load_ps(m->axis[0]), SplatX(d)),
            _mm_mul_ps(_mm_load_ps(m->axis[1]), SplatY(d))),
        _mm_mul_ps(_mm_load_ps(m->axis[2]), SplatZ(d)));
}

// repack transformed xyz, taking w from wSource
static __forceinline __m128 PackXyzW(__m128 xyz, __m128 wSource)
{
    return _mm_shuffle_ps(xyz, _mm_unpackhi_ps(xyz, wSource), 0xC4);
}

// decode a byte-packed unit vector to a direction, scaled by its packed length
static __forceinline __m128 DecodeUnitVec(uint32_t packed)
{
    __m64 bytes = _m_punpcklbw(_mm_cvtsi32_si64(packed), _mm_setzero_si64());
    __m128 d = _mm_div_ps(_mm_sub_ps(_mm_cvtpu16_ps(bytes), sse_encodeShift), sse_encodeScale);
    return _mm_mul_ps(d, SplatW(d));
}

// transform a packed unit vector by a bone's rotation and re-encode it (as floats ready to pack)
static __forceinline __m128 SkinUnitVec(const DObjSkelMat *m, uint32_t packed)
{
    __m128 transformed = TransformDir(m, DecodeUnitVec(packed));
    return _mm_add_ps(_mm_mul_ps(PackXyzW(transformed, _mm_load_ps(m->origin)), sse_encodeScale), sse_encodeShift);
}

// weight (uint16) -> [0, 1), broadcast to all 4 lanes
static __forceinline __m128 DecodeWeight(uint16_t weight)
{
    __m64 w = _mm_cvtsi32_si64(weight);
    w = _m_punpcklwd(w, w);
    return _mm_mul_ps(_mm_cvtpu16_ps(_m_punpcklwd(w, w)), sse_weightScale);
}

// pack an encoded normal & tangent (each a float4) into 8 output bytes
static __forceinline __m64 PackNormalTangent(__m128 encNormal, __m128 encTangent)
{
    return _m_packuswb(
        _m_packuswb(_mm_cvt_ps2pi(encNormal), _mm_cvt_ps2pi(_mm_movehl_ps(encNormal, encNormal))),
        _m_packuswb(_mm_cvt_ps2pi(encTangent), _mm_cvt_ps2pi(_mm_movehl_ps(encTangent, encTangent))));
}

// ---------------------------------------------------------------------------
// per-vertex skinners (templated on the number of extra blend weights)
// ---------------------------------------------------------------------------

// Full skin: blend the position across (1 + numWeights) bones; transform the
// normal & tangent by the primary bone. Returns the packed normal/tangent so the
// caller can also emit it to a separate normal stream. [Black Ops binormal fix]
template <int numWeights>
static __forceinline __m64 Sse_SkinVertex(const GfxPackedVertex *src, const uint16_t *blend,
    const DObjSkelMat *boneMatrix, __m64 *dst)
{
    const __m128 *v = (const __m128 *)src;
    __m128 position = LoadSkinPosition(v[0]);
    __m64 colorTexCoord = *(const __m64 *) & v[1].m128_u64[0];

    const DObjSkelMat *bone0 = BoneAt(boneMatrix, blend[0]);
    __m128 pos0 = PackXyzW(TransformPoint(bone0, position), position);
    __m128 outPos = pos0;
    for (int w = 1; w <= numWeights; ++w)
    {
        const DObjSkelMat *bone = BoneAt(boneMatrix, blend[2 * w - 1]);
        __m128 bonePos = PackXyzW(TransformPoint(bone, position), position);
        outPos = _mm_add_ps(outPos, _mm_mul_ps(DecodeWeight(blend[2 * w]), _mm_sub_ps(bonePos, pos0)));
    }

    __m64 normalTangent = PackNormalTangent(SkinUnitVec(bone0, v[1].m128_u32[2]),
        SkinUnitVec(bone0, v[1].m128_u32[3]));
    _mm_stream_ps((float *)dst, outPos);
    _mm_stream_pi(dst + 2, colorTexCoord);
    _mm_stream_pi(dst + 3, normalTangent);
    return normalTangent;
}

// Simple skin: blend the position only; the normal/tangent are supplied by the
// caller (a separate precomputed normal stream) and passed straight through.
template <int numWeights>
static __forceinline void Sse_SkinVertexSimple(const GfxPackedVertex *src, const uint16_t *blend,
    const DObjSkelMat *boneMatrix, __m64 *dst,
    __m64 normalTangent, GfxPackedVertexNormal *dstNormal)
{
    const __m128 *v = (const __m128 *)src;
    __m128 position = LoadSkinPosition(v[0]);
    __m64 colorTexCoord = *(const __m64 *) & v[1].m128_u64[0];

    const DObjSkelMat *bone0 = BoneAt(boneMatrix, blend[0]);
    __m128 pos0 = PackXyzW(TransformPoint(bone0, position), position);
    __m128 outPos = pos0;
    for (int w = 1; w <= numWeights; ++w)
    {
        const DObjSkelMat *bone = BoneAt(boneMatrix, blend[2 * w - 1]);
        __m128 bonePos = PackXyzW(TransformPoint(bone, position), position);
        outPos = _mm_add_ps(outPos, _mm_mul_ps(DecodeWeight(blend[2 * w]), _mm_sub_ps(bonePos, pos0)));
    }

    _mm_stream_ps((float *)dst, outPos);
    _mm_stream_pi(dst + 2, colorTexCoord);
    _mm_stream_pi(dst + 3, normalTangent);
    _mm_stream_pi((__m64 *)dstNormal, normalTangent);
}

// Rigid skin: single bone, no blend. Matches retail (raw binormal sign).
static __forceinline __m64 Sse_SkinVertexRigid(const DObjSkelMat *bone, const GfxPackedVertex *src, __m64 *dst)
{
    const __m128 *v = (const __m128 *)src;
    __m128 srcPos = v[0];
    __m64 colorTexCoord = *(const __m64 *) & v[1].m128_u64[0];

    __m128 outPos = PackXyzW(TransformPoint(bone, srcPos), srcPos);
    __m64 normalTangent = PackNormalTangent(SkinUnitVec(bone, v[1].m128_u32[2]),
        SkinUnitVec(bone, v[1].m128_u32[3]));
    _mm_stream_ps((float *)dst, outPos);
    _mm_stream_pi(dst + 2, colorTexCoord);
    _mm_stream_pi(dst + 3, normalTangent);
    return normalTangent;
}

static __forceinline void Sse_SkinVertexRigidSimple(const DObjSkelMat *bone, const GfxPackedVertex *src, __m64 *dst,
    __m64 normalTangent, GfxPackedVertexNormal *dstNormal)
{
    const __m128 *v = (const __m128 *)src;
    __m128 srcPos = v[0];
    __m64 colorTexCoord = *(const __m64 *) & v[1].m128_u64[0];

    __m128 outPos = PackXyzW(TransformPoint(bone, srcPos), srcPos);
    _mm_stream_ps((float *)dst, outPos);
    _mm_stream_pi(dst + 2, colorTexCoord);
    _mm_stream_pi(dst + 3, normalTangent);
    _mm_stream_pi((__m64 *)dstNormal, normalTangent);
}

// ---------------------------------------------------------------------------
// weighted block loops (templated on weight count)
// dstVertNormals == null: verts only; non-null: also emit the normal stream.
// ---------------------------------------------------------------------------

template <int numWeights>
static void SkinWeightBlock(const GfxPackedVertex *srcVerts, const uint16_t *vertexBlend, int vertCount,
    const DObjSkelMat *boneMatrix, GfxPackedVertex *dstVerts,
    GfxPackedVertexNormal *dstVertNormals, int *pVertexIndex)
{
    iassert(dstVerts);
    iassert(srcVerts);
    iassert(!(reinterpret_cast<uintptr_t>(dstVerts) & 15));
    iassert(!(reinterpret_cast<uintptr_t>(srcVerts) & 15));

    int vertIndex = *pVertexIndex;
    for (int i = 0; i < vertCount; ++i)
    {
        __m64 normalTangent = Sse_SkinVertex<numWeights>(&srcVerts[vertIndex], vertexBlend, boneMatrix,
            (__m64 *) & dstVerts[vertIndex]);
        if (dstVertNormals)
            _mm_stream_pi((__m64 *) & dstVertNormals[vertIndex], normalTangent);
        vertexBlend += 2 * numWeights + 1;
        ++vertIndex;
    }
    *pVertexIndex = vertIndex;
}

template <int numWeights>
static void SkinWeightBlockInOut(const GfxPackedVertex *srcVerts, const uint16_t *vertexBlend, int vertCount,
    const DObjSkelMat *boneMatrix, const GfxPackedVertexNormal *srcVertNormals,
    GfxPackedVertexNormal *dstVertNormals, GfxPackedVertex *dstVerts, int *pVertexIndex)
{
    iassert(dstVerts);
    iassert(dstVertNormals);
    iassert(srcVerts);
    iassert(srcVertNormals);
    iassert(!(reinterpret_cast<uintptr_t>(dstVerts) & 15));
    iassert(!(reinterpret_cast<uintptr_t>(srcVerts) & 15));

    int vertIndex = *pVertexIndex;
    for (int i = 0; i < vertCount; ++i)
    {
        __m64 normalTangent = *(const __m64 *) & srcVertNormals[vertIndex];
        Sse_SkinVertexSimple<numWeights>(&srcVerts[vertIndex], vertexBlend, boneMatrix,
            (__m64 *) & dstVerts[vertIndex], normalTangent, &dstVertNormals[vertIndex]);
        vertexBlend += 2 * numWeights + 1;
        ++vertIndex;
    }
    *pVertexIndex = vertIndex;
}

// ---------------------------------------------------------------------------
// weighted dispatchers: run each weight-count block over its slice
// ---------------------------------------------------------------------------

static void R_SkinXSurfaceWeightSse_Impl(const GfxPackedVertex *inVerts, const XSurfaceVertexInfo *vertexInfo,
    const DObjSkelMat *boneMatrix, GfxPackedVertex *outVerts,
    GfxPackedVertexNormal *outNormals)
{
    PROF_SCOPED("SkinXSurfaceWeight");

    int vertIndex = 0;
    const uint16_t *blend = vertexInfo->vertsBlend;
    if (vertexInfo->vertCount[0])
    {
        SkinWeightBlock<0>(inVerts, blend, vertexInfo->vertCount[0], boneMatrix, outVerts, outNormals, &vertIndex);
        blend += vertexInfo->vertCount[0];
    }
    if (vertexInfo->vertCount[1])
    {
        SkinWeightBlock<1>(inVerts, blend, vertexInfo->vertCount[1], boneMatrix, outVerts, outNormals, &vertIndex);
        blend += 3 * vertexInfo->vertCount[1];
    }
    if (vertexInfo->vertCount[2])
    {
        SkinWeightBlock<2>(inVerts, blend, vertexInfo->vertCount[2], boneMatrix, outVerts, outNormals, &vertIndex);
        blend += 5 * vertexInfo->vertCount[2];
    }
    if (vertexInfo->vertCount[3])
        SkinWeightBlock<3>(inVerts, blend, vertexInfo->vertCount[3], boneMatrix, outVerts, outNormals, &vertIndex);

    _mm_empty();
}

static void R_SkinXSurfaceWeightSse(const GfxPackedVertex *inVerts, const XSurfaceVertexInfo *vertexInfo,
    const DObjSkelMat *boneMatrix, GfxPackedVertex *outVerts)
{
    R_SkinXSurfaceWeightSse_Impl(inVerts, vertexInfo, boneMatrix, outVerts, nullptr);
}

static void R_SkinXSurfaceWeightSseOut(const GfxPackedVertex *inVerts, const XSurfaceVertexInfo *vertexInfo,
    const DObjSkelMat *boneMatrix, GfxPackedVertexNormal *outNormals,
    GfxPackedVertex *outVerts)
{
    R_SkinXSurfaceWeightSse_Impl(inVerts, vertexInfo, boneMatrix, outVerts, outNormals);
}

static void R_SkinXSurfaceWeightSseInOut(const GfxPackedVertex *inVerts, const XSurfaceVertexInfo *vertexInfo,
    const DObjSkelMat *boneMatrix, const GfxPackedVertexNormal *srcVertNormals,
    GfxPackedVertexNormal *dstVertNormals, GfxPackedVertex *outVerts)
{
    PROF_SCOPED("SkinXSurfaceWeight");

    int vertIndex = 0;
    const uint16_t *blend = vertexInfo->vertsBlend;
    if (vertexInfo->vertCount[0])
    {
        SkinWeightBlockInOut<0>(inVerts, blend, vertexInfo->vertCount[0], boneMatrix, srcVertNormals, dstVertNormals, outVerts, &vertIndex);
        blend += vertexInfo->vertCount[0];
    }
    if (vertexInfo->vertCount[1])
    {
        SkinWeightBlockInOut<1>(inVerts, blend, vertexInfo->vertCount[1], boneMatrix, srcVertNormals, dstVertNormals, outVerts, &vertIndex);
        blend += 3 * vertexInfo->vertCount[1];
    }
    if (vertexInfo->vertCount[2])
    {
        SkinWeightBlockInOut<2>(inVerts, blend, vertexInfo->vertCount[2], boneMatrix, srcVertNormals, dstVertNormals, outVerts, &vertIndex);
        blend += 5 * vertexInfo->vertCount[2];
    }
    if (vertexInfo->vertCount[3])
        SkinWeightBlockInOut<3>(inVerts, blend, vertexInfo->vertCount[3], boneMatrix, srcVertNormals, dstVertNormals, outVerts, &vertIndex);

    _mm_empty();
}

// ---------------------------------------------------------------------------
// rigid skinning: one bone per vertex list, no blend
// ---------------------------------------------------------------------------

static void R_SkinXSurfaceRigidSse(const XSurface *surf, int totalVertCount,
    const DObjSkelMat *boneMatrix, GfxPackedVertex *dstVerts)
{
    iassert(dstVerts);
    iassert(!(reinterpret_cast<uintptr_t>(dstVerts) & 15));
    iassert(!(reinterpret_cast<uintptr_t>(boneMatrix) & 15));

    PROF_SCOPED("SkinXSurfaceRigid");

    const GfxPackedVertex *src = (const GfxPackedVertex *)surf->verts0;
    GfxPackedVertex *dst = dstVerts;
    for (unsigned list = 0; list < surf->vertListCount; ++list)
    {
        const XRigidVertList *vertList = &surf->vertList[list];
        const DObjSkelMat *bone = BoneAt(boneMatrix, vertList->boneOffset);
        for (int i = 0; i < vertList->vertCount; ++i)
        {
            _mm_prefetch((const char *)&src[4], 0);
            Sse_SkinVertexRigid(bone, src, (__m64 *)dst);
            ++src;
            ++dst;
        }
    }
    iassert(dst - dstVerts == totalVertCount);
    _mm_empty();
}

static void R_SkinXSurfaceRigidSseOut(const XSurface *surf, int totalVertCount, const DObjSkelMat *boneMatrix,
    GfxPackedVertexNormal *dstVertNormals, GfxPackedVertex *dstVerts)
{
    iassert(dstVerts);
    iassert(dstVertNormals);
    iassert(!(reinterpret_cast<uintptr_t>(dstVerts) & 15));
    iassert(!(reinterpret_cast<uintptr_t>(boneMatrix) & 15));

    PROF_SCOPED("SkinXSurfaceRigid");

    const GfxPackedVertex *src = (const GfxPackedVertex *)surf->verts0;
    GfxPackedVertex *dst = dstVerts;
    for (unsigned list = 0; list < surf->vertListCount; ++list)
    {
        const XRigidVertList *vertList = &surf->vertList[list];
        const DObjSkelMat *bone = BoneAt(boneMatrix, vertList->boneOffset);
        for (int i = 0; i < vertList->vertCount; ++i)
        {
            _mm_prefetch((const char *)&src[4], 0);
            __m64 normalTangent = Sse_SkinVertexRigid(bone, src, (__m64 *)dst);
            _mm_stream_pi((__m64 *)dstVertNormals, normalTangent);
            ++src;
            ++dst;
            ++dstVertNormals;
        }
    }
    iassert(dst - dstVerts == totalVertCount);
    _mm_empty();
}

static void R_SkinXSurfaceRigidSseInOut(const XSurface *surf, int totalVertCount, const DObjSkelMat *boneMatrix,
    const __m64 *srcVertNormals, GfxPackedVertexNormal *dstVertNormals,
    GfxPackedVertex *dstVerts)
{
    iassert(dstVerts);
    iassert(dstVertNormals);
    iassert(!(reinterpret_cast<uintptr_t>(dstVerts) & 15));
    iassert(!(reinterpret_cast<uintptr_t>(boneMatrix) & 15));

    PROF_SCOPED("SkinXSurfaceRigid");

    const GfxPackedVertex *src = (const GfxPackedVertex *)surf->verts0;
    GfxPackedVertex *dst = dstVerts;
    for (unsigned list = 0; list < surf->vertListCount; ++list)
    {
        const XRigidVertList *vertList = &surf->vertList[list];
        const DObjSkelMat *bone = BoneAt(boneMatrix, vertList->boneOffset);
        for (int i = 0; i < vertList->vertCount; ++i)
        {
            _mm_prefetch((const char *)&src[4], 0);
            Sse_SkinVertexRigidSimple(bone, src, (__m64 *)dst, *srcVertNormals, dstVertNormals);
            ++src;
            ++dst;
            ++srcVertNormals;
            ++dstVertNormals;
        }
    }
    iassert(dst - dstVerts == totalVertCount);
    _mm_empty();
}

// ---------------------------------------------------------------------------
// public entry point
// ---------------------------------------------------------------------------

void __cdecl R_SkinXSurfaceSkinnedSse(const XSurface *xsurf, const DObjSkelMat *boneMatrix,
    GfxPackedVertexNormal *skinVertNormalIn, GfxPackedVertexNormal *skinVertNormalOut,
    GfxPackedVertex *skinVerticesOut)
{
    if (skinVertNormalOut)
    {
        if (skinVertNormalIn)
        {
            if ((xsurf->flags & 0x80) != 0)
                R_SkinXSurfaceWeightSseInOut(xsurf->verts0, &xsurf->vertInfo, boneMatrix,
                    skinVertNormalIn, skinVertNormalOut, skinVerticesOut);
            else
                R_SkinXSurfaceRigidSseInOut(xsurf, xsurf->vertCount, boneMatrix,
                    (const __m64 *)skinVertNormalIn, skinVertNormalOut, skinVerticesOut);
        }
        else if ((xsurf->flags & 0x80) != 0)
        {
            R_SkinXSurfaceWeightSseOut(xsurf->verts0, &xsurf->vertInfo, boneMatrix, skinVertNormalOut, skinVerticesOut);
        }
        else
        {
            R_SkinXSurfaceRigidSseOut(xsurf, xsurf->vertCount, boneMatrix, skinVertNormalOut, skinVerticesOut);
        }
    }
    else
    {
        iassert(!skinVertNormalIn);
        if ((xsurf->flags & 0x80) != 0)
            R_SkinXSurfaceWeightSse(xsurf->verts0, &xsurf->vertInfo, boneMatrix, skinVerticesOut);
        else
            R_SkinXSurfaceRigidSse(xsurf, xsurf->vertCount, boneMatrix, skinVerticesOut);
    }
}

#endif
