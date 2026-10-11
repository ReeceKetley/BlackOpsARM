#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include <new>
#pragma comment(linker, "\"/manifestuac:level='asInvoker' uiAccess='false'\"")

static void require(bool ok, const char *why) { if (!ok) { fprintf(stderr, "FAIL: %s\n", why); exit(1); } }
#define iassert(x) require(!!(x), #x)
bool Assert_MyHandler(const char *, int, int, const char *, ...) { require(false, "production assertion"); return true; }
constexpr int TRACK_WATERSIM = 0;
struct tilemesh_t;
std::vector<void *> allocations;
void *_PMem_Alloc(unsigned size, unsigned, unsigned, unsigned, int, const char *, int) {
    static uintptr_t address = sizeof(void *) == 8 ? uintptr_t(0x180000000ull) : 0;
    size_t rounded = (size + 4095) & ~size_t(4095);
    auto *base = static_cast<unsigned char *>(VirtualAlloc(reinterpret_cast<void *>(address),
        rounded + 8192, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    require(base != nullptr, "guarded channel allocation");
    if (sizeof(void *) == 8) { require(uintptr_t(base) > 0x100000000ull, "channel above4GB"); address += 0x200000; }
    DWORD previous;
    require(VirtualProtect(base, 4096, PAGE_NOACCESS, &previous) != 0, "front guard");
    require(VirtualProtect(base + 4096 + rounded, 4096, PAGE_NOACCESS, &previous) != 0, "end guard");
    allocations.push_back(base);
    return base + 4096 + rounded - size;
}
// PRODUCTION_TYPES
waterconfig_t config;
struct WaterData {
    channel_t<float4> buffer[2];
    channel_t<short> waterheight;
    channel_t<char> flooroffset, shoredist;
    unsigned centerU, centerV;
};
WaterData &data = *new(_PMem_Alloc(sizeof(WaterData), 16, 4, 0, 0, "", 0)) WaterData{};
struct { struct { bool enabled; } current; } enabledDvar, flattenDvar;
auto *r_watersim_enabled = &enabledDvar;
auto *r_watersim_flatten = &flattenDvar;
bool waterSimInitialized = true;
struct ValueDvar { struct { float value; } current; };
// PRODUCTION_CONFIG_DVARS
// PRODUCTION_CONFIG
// PRODUCTION_UPDATE

static void read(FILE *file, void *ptr, size_t size) { require(fread(ptr, 1, size, file) == size, "fixture read"); }
template<class T> static T read(FILE *file) { T value; read(file, &value, sizeof(value)); return value; }
int main(int argc, char **argv) {
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    require(argc == 2, "oracle file argument"); FILE *file = nullptr; fopen_s(&file, argv[1], "rb"); require(file != nullptr, "oracle file");
    char magic[8]; read(file, magic, 8); require(memcmp(magic, "BO1UPDT1", 8) == 0, "fixture version");
    unsigned cases = read<unsigned>(file), total = 0;
    for (float degrees : {0.0f, 90.0f, 180.0f, 270.0f, -90.0f, 450.0f}) {
        r_watersim_windDir->current.value = degrees;
        R_WaterSimulationUpdateConfig();
        require(config.windDir[1] == 0.0f, "retail wind has zero vertical Y");
        const double radians = double(degrees) * 3.14159265358979323846 / 180.0;
        require(fabs(config.windDir[0] - cos(radians)) < 0.000001 &&
                fabs(config.windDir[2] - sin(radians)) < 0.000001, "retail horizontal wind uses X/Z");
    }
    puts("PASS: production wind config matches retail X/0/Z axes");
    enabledDvar.current.enabled = true;
    data.buffer[0].Alloc(0); data.buffer[1].Alloc(0); data.waterheight.Alloc(0); data.flooroffset.Alloc(0); data.shoredist.Alloc(0);
    for (unsigned c = 0; c < cases; ++c) {
        char name[32]; read(file, name, 32); read(file, &config, sizeof(config));
        unsigned start = read<unsigned>(file); data.centerU = read<unsigned>(file); data.centerV = read<unsigned>(file);
        flattenDvar.current.enabled = read<unsigned>(file) != 0; unsigned steps = read<unsigned>(file);
        for (auto &bank : data.buffer) {
            read(file, bank.v, sizeof(float4) * 65536);
            memset(reinterpret_cast<unsigned char *>(bank.v) + sizeof(float4) * 65536, 0xcd, 128);
            for (auto &cache : bank.cache) { cache.y = -1; cache.modified = false; } bank.cache_now = 0;
        }
        read(file, data.waterheight.v, 65536 * 2); read(file, data.flooroffset.v, 65536); read(file, data.shoredist.v, 65536);
        for (unsigned i = 0; i < 4; ++i) {
            data.waterheight.Cache(0, false, true); data.flooroffset.Cache(0, false, true); data.shoredist.Cache(0, false, true);
        }
        std::vector<float4> expected[2];
        for (auto &bank : expected) { bank.resize(65536); read(file, bank.data(), bank.size() * sizeof(float4)); }
        for (unsigned step = 0; step < steps; ++step) {
            WaterSimulationCmd cmd{}; cmd.update.startTile = start; cmd.update.srcBuf = step & 1; cmd.update.destBuf = (step & 1) ^ 1;
            R_WaterSimulationUpdateFrame(&cmd);
            for (auto &bank : data.buffer) for (unsigned i = 0; i < 4; ++i) bank.Cache(0, false, true);
        }
        for (unsigned bank = 0; bank < 2; ++bank) for (unsigned cell = 0; cell < 65536; ++cell)
            for (unsigned lane = 0; lane < 4; ++lane) if (data.buffer[bank].v[cell].u[lane] != expected[bank][cell].u[lane]) {
                fprintf(stderr, "FAIL %s bank%u row%u col%u lane%u got %.9g/%08x expected %.9g/%08x\n", name, bank,
                    cell / 256, cell % 256, lane, data.buffer[bank].v[cell].v[lane], data.buffer[bank].v[cell].u[lane],
                    expected[bank][cell].v[lane], expected[bank][cell].u[lane]); return 1;
            }
        printf("PASS %s: %u retail-identical cell updates, both banks checked\n", name, steps * 4096);
        total += steps * 4096;
    }
    require(fgetc(file) == EOF, "whole fixture consumed"); fclose(file);
    printf("PASS: %u exact retail cell updates, pointer width%zu\n", total, sizeof(void *));
}
