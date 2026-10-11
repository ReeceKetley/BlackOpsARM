#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <thread>
#include <algorithm>

using _BYTE = unsigned char;
using _DWORD = unsigned int;
using _QWORD = uint64_t;
#define LODWORD(x) (*reinterpret_cast<unsigned int *>(&(x)))
#define HIDWORD(x) (*(reinterpret_cast<unsigned int *>(&(x)) + 1))
static void require(bool value, const char *why) {
    if (!value) { fprintf(stderr, "FAIL: %s\n", why); exit(1); }
}
#define iassert(x) require(!!(x), #x)
bool Assert_MyHandler(const char *, int, int, const char *, ...) {
    require(false, "production assertion"); return true;
}
struct IDirect3DIndexBuffer9 {};
struct IDirect3DVertexBuffer9 {};
struct GfxWaterVertex {};
struct jqModule {};
struct jqQueue {};
// PRODUCTION_TYPES

struct GfxDrawSurf { union { uint64_t packed; uint64_t fields; }; };
struct Material { struct { GfxDrawSurf drawSurf; } info; } material;
struct { Material *waterMaterial; } s_world;
struct { struct { float origin[3]; } correctedLodParms[4]; } rg;
struct { float gridScale; } config;
struct tile_t {
    bool occupied;
    float minMax[2][3];
    int worldX, worldY, worldZ;
    struct { unsigned head, tail; tilemesh_t data[4]; } meshes;
};
struct {
    tile_t tiles[256]; unsigned frame;
    IDirect3DVertexBuffer9 *vertexBuffer;
    IDirect3DIndexBuffer9 *indexBuffer[2];
    unsigned tileTriCount[2];
    struct { float4 *v; } buffer[2];
} data;
struct alignas(32) GfxBackEndData {
    unsigned char surfsBuffer[262144];
    volatile unsigned int surfPos;
};
GfxBackEndData *frontEndDataOut;
struct { GfxDrawSurf drawSurfs[2][256]; unsigned drawSurfCount[2]; } scene;
// PRODUCTION_SWIZZLE
constexpr int R_WARN_MAX_SCENE_SURFS_SIZE = 0;
unsigned warnings;
void R_WarnOncePerFrame(int) { ++warnings; }
bool R_CullBoxCurDpvs(const float *, unsigned) { return false; }
float Vec3DistanceSq(const float *a, const float *b) {
    float sum = 0; for (int i = 0; i < 3; ++i) sum += (a[i] - b[i]) * (a[i] - b[i]);
    return sum;
}
unsigned waterCompareExchange(volatile unsigned *p, unsigned next, unsigned previous) {
    return InterlockedCompareExchange(reinterpret_cast<volatile LONG *>(p), next, previous);
}
unsigned waterExchangeAdd(volatile unsigned *p, unsigned size) {
    return InterlockedExchangeAdd(reinterpret_cast<volatile LONG *>(p), size);
}
#define _InterlockedCompareExchange waterCompareExchange
#define _InterlockedExchangeAdd waterExchangeAdd
char *__cdecl R_SurfaceAlloc(unsigned size);
#if defined(_WIN64)
#define JQ_CMD_SIZE(T, win32Size) ((unsigned int)sizeof(T))
#define JQ_PARAM_INLINE 3
#else
#define JQ_CMD_SIZE(T, win32Size) (win32Size)
#define JQ_PARAM_INLINE 2
#endif
struct jqBatch { unsigned int ParamData[24]; void *Input; };
unsigned char queueHeap[256];
unsigned char *jqAllocBatchData(unsigned size) {
    require(size <= sizeof(queueHeap), "queue fixture capacity");
    return queueHeap;
}
jqModule r_water_simModule;
volatile unsigned r_water_simLimit;
// PRODUCTION_WORKER
// PRODUCTION_FUNCTIONS

