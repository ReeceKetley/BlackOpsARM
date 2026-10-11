#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <array>
#include <new>
static void require(bool ok, const char *why) { if (!ok) { fprintf(stderr, "FAIL: %s\n", why); exit(1); } }
#define iassert(x) require(!!(x), #x)
bool Assert_MyHandler(const char *, int, int, const char *, ...) { require(false, "production assertion"); return true; }
constexpr int TRACK_WATERSIM = 0;
void *_PMem_Alloc(unsigned, unsigned, unsigned, unsigned, int, const char *, int) { require(false, "unexpected channel allocation"); return nullptr; }
struct Material; struct IDirect3DIndexBuffer9; struct IDirect3DVertexBuffer9;
// PRODUCTION_TYPES
static void *guarded(size_t size) {
    static uintptr_t address = sizeof(void *) == 8 ? uintptr_t(0x180000000ull) : 0;
    size_t rounded = (size + 4095) & ~size_t(4095);
    auto *base = static_cast<unsigned char *>(VirtualAlloc(reinterpret_cast<void *>(address),
        rounded + 8192, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    require(base != nullptr, "guarded native allocation");
    if (sizeof(void *) == 8) { require(uintptr_t(base) > 0x100000000ull, "state/output above4GB"); address += 0x1000000; }
    DWORD previous;
    require(VirtualProtect(base, 4096, PAGE_NOACCESS, &previous) != 0, "front guard");
    require(VirtualProtect(base + 4096 + rounded, 4096, PAGE_NOACCESS, &previous) != 0, "end guard");
    return base + 4096 + rounded - size;
}
waterdata_t &data = *new(guarded(sizeof(waterdata_t))) waterdata_t{};
GfxWaterVertex *vertices = static_cast<GfxWaterVertex *>(guarded(800 * 289 * sizeof(GfxWaterVertex)));
fifo_t<meshExpire_t,1024> expiredMeshes;
fifo_t<debugpoint_t,16> debugPoints;
WaterSimulationCmd prevCmd;
unsigned numFreeVertBlocks, freeVertBlocks[800];
bool buffersAllocated;
struct { unsigned numWaterCells; } comWorld;
struct Dvar { struct { bool enabled; } current; } enabledDvar, drawDvar, debugDvar;
auto *r_watersim_enabled = &enabledDvar; auto *r_drawWater = &drawDvar; auto *r_watersim_debug = &debugDvar;
std::vector<unsigned> calls;
std::vector<WaterSimulationCmd> queued;
int r_water_simWorkerCmd;
void R_WaterSimulationUpdateConfig() { calls.push_back(1); }
void R_WaterSimulationTrackView(const float *) { calls.push_back(2); }
void R_RenderWaterModel(unsigned index) { require(index == 7, "view index"); calls.push_back(3); }
void Sys_AddWorkerCmdInternal(int *worker, unsigned char *cmd, int flags) {
    require(worker == &r_water_simWorkerCmd && flags == 0, "worker dispatch");
    calls.push_back(4); WaterSimulationCmd copy; memcpy(&copy, cmd, sizeof(copy)); queued.push_back(copy);
}
void R_WaterSimulationGetInfo(const float *, WaterSimInfo *) { require(false, "debug disabled"); }
struct { int debugGlobals; } frontEnd; auto *frontEndDataOut = &frontEnd;
float colorRed[4], colorGreen[4], colorBlue[4], colorYellow[4];
void R_AddDebugLine(int *, const float *, const float *, const float *, int) { require(false, "debug disabled"); }
// PRODUCTION_FUNCTIONS
template<class T> static T read(FILE *file) { T value; require(fread(&value, sizeof(T), 1, file) == 1, "fixture read"); return value; }
static void read(FILE *file, void *value, size_t size) { require(fread(value, 1, size, file) == size, "fixture read"); }
static std::array<unsigned,37> normalize(const WaterSimulationCmd &cmd) {
    std::array<unsigned,37> result{cmd.update.startTile, cmd.update.srcBuf, cmd.update.destBuf, cmd.skin.startTile, cmd.skin.srcBuf};
    for (unsigned m = 0; m < 16; ++m) {
        bool found = false;
        for (unsigned t = 0; t < 256 && !found; ++t) for (unsigned s = 0; s < 4; ++s)
            if (cmd.skin.meshes[m] == &data.tiles[t].meshes.data[s]) {
                result[5 + m * 2] = t; result[6 + m * 2] = s; found = true; break;
            }
        require(found, "queued native mesh pointer belongs to tile/slot");
    }
    return result;
}
int main(int argc, char **argv) {
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    require(argc == 2, "fixture argument"); FILE *file = nullptr; fopen_s(&file, argv[1], "rb"); require(file != nullptr, "fixture file");
    char magic[8]; read(file, magic, 8); require(memcmp(magic, "BO1SCHD1", 8) == 0, "fixture version");
    unsigned count = read<unsigned>(file), commandTotal = 0;
    for (unsigned c = 0; c < count; ++c) {
        char name[32]; read(file, name, sizeof(name));
        std::array<int,12> input; read(file, input.data(), sizeof(input));
        std::array<unsigned,8> expected; read(file, expected.data(), sizeof(expected));
        std::vector<unsigned> expectedFree(expected[3]); read(file, expectedFree.data(), expectedFree.size()*4);
        std::array<unsigned,2048> expectedExpired; read(file, expectedExpired.data(), sizeof(expectedExpired));
        std::vector<unsigned> expectedCalls(expected[6]); read(file, expectedCalls.data(), expectedCalls.size()*4);
        std::vector<std::array<unsigned,37>> expectedCommands(expected[7]); read(file, expectedCommands.data(), expectedCommands.size()*sizeof(expectedCommands[0]));
        std::array<std::array<unsigned,35>,256> expectedTiles; read(file, expectedTiles.data(), sizeof(expectedTiles));
        data.~waterdata_t(); new(&data) waterdata_t{}; data.lockedData = vertices;
        data.frame = input[3]; data.timeDelta = input[4]; data.enabled = false;
        enabledDvar.current.enabled = input[0] != 0; comWorld.numWaterCells = input[1]; buffersAllocated = input[2] != 0;
        drawDvar.current.enabled = input[7] != 0; debugDvar.current.enabled = false;
        memset(&prevCmd, 0, sizeof(prevCmd)); prevCmd.update.startTile = input[6]; prevCmd.update.destBuf = 1;
        for (unsigned t = 0; t < 256; ++t) {
            tile_t &tile = data.tiles[t]; tile.srcU = t%16*16; tile.srcV = t/16*16;
            tile.worldX = int(t)*3-200; tile.worldY = int(t)*5+10; tile.worldZ = t%9-4;
            tile.lastUpdateTime = input[8] && t/16 != 2 ? input[3] : 0;
            tile.meshes.head = input[9] ? 3 : 0; tile.meshes.tail = input[9] ? 1 : 0;
            for (unsigned s = 0; s < 4; ++s) {
                tilemesh_t &mesh = tile.meshes.data[s]; unsigned old = (t*2+(s==0 ? 1 : 0))%512;
                mesh.srcU=-10; mesh.srcV=-20; mesh.worldX=-30; mesh.worldY=-40; mesh.worldZ=-50; mesh.lastSkinTime=-1;
                mesh.baseVertex = old*289; mesh.lockedData = vertices+old*289;
            }
        }
        numFreeVertBlocks = input[10]; for (unsigned n = 0; n < numFreeVertBlocks; ++n) freeVertBlocks[n]=(n+(input[9]?512:0))*289;
        memset(&expiredMeshes, 0, sizeof(expiredMeshes));
        if (input[11]) {
            expiredMeshes.head=1022; expiredMeshes.tail=1;
            expiredMeshes.data[1022]={1,600*289}; expiredMeshes.data[1023]={0,601*289}; expiredMeshes.data[0]={2,602*289};
        }
        calls.clear(); queued.clear(); const float eye[3]{};
        R_WaterSimulationRender(eye,input[5],7);
        require(data.frame==expected[0] && unsigned(data.enabled)==expected[1] && unsigned(data.timeDelta)==expected[2], "retail frame/enabled/timestep");
        require(numFreeVertBlocks==expected[3] && expiredMeshes.head==expected[4] && expiredMeshes.tail==expected[5], "retail allocation/expiry counts");
        require(memcmp(freeVertBlocks,expectedFree.data(),expectedFree.size()*4)==0, "retail free block order");
        require(memcmp(expiredMeshes.data,expectedExpired.data(),sizeof(expectedExpired))==0, "retail full expiry queue");
        require(calls==expectedCalls && queued.size()==expectedCommands.size(), "retail call order/command count");
        for (unsigned n = 0; n < queued.size(); ++n) require(normalize(queued[n])==expectedCommands[n], "retail native queued payload");
        if (!queued.empty()) require(normalize(prevCmd)==expectedCommands.back(), "previous command preserves complete native payload");
        for (unsigned t = 0; t < 256; ++t) {
            const tile_t &tile=data.tiles[t]; const auto &wanted=expectedTiles[t];
            require(tile.lastUpdateTime==wanted[0] && tile.meshes.head==wanted[1] && tile.meshes.tail==wanted[2], "retail tile timestamps/FIFO");
            for (unsigned s = 0; s < 4; ++s) {
                const tilemesh_t &m=tile.meshes.data[s]; const unsigned actual[8]={unsigned(m.srcU),unsigned(m.srcV),unsigned(m.worldX),unsigned(m.worldY),unsigned(m.worldZ),unsigned(m.lastSkinTime),m.baseVertex,unsigned(m.lockedData-vertices)};
                require(memcmp(actual,wanted.data()+3+s*8,sizeof(actual))==0, "retail mesh source/world/time/vertex fields");
            }
            require(tile.srcU==int(t%16*16) && tile.srcV==int(t/16*16) && tile.worldX==int(t)*3-200 && tile.worldY==int(t)*5+10 && tile.worldZ==int(t%9)-4 && !tile.occupied, "source tile fields unchanged");
        }
        commandTotal += unsigned(queued.size()); printf("PASS %s: %zu retail-identical commands and complete tile/expiry/free state\n",name,queued.size());
    }
    printf("PASS: %u scheduler cases, %u native queued commands, pointer width%zu\n",count,commandTotal,sizeof(void *)); fclose(file);
}
