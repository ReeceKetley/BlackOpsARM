#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cfloat>
#include <vector>
#include <algorithm>
#if defined(WATER_GPU)
#include <d3d9.h>
#endif

using _BYTE = unsigned char;
using _WORD = unsigned short;
using _DWORD = unsigned int;
using _QWORD = uint64_t;
#define LODWORD(x) (*reinterpret_cast<unsigned int *>(&(x)))
#define HIDWORD(x) (*(reinterpret_cast<unsigned int *>(&(x)) + 1))
#undef LOBYTE
#undef HIBYTE
#define LOBYTE(x) (*reinterpret_cast<unsigned char *>(&(x)))
#define BYTE1(x) (*(reinterpret_cast<unsigned char *>(&(x)) + 1))
#define BYTE2(x) (*(reinterpret_cast<unsigned char *>(&(x)) + 2))
#define HIBYTE(x) (*(reinterpret_cast<unsigned char *>(&(x)) + sizeof(x) - 1))
#define __PAIR64__(hi,lo) ((uint64_t(hi) << 32) | uint32_t(lo))
struct GfxColor { union { unsigned int packed; unsigned char array[4]; }; };
static void require(bool ok, const char *why) { if (!ok) { fprintf(stderr, "FAIL: %s\n", why); exit(1); } }
// PRODUCTION_TYPES
struct { float gridScale; } config;
struct {
    struct { float4 *v; } buffer[2];
    struct { short *v; } waterheight;
    struct { char *v; } shoredist;
    struct { GfxColor *v; } colors;
    void *vertexBuffer;
} data;
GfxWaterVertex *vertexOutput;
bool deviceOK = true, lockOK = true;
unsigned locks, unlocks;
bool Sys_QueryD3DDeviceOKEvent() { return deviceOK; }
void *R_LockVertexBuffer(void *buffer, unsigned a, unsigned b, unsigned flags) {
    require(a == 0 && b == 0 && flags == 0x1000, "retail lock parameters");
    ++locks;
    if (!lockOK) return nullptr;
#if defined(WATER_GPU)
    void *result = nullptr;
    require(SUCCEEDED(static_cast<IDirect3DVertexBuffer9 *>(buffer)->Lock(a, b, &result, flags)), "real vertex buffer lock");
    return result;
#else
    return vertexOutput;
#endif
}
void R_UnlockVertexBuffer(void *buffer) {
    ++unlocks;
#if defined(WATER_GPU)
    require(SUCCEEDED(static_cast<IDirect3DVertexBuffer9 *>(buffer)->Unlock()), "real vertex buffer unlock");
#endif
}
float4 *__cdecl ReadValue(float4 *, unsigned char, int, unsigned);
// PRODUCTION_FUNCTIONS