static void checkChannels() {
    // Actual ReadValue samples both buffers and wraps the seam without turning an ID into a pointer.
    for (unsigned buf = 0; buf < 2; ++buf) {
        data.buffer[buf].v = static_cast<float4 *>(VirtualAlloc(nullptr, 65536 * sizeof(float4),
            MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
        require(data.buffer[buf].v != nullptr, "channel fixture allocation");
        for (unsigned y = 0; y < 256; ++y) for (unsigned x = 0; x < 256; ++x)
            for (unsigned c = 0; c < 4; ++c)
                data.buffer[buf].v[256 * y + x].u[c] = (buf << 28) | (y << 16) | (x << 8) | c;
    }
    for (unsigned buf = 0; buf < 2; ++buf) for (int y : {-1, 0, 255, 256}) {
        float4 value{}; ReadValue(&value, 255, y, buf);
        auto expected = data.buffer[buf ^ ((y >> 8) & 1)].v[256 * (unsigned char)y + 255];
        require(value.u[0] == expected.u[0] && value.u[1] == expected.u[2] &&
                value.u[2] == expected.u[1] && value.u[3] == expected.u[3], "channel seam and XZYW swizzle");
    }
}

int main(int argc, char **argv) {
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    if (argc > 1 && strcmp(argv[1], "--lookup") == 0) { checkChannels(); puts("PASS: channel lookup"); return 0; }
    auto *arena = static_cast<unsigned char *>(VirtualAlloc(
        sizeof(void *) == 8 ? reinterpret_cast<void *>(uintptr_t(0x180000000ull)) : nullptr,
        0x60000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    require(arena != nullptr, "allocate high-address arena");
    if (sizeof(void *) == 8) require(uintptr_t(arena) > 0x100000000ull, "arena above 4 GB");
    if (argc == 1) {
    WaterSimulationCmd command{};
    command.update.startTile = 13; command.update.srcBuf = 1;
    command.skin.startTile = 7;
    for (unsigned i = 0; i < 16; ++i)
        command.skin.meshes[i] = reinterpret_cast<tilemesh_t *>(arena + 0x54000 + 64 * i);
    require(r_water_simWorkerCmd.dataSize == sizeof(command), "worker reserves the whole native water command");
    jqBatch batch{};
    batch.Input = jqCommitMemory(&batch, reinterpret_cast<unsigned char *>(&command), r_water_simWorkerCmd.dataSize);
    auto worker = &r_water_simWorkerCmd;
    memcpy(batch.ParamData, &worker, sizeof(worker));
    auto *received = static_cast<WaterSimulationCmd *>(jqLockData(&batch));
    require(memcmp(received, &command, sizeof(command)) == 0, "worker queue retains all 16 native mesh pointers");
    require(sizeof(void *) == 8 ? batch.Input == queueHeap : batch.Input == &batch.ParamData[JQ_PARAM_INLINE],
            "large native payload uses heap and Win32 remains inline");
    }
    frontEndDataOut = new(arena) GfxBackEndData{};
    config.gridScale = 8;
    data.frame = 100;
    data.vertexBuffer = reinterpret_cast<IDirect3DVertexBuffer9 *>(arena + 0x51000);
    data.indexBuffer[0] = reinterpret_cast<IDirect3DIndexBuffer9 *>(arena + 0x52000);
    data.indexBuffer[1] = reinterpret_cast<IDirect3DIndexBuffer9 *>(arena + 0x53000);
    data.tileTriCount[0] = 512; data.tileTriCount[1] = 128;
    material.info.drawSurf.packed = 3ull << 58;
    s_world.waterMaterial = &material;
    for (unsigned head = 0; head < 4; ++head) {
        memset(data.tiles, 0, sizeof(data.tiles));
        memset(&scene, 0, sizeof(scene));
        auto &tile = data.tiles[1];
        tile.occupied = true; tile.worldX = 10; tile.worldY = 20; tile.worldZ = 30;
        tile.meshes.head = head; tile.meshes.tail = (head + 2) & 3;
        tile.meshes.data[head].baseVertex = 17;
        tile.meshes.data[(head + 1) & 3].baseVertex = 34;
        tile.meshes.data[(head + 1) & 3].lastSkinTime = 92;
        for (bool distant : {false, true}) {
            frontEndDataOut->surfPos = 4; // another producer left a four-byte boundary
            scene.drawSurfCount[1] = 0;
            rg.correctedLodParms[0].origin[0] = distant ? 10000.0f : 0.0f;
            R_RenderWaterModel(0);
            require(scene.drawSurfCount[1] == 1, "one water surface");
            auto *surf = reinterpret_cast<GfxModelWaterSurface *>(arena +
                4 * static_cast<unsigned short>(scene.drawSurfs[1][0].packed));
            require(uintptr_t(surf) % alignof(GfxModelWaterSurface) == 0, "native surface alignment");
            require(surf->vb0 == data.vertexBuffer && surf->vb1 == data.vertexBuffer,
                    "GPU vertex pointers retained");
            require(surf->ib == data.indexBuffer[distant], "GPU index pointer and LOD retained");
            require(surf->baseVertex0 == 17 && surf->baseVertex1 == 34, "mesh ring and base vertices");
            require(surf->interpolate == 0.5f && surf->vertCount == 289 &&
                    surf->triCount == int(data.tileTriCount[distant]), "native scalar surface fields");
            require(surf->origin[0] == 80 && surf->origin[1] == 160 && surf->origin[2] == 240,
                    "native surface origin");
            require(frontEndDataOut->surfPos >= (unsigned char *)(surf + 1) - arena,
                    "whole native surface reserved");
        }
    }
    frontEndDataOut->surfPos = 262144 - sizeof(GfxModelWaterSurface);
    require(R_SurfaceAlloc(sizeof(GfxModelWaterSurface)) != nullptr, "last exact-fit surface");
    require(R_SurfaceAlloc(1) == nullptr && frontEndDataOut->surfPos == 262144, "full arena bounded");
    require(R_SurfaceAlloc(UINT_MAX) == nullptr, "oversized allocation cannot wrap");
    frontEndDataOut->surfPos = 0;
    std::vector<uintptr_t> addresses[4];
    std::thread workers[4];
    for (unsigned t = 0; t < 4; ++t) workers[t] = std::thread([&, t] {
        for (unsigned i = 0; i < 500; ++i) {
            auto p = R_SurfaceAlloc(sizeof(GfxModelWaterSurface));
            require(p != nullptr && uintptr_t(p) % alignof(GfxModelWaterSurface) == 0,
                    "concurrent allocation succeeds and is aligned");
            addresses[t].push_back(uintptr_t(p));
        }
    });
    std::vector<uintptr_t> all;
    for (unsigned t = 0; t < 4; ++t) { workers[t].join(); all.insert(all.end(), addresses[t].begin(), addresses[t].end()); }
    std::sort(all.begin(), all.end());
    for (unsigned i = 1; i < all.size(); ++i)
        require(all[i] - all[i - 1] >= sizeof(GfxModelWaterSurface), "concurrent surfaces do not overlap");
    checkChannels();
    printf("PASS: %zu-bit water surfaces, GPU pointers, ring/LOD, bounds, 2000 concurrent allocations, channel seams, native worker payload\n",
           8 * sizeof(void *));
    return 0;
}