struct Guarded {
    unsigned char *base, *ptr;
    size_t size;
    explicit Guarded(size_t requested) : size(requested) {
        size_t rounded = (requested + 4095) & ~size_t(4095);
        static uintptr_t address = sizeof(void *) == 8 ? uintptr_t(0x180000000ull) : 0;
        base = static_cast<unsigned char *>(VirtualAlloc(reinterpret_cast<void *>(address),
            rounded + 8192, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
        require(base != nullptr, "guarded allocation");
        if (sizeof(void *) == 8) {
            require(uintptr_t(base) > 0x100000000ull, "real high-address fixture");
            address += 0x200000;
        }
        DWORD previous;
        require(VirtualProtect(base, 4096, PAGE_NOACCESS, &previous) != 0, "front guard");
        require(VirtualProtect(base + 4096 + rounded, 4096, PAGE_NOACCESS, &previous) != 0, "end guard");
        ptr = base + 4096 + rounded - requested;
        if (requested % 16 == 0) require(uintptr_t(ptr) % 16 == 0, "SIMD alignment");
    }
    ~Guarded() { VirtualFree(base, 0, MEM_RELEASE); }
};
static void read(FILE *file, void *ptr, size_t size) {
    require(fread(ptr, 1, size, file) == size, "fixture read");
}
template<class T> static T read(FILE *file) { T v; read(file, &v, sizeof(v)); return v; }
int main(int argc, char **argv) {
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    require(argc == 2, "oracle file argument");
    FILE *file = nullptr; fopen_s(&file, argv[1], "rb"); require(file != nullptr, "oracle file");
    char magic[8]; read(file, magic, 8); require(memcmp(magic, "BO1WATR1", 8) == 0, "fixture version");
    unsigned cases = read<unsigned>(file), vertices = read<unsigned>(file), total = 0;
    Guarded bank0(65536 * 16), bank1(65536 * 16), height(65536 * 2), shore(65536), color(65536 * 4);
    Guarded output(vertices * 16), meshArena(sizeof(tilemesh_t) * 16), commandArena(sizeof(WaterSimulationCmd));
    vertexOutput = reinterpret_cast<GfxWaterVertex *>(output.ptr);
#if defined(WATER_GPU)
    // A hidden helper window keeps this fixture independent of interactive play.
    HWND window = CreateWindowExW(0, L"STATIC", L"BO1 water vertex fixture", WS_OVERLAPPED,
        60, 60, 64, 64, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    require(window != nullptr, "GPU fixture window");
    IDirect3D9 *d3d = Direct3DCreate9(D3D_SDK_VERSION); require(d3d != nullptr, "D3D9 runtime");
    D3DPRESENT_PARAMETERS params{}; params.Windowed = TRUE; params.SwapEffect = D3DSWAPEFFECT_DISCARD;
    params.hDeviceWindow = window; params.BackBufferWidth = params.BackBufferHeight = 64;
    params.BackBufferFormat = D3DFMT_UNKNOWN; params.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    IDirect3DDevice9 *device = nullptr;
    require(SUCCEEDED(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window,
        D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE, &params, &device)), "real D3D9 device");
    IDirect3DVertexBuffer9 *buffer = nullptr;
    require(SUCCEEDED(device->CreateVertexBuffer(static_cast<UINT>(output.size), D3DUSAGE_DYNAMIC,
        0, D3DPOOL_DEFAULT, &buffer, nullptr)), "real dynamic water vertex buffer");
    data.vertexBuffer = buffer;
    for (const char *name : {"d3d9.dll", "vulkan-1.dll", "nvoglv64.dll", "VkLayer_khronos_validation.dll"}) {
        HMODULE module = GetModuleHandleA(name);
        if (module) { char path[MAX_PATH]; GetModuleFileNameA(module, path, MAX_PATH); printf("MODULE %s: %s\n", name, path); }
    }
#endif
    data.buffer[0].v = reinterpret_cast<float4 *>(bank0.ptr); data.buffer[1].v = reinterpret_cast<float4 *>(bank1.ptr);
    data.waterheight.v = reinterpret_cast<short *>(height.ptr); data.shoredist.v = reinterpret_cast<char *>(shore.ptr);
    data.colors.v = reinterpret_cast<GfxColor *>(color.ptr);
    for (unsigned c = 0; c < cases; ++c) {
        char name[32]; read(file, name, 32);
        config.gridScale = read<float>(file);
        unsigned bank = read<unsigned>(file), meshCount = read<unsigned>(file);
        auto *cmd = new(commandArena.ptr) WaterSimulationCmd{};
        cmd->skin.srcBuf = bank;
        auto *meshes = reinterpret_cast<tilemesh_t *>(meshArena.ptr);
        memset(meshes, 0, sizeof(tilemesh_t) * 16);
        for (unsigned i = 0; i < meshCount; ++i) {
            int fields[7]; read(file, fields, sizeof(fields));
            meshes[i].srcU = fields[0]; meshes[i].srcV = fields[1]; meshes[i].worldX = fields[2];
            meshes[i].worldY = fields[3]; meshes[i].worldZ = fields[4]; meshes[i].lastSkinTime = fields[5];
            meshes[i].baseVertex = fields[6]; cmd->skin.meshesEA[i] = &meshes[i];
        }
        for (auto *allocation : {&bank0, &bank1, &height, &shore, &color}) read(file, allocation->ptr, allocation->size);
        std::vector<unsigned char> expected(output.size); read(file, expected.data(), expected.size());
        memset(output.ptr, 0xcd, output.size); locks = unlocks = 0;
#if defined(WATER_GPU)
        void *mapped = nullptr;
        require(SUCCEEDED(buffer->Lock(0, 0, &mapped, D3DLOCK_DISCARD)), "initialize real buffer");
        memset(mapped, 0xcd, output.size); require(SUCCEEDED(buffer->Unlock()), "initialize unlock");
#endif
        R_WaterSimulationSkinMesh(cmd);
        require(locks == meshCount && unlocks == meshCount, "one balanced VB lock per populated slot");
#if defined(WATER_GPU)
        require(SUCCEEDED(buffer->Lock(0, 0, &mapped, D3DLOCK_READONLY)), "read real packed output");
        memcpy(output.ptr, mapped, output.size); require(SUCCEEDED(buffer->Unlock()), "read unlock");
#endif
        for (size_t byte = 0; byte < output.size; ++byte) if (output.ptr[byte] != expected[byte]) {
            fprintf(stderr, "FAIL %s: vertex %zu byte %zu got %u expected %u\n", name, byte / 16, byte % 16,
                output.ptr[byte], expected[byte]); return 1;
        }
        printf("PASS %s: %u retail-identical vertices, output gaps unchanged\n", name, meshCount * 289);
        total += meshCount * 289;
        // Device loss and failed locks must leave the buffer untouched and balanced.
        memset(output.ptr, 0xcd, output.size); locks = unlocks = 0; deviceOK = false;
        R_WaterSimulationSkinMesh(cmd); require(locks == 0 && unlocks == 0, "device loss does not lock");
        deviceOK = true; lockOK = false; R_WaterSimulationSkinMesh(cmd);
        require(locks == 1 && unlocks == 0, "failed lock is not unlocked"); lockOK = true;
        require(std::all_of(output.ptr, output.ptr + output.size, [](unsigned char x) { return x == 0xcd; }), "failed paths do not write");
    }
    require(fgetc(file) == EOF, "whole fixture consumed"); fclose(file);
#if defined(WATER_GPU)
    buffer->Release(); device->Release(); d3d->Release(); DestroyWindow(window);
    puts("PASS: real D3D9 vertex buffer lifecycle");
#endif
    printf("PASS: %u vertices match retail, pointer width %zu\n", total, sizeof(void *));
}
