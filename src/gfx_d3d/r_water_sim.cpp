#include "r_water_sim.h"
#if defined(__ANDROID__)
#include <platform/android/water_mutex.h>
#endif
#include "r_init.h"
#include <qcommon/com_bsp.h>
#include "r_foliage.h"
#include "r_dvars.h"
#include <universal/com_workercmds.h>
#include "r_debug.h"
#include "r_workercmds.h"
#include "r_warn.h"
#include "rb_tess.h"
#include <cstdint>
#include <qcommon/threads.h>

float max_plop_size = 5.0;
float max_plop_height = 5.0;
int explodesize = 3;
float explodeheight = 3.0;
int debrissize = 1;
float debrisheight = 1.0;

int water_rand_seed = 3;

float4 g_255 = { { 255.0, 255.0, 255.0, 255.0 } };
// These are packed byte indices, not floating-point values.
static float4 WaterSwizzle(unsigned int x, unsigned int y, unsigned int z, unsigned int w)
{
    float4 result;
    result.u[0] = x;
    result.u[1] = y;
    result.u[2] = z;
    result.u[3] = w;
    return result;
}
float4 g_swizzleXZYW = WaterSwizzle(66051u, 134810123u, 67438087u, 202182159u);
float4 g_swizzleXYAB = WaterSwizzle(66051u, 67438087u, 269554195u, 336926231u);

unsigned __int16 tessTable_0000[25] =
{
  0u,
  3u,
  1u,
  1u,
  3u,
  4u,
  1u,
  4u,
  2u,
  2u,
  4u,
  5u,
  3u,
  6u,
  4u,
  4u,
  6u,
  7u,
  4u,
  7u,
  5u,
  5u,
  7u,
  8u,
  65535u
};

unsigned __int16 tessTable_0001[22] =
{
  0u,
  3u,
  1u,
  1u,
  3u,
  4u,
  1u,
  4u,
  2u,
  2u,
  4u,
  5u,
  3u,
  6u,
  4u,
  4u,
  5u,
  8u,
  4u,
  8u,
  5u,
  65535u
};

unsigned __int16 tessTable_0010[22] =
{
  0u,
  3u,
  4u,
  0u,
  4u,
  2u,
  2u,
  4u,
  5u,
  3u,
  6u,
  4u,
  4u,
  6u,
  7u,
  4u,
  7u,
  5u,
  5u,
  7u,
  8u,
  65535u
};

unsigned __int16 tessTable_0011[13] =
{ 0u, 3u, 2u, 2u, 3u, 5u, 3u, 6u, 5u, 5u, 6u, 8u, 65535u };

unsigned __int16 tessTable_0100[22] =
{
  0u,
  3u,
  1u,
  1u,
  3u,
  4u,
  1u,
  4u,
  2u,
  2u,
  4u,
  8u,
  3u,
  6u,
  4u,
  4u,
  6u,
  7u,
  4u,
  7u,
  8u,
  65535u
};

unsigned __int16 tessTable_0101[13] =
{ 0u, 3u, 1u, 1u, 3u, 8u, 1u, 8u, 2u, 3u, 6u, 8u, 65535u };

unsigned __int16 tessTable_0110[13] =
{ 0u, 3u, 2u, 3u, 7u, 2u, 3u, 6u, 7u, 2u, 7u, 8u, 65535u };

unsigned __int16 tessTable_0111[10] =
{ 0u, 3u, 2u, 2u, 3u, 8u, 3u, 6u, 8u, 65535u };

unsigned __int16 tessTable_1000[22] =
{
  0u,
  6u,
  4u,
  0u,
  4u,
  1u,
  1u,
  4u,
  2u,
  2u,
  4u,
  5u,
  4u,
  6u,
  7u,
  4u,
  7u,
  5u,
  5u,
  7u,
  8u,
  65535u
};

unsigned __int16 tessTable_1001[13] =
{ 0u, 6u, 1u, 1u, 5u, 2u, 1u, 6u, 5u, 5u, 6u, 8u, 65535u };

unsigned __int16 tessTable_1010[13] =
{ 0u, 5u, 2u, 0u, 7u, 5u, 0u, 6u, 7u, 5u, 7u, 8u, 65535u };

unsigned __int16 tessTable_1011[10] =
{ 0u, 5u, 2u, 0u, 6u, 5u, 5u, 6u, 8u, 65535u };

unsigned __int16 tessTable_1100[13] =
{ 0u, 6u, 1u, 1u, 6u, 7u, 1u, 7u, 2u, 2u, 7u, 8u, 65535u };

unsigned __int16 tessTable_1101[10] =
{ 0u, 6u, 1u, 1u, 6u, 8u, 1u, 8u, 2u, 65535u };

unsigned __int16 tessTable_1110[10] =
{ 0u, 7u, 2u, 0u, 6u, 7u, 2u, 7u, 8u, 65535u };

unsigned __int16 tessTable_1111[7] =
{ 0u, 6u, 2u, 2u, 6u, 8u, 65535u };


// *WARNING* One or more selections were skipped as they could not be interpreted as c data

unsigned __int16 *tessTable[16] =
{
    tessTable_0000,
    tessTable_0001,
    tessTable_0010,
    tessTable_0011,
    tessTable_0100,
    tessTable_0101,
    tessTable_0110,
    tessTable_0111,
    tessTable_1000,
    tessTable_1001,
    tessTable_1010,
    tessTable_1011,
    tessTable_1100,
    tessTable_1101,
    tessTable_1110,
    tessTable_1111
};



bool waterSimInitialized;
bool vertexBuffersAllocated;

const dvar_t *r_watersim_enabled;
const dvar_t *r_watersim_debug;
const dvar_t *r_watersim_flatten;
const dvar_t *r_watersim_waveSeedDelay;
const dvar_t *r_watersim_curlAmount;
const dvar_t *r_watersim_curlMax;
const dvar_t *r_watersim_curlReduce;
const dvar_t *r_watersim_minShoreHeight;
const dvar_t *r_watersim_foamAppear;
const dvar_t *r_watersim_foamDisappear;
const dvar_t *r_watersim_windAmount;
const dvar_t *r_watersim_windDir;
const dvar_t *r_watersim_windMax;
const dvar_t *r_watersim_particleGravity;
const dvar_t *r_watersim_particleLimit;
const dvar_t *r_watersim_particleLength;
const dvar_t *r_watersim_particleWidth;
const dvar_t *r_watersim_scroll;

waterconfig_t config;
waterdata_t data;

HANDLE waterUpdateMutex;
unsigned int freeVertBlocks[800];
unsigned int numFreeVertBlocks;

fifo_t<meshExpire_t, 1024> expiredMeshes;

WaterSimulationCmd prevCmd;

fifo_t<debugpoint_t, 16> debugPoints;

bool buffersAllocated = false;

void __cdecl R_InitWaterSimulation()
{
    unsigned int n; // [esp+1Ch] [ebp-8h]

    if ( waterSimInitialized )
    {
        R_WaterSimulationRestart();
    }
    else
    {
        waterSimInitialized = 1;
        r_watersim_enabled = _Dvar_RegisterBool("r_watersim_enabled", 1, 0x5500u, "Enables dynamic water simulation");
        r_watersim_debug = _Dvar_RegisterBool("r_watersim_debug", 0, 0x4400u, "Enables bullet debug markers");
        r_watersim_flatten = _Dvar_RegisterBool("r_watersim_flatten", 0, 0x4400u, "Flattens the water surface out");
        r_watersim_waveSeedDelay = _Dvar_RegisterFloat(
                                                                 "r_watersim_waveSeedDelay",
                                                                 500.0,
                                                                 1.0,
                                                                 50000.0,
                                                                 0x4400u,
                                                                 "Time between seeding a new wave (ms)");
        r_watersim_curlAmount = _Dvar_RegisterFloat(
                                                            "r_watersim_curlAmount",
                                                            0.5,
                                                            0.0,
                                                            1.0,
                                                            0x4400u,
                                                            "Amount of curl applied");
        r_watersim_curlMax = _Dvar_RegisterFloat("r_watersim_curlMax", 0.40000001, 0.0, 8.0, 0x4400u, "Maximum curl limit");
        r_watersim_curlReduce = _Dvar_RegisterFloat(
                                                            "r_watersim_curlReduce",
                                                            0.94999999,
                                                            0.0,
                                                            1.0,
                                                            0x4400u,
                                                            "Amount curl gets reduced by when over limit");
        r_watersim_minShoreHeight = _Dvar_RegisterFloat(
                                                                    "r_watersim_minShoreHeight",
                                                                    0.039999999,
                                                                    -5.0,
                                                                    5.0,
                                                                    0x4400u,
                                                                    "Allows water to lap over the shoreline edge");
        r_watersim_foamAppear = _Dvar_RegisterFloat(
                                                            "r_watersim_foamAppear",
                                                            20.0,
                                                            0.0,
                                                            20.0,
                                                            0x4400u,
                                                            "Rate foam appears at");
        r_watersim_foamDisappear = _Dvar_RegisterFloat(
                                                                 "r_watersim_foamDisappear",
                                                                 0.77999997,
                                                                 0.0,
                                                                 1.0,
                                                                 0x4400u,
                                                                 "Rate foam disappears at");
        r_watersim_windAmount = _Dvar_RegisterFloat(
                                                            "r_watersim_windAmount",
                                                            0.02,
                                                            0.0,
                                                            0.2,
                                                            0x4400u,
                                                            "Amount of wind applied");
        r_watersim_windDir = _Dvar_RegisterFloat(
                                                     "r_watersim_windDir",
                                                     45.0,
                                                     0.0,
                                                     360.0,
                                                     0x4400u,
                                                     "Wind direction (degrees)");
        r_watersim_windMax = _Dvar_RegisterFloat("r_watersim_windMax", 0.40000001, 0.0, 4.0, 0x4400u, "Maximum wind limit");
        r_watersim_particleGravity = _Dvar_RegisterFloat(
                                                                     "r_watersim_particleGravity",
                                                                     0.029999999,
                                                                     0.0,
                                                                     0.5,
                                                                     0x4400u,
                                                                     "Particle gravity");
        r_watersim_particleLimit = _Dvar_RegisterFloat(
                                                                 "r_watersim_particleLimit",
                                                                 2.5,
                                                                 0.0,
                                                                 10.0,
                                                                 0x4400u,
                                                                 "Limit at which particles get spawned");
        r_watersim_particleLength = _Dvar_RegisterFloat(
                                                                    "r_watersim_particleLength",
                                                                    0.029999999,
                                                                    0.0,
                                                                    0.5,
                                                                    0x4400u,
                                                                    "Length of each particle");
        r_watersim_particleWidth = _Dvar_RegisterFloat(
                                                                 "r_watersim_particleWidth",
                                                                 2.0,
                                                                 0.0,
                                                                 8.0,
                                                                 0x4400u,
                                                                 "Width of each particle");
        r_watersim_scroll = _Dvar_RegisterVec2(
                                                    "r_watersim_scroll",
                                                    (0.0),
                                                    (0.0),
                                                    -10.0,
                                                    10.0,
                                                    0x4400u,
                                                    "XY coords to scroll water in");
        config.gridScale = 25.0f;
        config.waveSeedHeight = 25.0f;
        config.waveSeedRadius = 125.0f;
        config.waveDamping = 64.0f;
        memset((unsigned __int8 *)data.buffer[0].v, 0, data.buffer[0].bufferSize);
        memset((unsigned __int8 *)data.buffer[1].v, 0, data.buffer[1].bufferSize);
        memset((unsigned __int8 *)data.waterheight.v, 0, data.waterheight.bufferSize);
        memset((unsigned __int8 *)data.flooroffset.v, 0, data.flooroffset.bufferSize);
        memset((unsigned __int8 *)data.shoredist.v, 0, data.shoredist.bufferSize);
        memset((unsigned __int8 *)data.colors.v, 0, data.colors.bufferSize);
        expiredMeshes.tail = 0;
        expiredMeshes.head = 0;
        numFreeVertBlocks = 0;
        for ( n = 0; n < 0x320; ++n )
            freeVertBlocks[numFreeVertBlocks++] = 289 * n;
#if defined(__ANDROID__)
        waterUpdateMutex = bo1::android::InitializeWaterUpdateMutex();
#else
        waterUpdateMutex = CreateMutexA(0, 0, 0);
#endif
        data.sprayMaterial = Material_RegisterHandle((char*)"water_dynamic_spray", 6);
        R_WaterSimulationRestart();
    }
}

void __cdecl R_WaterSimulationRestart()
{
    int i; // [esp+4h] [ebp-8h]
    unsigned int n; // [esp+8h] [ebp-4h]

    data.waveTimeleft = 0.0f;
    data.timeDelta = 0;
    data.enabled = 0;
    data.localSurfaceDistance = 0.0f;
    data.localEyeHeight = -32768.0;
    data.speedScale = 1.0f;
    data.oldEyePos[0] = 0.0f;
    data.oldEyePos[1] = 0.0f;
    data.oldEyePos[2] = 0.0f;
    for ( n = 0; n < 0x100; ++n )
        ExpireAllMeshes(&data.tiles[n]);
    memset((unsigned __int8 *)&prevCmd, 0, sizeof(prevCmd));
    R_WaterSimulationTeleport(-559038737, -559038737);
    if ( rgp.world )
    {
        Dvar_SetFloat((dvar_s *)r_watersim_windDir, rgp.world->waterDirection);
        for ( i = 0; i < 2; ++i )
        {
            if ( rgp.world->waterBuffers[i].bufferSize )
                memcpy(
                    (unsigned __int8 *)data.buffer[i].v,
                    (unsigned __int8 *)rgp.world->waterBuffers[i].buffer,
                    rgp.world->waterBuffers[i].bufferSize);
        }
    }
}

void __cdecl ExpireAllMeshes(tile_t *tile)
{
    while ( ((tile->meshes.tail - tile->meshes.head) & 3) != 0 )
        ExpireMesh(tile);
}

void __cdecl ExpireMesh(tile_t *tile)
{
    meshExpire_t expire; // [esp+0h] [ebp-Ch] BYREF
    tilemesh_t *mesh; // [esp+8h] [ebp-4h]

    mesh = &tile->meshes.data[tile->meshes.head];
    if ( ((LOWORD(expiredMeshes.tail) - LOWORD(expiredMeshes.head)) & 0x3FF) == 0x3FF
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_water_sim.cpp",
                    951,
                    0,
                    "%s",
                    "expiredMeshes.free() > 0") )
    {
        __debugbreak();
    }
    expire.framesLeft = 3;
    expire.baseVertex = mesh->baseVertex;
    //fifo_t<meshExpire_t,1024>::add(&expiredMeshes, &expire);
    expiredMeshes.add(&expire);
    ++tile->meshes.head;
    tile->meshes.head &= 3u;
}

void __cdecl R_WaterSimulationTeleport(int posX, int posY)
{
    tile_t *tile; // [esp+0h] [ebp-10h]
    unsigned int x; // [esp+8h] [ebp-8h]
    unsigned int y; // [esp+Ch] [ebp-4h]

    data.gridX = posX;
    data.gridY = posY;
    data.centerU = 128;
    data.centerV = 128;
    data.frame = 15;
    for ( y = 0; y < 0x10; ++y )
    {
        for ( x = 0; x < 0x10; ++x )
        {
            tile = &data.tiles[16 * y + x];
            tile->occupied = 0;
            tile->lastUpdateTime = (x + 16 * y) >> 4;
            tile->srcU = 16 * x;
            tile->srcV = 16 * y;
            tile->worldX = 16 * x + data.gridX - 128;
            tile->worldY = 16 * y + data.gridY - 128;
            tile->worldZ = 0;
            ExpireAllMeshes(tile);
        }
    }
}

void __cdecl R_InitWaterSimulationBuffers(unsigned int location)
{

    if ( comWorld.numWaterCells )
    {
        if ( !buffersAllocated )
        {
            PMem_BeginAlloc("water buffers", location, TRACK_WATERSIM);
            //channel_t<float4>::Alloc(data.buffer, location);
            //channel_t<float4>::Alloc(&data.buffer[1], location);
            //channel_t<short>::Alloc(&data.waterheight, location);
            //channel_t<char>::Alloc(&data.flooroffset, location);
            //channel_t<char>::Alloc(&data.shoredist, location);
            //channel_t<GfxColor>::Alloc(&data.colors, location);
            data.buffer[0].Alloc(location);
            data.buffer[1].Alloc(location);
            data.waterheight.Alloc(location);
            data.flooroffset.Alloc(location);
            data.shoredist.Alloc(location);
            data.colors.Alloc(location);
            memset((unsigned __int8 *)data.buffer[0].v, 0, data.buffer[0].bufferSize);
            memset((unsigned __int8 *)data.buffer[1].v, 0, data.buffer[1].bufferSize);
            memset((unsigned __int8 *)data.waterheight.v, 0, data.waterheight.bufferSize);
            memset((unsigned __int8 *)data.flooroffset.v, 0, data.flooroffset.bufferSize);
            memset((unsigned __int8 *)data.shoredist.v, 0, data.shoredist.bufferSize);
            memset((unsigned __int8 *)data.colors.v, 0, data.colors.bufferSize);
            PMem_EndAlloc("water buffers", location);
            R_InitWaterSimulationVertexBuffers();
            buffersAllocated = 1;
        }
    }
    else
    {
        R_WaterSimulationSwitchOff();
    }
}

void __cdecl R_FreeWaterSimulationBuffers()
{
    if ( buffersAllocated )
    {
        R_FreeWaterSimulationVertexBuffers();
        PMem_Free("water buffers");
        //channel_t<GfxColor>::Free((channel_t<GfxColor> *)&data.waterheight);
        //channel_t<GfxColor>::Free((channel_t<GfxColor> *)&data.flooroffset);
        //channel_t<GfxColor>::Free((channel_t<GfxColor> *)&data.shoredist);
        //channel_t<GfxColor>::Free(&data.colors);
        //channel_t<GfxColor>::Free((channel_t<GfxColor> *)&data.buffer[1]);
        //channel_t<GfxColor>::Free((channel_t<GfxColor> *)data.buffer);
        data.waterheight.Free();
        data.flooroffset.Free(); 
        data.shoredist.Free();
        data.colors.Free();
        data.buffer[1].Free();
        data.buffer[0].Free();
        buffersAllocated = 0;
    }
}

void __cdecl R_SetWaterSimulationConstants(GfxCmdBufSourceState *state, float interp)
{
    if ( state->input.data->viewInfo->isRenderingFullScreen && data.enabled )
        R_UpdateCodeConstant(
            state,
            CONST_SRC_CODE_WATER_PARMS,
            0.0,
            interp,
            data.localSurfaceDistance,
            1.0 / (float)((float)((float)(222.72 * data.speedScale) * config.gridScale) / 2.0));
    else
        R_UpdateCodeConstant(state, CONST_SRC_CODE_WATER_PARMS, 0.0, interp, data.localSurfaceDistance, 100000.0);
    R_UpdateCodeConstant(state, CONST_SRC_CODE_WATER_SCROLL, r_watersim_scroll->current.value, r_watersim_scroll->current.vector[1], 0.0, 0.0);
}

#if 0
// local variable allocation has failed, the output may be wrong!
void    R_WaterSimulationRender(const float *eyePos, int g_msgTime, unsigned int viewIndex)
{
    float v4[3]; // [esp-Ch] [ebp-118h] BYREF
    float dw[3]; // [esp+0h] [ebp-10Ch] BYREF
    float dz[3]; // [esp+Ch] [ebp-100h] BYREF
    float dy[3]; // [esp+18h] [ebp-F4h] BYREF
    float v8[3]; // [esp+24h] [ebp-E8h] BYREF
    WaterSimInfo info; // [esp+30h] [ebp-DCh]
    debugpoint_t *querypos; // [esp+6Ch] [ebp-A0h]
    unsigned int k; // [esp+70h] [ebp-9Ch]
    tile_t *v12; // [esp+74h] [ebp-98h]
    unsigned int j; // [esp+78h] [ebp-94h]
    tile_t *v14; // [esp+7Ch] [ebp-90h]
    _BYTE tile[84]; // [esp+80h] [ebp-8Ch] OVERLAPPED BYREF
    unsigned int v16; // [esp+E0h] [ebp-2Ch]
    tile_t *v17; // [esp+E4h] [ebp-28h]
    unsigned int i; // [esp+E8h] [ebp-24h]
    unsigned int diff; // [esp+ECh] [ebp-20h]
    tile_t *slice; // [esp+F0h] [ebp-1Ch]
    int v21; // [esp+F4h] [ebp-18h]
    unsigned int prevFrame; // [esp+F8h] [ebp-14h]
    int frameTime; // [esp+FCh] [ebp-10h]
    //int v24; // [esp+100h] [ebp-Ch]
    //meshExpire_t *entry; // [esp+104h] [ebp-8h]
    //meshExpire_t *retaddr; // [esp+10Ch] [ebp+0h]

    //v24 = a1;
    //entry = retaddr;
    if ( r_watersim_enabled->current.enabled && comWorld.numWaterCells )
    {
        if ( buffersAllocated )
        {
            for ( frameTime = expiredMeshes.head; frameTime != expiredMeshes.tail; frameTime = ((_WORD)frameTime + 1) & 0x3FF )
            {
                prevFrame = 8 * frameTime + 181818728;
                if ( expiredMeshes.data[frameTime].framesLeft )
                {
                    if ( !--*(unsigned int *)prevFrame )
                        FreeMeshVerts(*(unsigned int *)(prevFrame + 4));
                }
            }
            while ( ((LOWORD(expiredMeshes.tail) - LOWORD(expiredMeshes.head)) & 0x3FF) != 0
                     && !expiredMeshes.data[expiredMeshes.head].framesLeft )
            {
                ++expiredMeshes.head;
                expiredMeshes.head = expiredMeshes.head & 0x3FF;
            }
            R_WaterSimulationUpdateConfig();
            R_WaterSimulationTrackView(eyePos);
            R_RenderWaterModel(viewIndex);
            data.enabled = 1;
            if ( g_msgTime < 0 )
                v21 = 0;
            else
                v21 = g_msgTime;
            data.timeDelta += v21;
            slice = (tile_t *)18;
            while ( data.timeDelta >= 18 )
            {
                data.timeDelta -= 18;
                diff = data.frame;
                if ( r_drawWater->current.enabled )
                    ++data.frame;
                for ( i = 0; i < 0x10; ++i )
                {
                    v17 = &data.tiles[16 * i];
                    v16 = data.frame - v17->lastUpdateTime;
                    if ( v16 >= 0x10 )
                    {
                        if ( NumMeshVertsFree() < 0x10 )
                        {
                            data.timeDelta = 0;
                            data.frame = diff;
                            return;
                        }
                        *(unsigned int *)tile = 16 * i;
                        if ( (data.frame & 0x10) != 0 )
                        {
                            *(unsigned int *)&tile[4] = 1;
                            *(unsigned int *)&tile[8] = 0;
                        }
                        else
                        {
                            *(unsigned int *)&tile[4] = 0;
                            *(unsigned int *)&tile[8] = 1;
                        }
                        *(unsigned int *)&tile[16] = prevCmd.update.destBuf;
                        *(unsigned int *)&tile[12] = prevCmd.update.startTile;
                        v14 = &data.tiles[prevCmd.update.startTile];
                        for ( j = 0; j < 0x10; ++j )
                        {
                            v12 = &v17[j];
                            v12->lastUpdateTime = data.frame;
                        }
                        for ( k = 0; k < 0x10; ++k )
                        {
                            for ( querypos = (debugpoint_t *)&v14[k]; ; ExpireMesh((tile_t *)querypos) )
                            {
                                LODWORD(info.waveVelocity[2]) = &querypos[4].pos[1];
                                if ( ((LODWORD(querypos[4].pos[2]) - LODWORD(querypos[4].pos[1])) & 3u) < 2 )
                                    break;
                            }
                            *(unsigned int *)&tile[4 * k + 20] = (char *)querypos + 32 * LODWORD(querypos[4].pos[2])++ + 60;
                            LODWORD(querypos[4].pos[2]) &= 3u;
                            *(unsigned int *)(*(unsigned int *)&tile[4 * k + 20] + 20) = data.frame;
                            **(unsigned int **)&tile[4 * k + 20] = LODWORD(querypos->pos[0]);
                            *(float *)(*(unsigned int *)&tile[4 * k + 20] + 4) = querypos->pos[1];
                            *(float *)(*(unsigned int *)&tile[4 * k + 20] + 8) = querypos->pos[2];
                            *(float *)(*(unsigned int *)&tile[4 * k + 20] + 12) = querypos[1].pos[0];
                            *(float *)(*(unsigned int *)&tile[4 * k + 20] + 16) = querypos[1].pos[1];
                            *(unsigned int *)(*(unsigned int *)&tile[4 * k + 20] + 24) = AllocMeshVerts();
                            *(unsigned int *)(*(unsigned int *)&tile[4 * k + 20] + 28) = &data.lockedData[*(unsigned int *)(*(unsigned int *)&tile[4 * k + 20]
                                                                                                                                                                                    + 24)];
                        }
                        Sys_AddWorkerCmdInternal(&r_water_simWorkerCmd, tile, 0);
                        memcpy(&prevCmd, tile, sizeof(prevCmd));
                    }
                }
            }
            if ( r_watersim_debug->current.enabled )
            {
                for ( LODWORD(info.waveVelocity[1]) = debugPoints.head;
                            LODWORD(info.waveVelocity[1]) != debugPoints.tail;
                            LODWORD(info.waveVelocity[1]) = (LOBYTE(info.waveVelocity[1]) + 1) & 0xF )
                {
                    LODWORD(info.waveVelocity[0]) = debugPoints.data[LODWORD(info.waveVelocity[1])].pos;
                    R_WaterSimulationGetInfo((const float *)LODWORD(info.waveVelocity[0]), (WaterSimInfo *)v8);
                    dy[0] = (float)(10.0 * info.pos[0]) + v8[0];
                    dy[1] = (float)(10.0 * info.pos[1]) + v8[1];
                    dy[2] = (float)(10.0 * info.pos[2]) + v8[2];
                    dz[0] = (float)(10.0 * info.tangent[0]) + v8[0];
                    dz[1] = (float)(10.0 * info.tangent[1]) + v8[1];
                    dz[2] = (float)(10.0 * info.tangent[2]) + v8[2];
                    dw[0] = (float)(10.0 * info.binormal[0]) + v8[0];
                    dw[1] = (float)(10.0 * info.binormal[1]) + v8[1];
                    dw[2] = (float)(10.0 * info.binormal[2]) + v8[2];
                    v4[0] = (float)(1.0 * info.normal[0]) + v8[0];
                    v4[1] = (float)(1.0 * info.normal[1]) + v8[1];
                    v4[2] = (float)(1.0 * info.normal[2]) + v8[2];
                    R_AddDebugLine(&frontEndDataOut->debugGlobals, v8, dy, colorRed, 0);
                    R_AddDebugLine(&frontEndDataOut->debugGlobals, v8, dz, colorGreen, 0);
                    R_AddDebugLine(&frontEndDataOut->debugGlobals, v8, dw, colorBlue, 0);
                    R_AddDebugLine(&frontEndDataOut->debugGlobals, v8, v4, colorYellow, 0);
                }
            }
        }
    }
    else
    {
        R_WaterSimulationSwitchOff();
    }
}
#endif

// Typed scheduler follows the original row and mesh FIFO transitions.
void R_WaterSimulationRender(const float *eyePos, int time, unsigned int viewIndex)
{
    unsigned int frameTime;
    unsigned int diff;
    unsigned int i, j, k;
    int stepTime;

    if (!r_watersim_enabled->current.enabled || !comWorld.numWaterCells)
    {
        R_WaterSimulationSwitchOff();
        return;
    }

    if (!buffersAllocated)
        return;

    /* ------------------------------------------------------------ */
    /* Expire old meshes                                             */
    /* ------------------------------------------------------------ */
    for (frameTime = expiredMeshes.head;
        frameTime != expiredMeshes.tail;
        frameTime = (frameTime + 1) & 0x3FF)
    {
        meshExpire_t *e = &expiredMeshes.data[frameTime];

        if (e->framesLeft && !--e->framesLeft)
            FreeMeshVerts(e->baseVertex);
    }

    while (((expiredMeshes.tail - expiredMeshes.head) & 0x3FF) &&
        !expiredMeshes.data[expiredMeshes.head].framesLeft)
    {
        expiredMeshes.head = (expiredMeshes.head + 1) & 0x3FF;
    }

    /* ------------------------------------------------------------ */
    /* Per-frame setup                                               */
    /* ------------------------------------------------------------ */
    R_WaterSimulationUpdateConfig();
    R_WaterSimulationTrackView(eyePos);
    R_RenderWaterModel(viewIndex);

    data.enabled = true;

    stepTime = (time < 0) ? 0 : time;
    data.timeDelta += stepTime;

    /* ------------------------------------------------------------ */
    /* Simulation steps (18 ms fixed timestep)                       */
    /* ------------------------------------------------------------ */
    while (data.timeDelta >= 18)
    {
        data.timeDelta -= 18;
        diff = data.frame;

        if (r_drawWater->current.enabled)
            ++data.frame;

        for (i = 0; i < 16; ++i)
        {
            tile_t *row = &data.tiles[i * 16];
            unsigned int age = data.frame - row->lastUpdateTime;

            if (age < 16)
                continue;

            if (NumMeshVertsFree() < 16)
            {
                data.timeDelta = 0;
                data.frame = diff;
                return;
            }

            WaterSimulationCmd cmd;
            memset(&cmd, 0, sizeof(cmd));

            cmd.update.startTile = 16 * i;

            if (data.frame & 0x10)
            {
                cmd.update.srcBuf = 1;
                cmd.update.destBuf = 0;
            }
            else
            {
                cmd.update.srcBuf = 0;
                cmd.update.destBuf = 1;
            }

            cmd.skin.startTile = prevCmd.update.startTile;
            cmd.skin.srcBuf = prevCmd.update.destBuf;

            tile_t *srcRow = &data.tiles[cmd.skin.startTile];

            /* mark tiles updated */
            for (j = 0; j < 16; ++j)
                row[j].lastUpdateTime = data.frame;

            /* allocate meshes */
            for (k = 0; k < 16; ++k)
            {
                tile_t *tile = &srcRow[k];
                tilemesh_t *mesh;

                for (;;)
                {
                    mesh = &tile->meshes.data[tile->meshes.tail];

                    if (((tile->meshes.tail - tile->meshes.head) & 3) < 2)
                        break;

                    ExpireMesh(tile);
                }

                tile->meshes.tail = (tile->meshes.tail + 1) & 3;

                mesh->srcU = tile->srcU;
                mesh->srcV = tile->srcV;
                mesh->worldX = tile->worldX;
                mesh->worldY = tile->worldY;
                mesh->worldZ = tile->worldZ;
                mesh->lastSkinTime = data.frame;
                mesh->baseVertex = AllocMeshVerts();
                mesh->lockedData = &data.lockedData[mesh->baseVertex];

                cmd.skin.meshes[k] = mesh;
            }

            Sys_AddWorkerCmdInternal(&r_water_simWorkerCmd, (unsigned char*)&cmd, 0);
            prevCmd = cmd;
        }
    }

    /* ------------------------------------------------------------ */
    /* Debug draw                                                    */
    /* ------------------------------------------------------------ */
    if (r_watersim_debug->current.enabled)
    {
        float base[3], a[3], b[3], c[3], d[3];
        WaterSimInfo info;

        for (unsigned int n = debugPoints.head;
            n != debugPoints.tail;
            n = (n + 1) & 0xF)
        {
            const float *p = debugPoints.data[n].pos;

            R_WaterSimulationGetInfo(p, &info);

            base[0] = p[0];
            base[1] = p[1];
            base[2] = p[2];

            for (i = 0; i < 3; ++i)
            {
                a[i] = base[i] + 10.0f * info.pos[i];
                b[i] = base[i] + 10.0f * info.tangent[i];
                c[i] = base[i] + 10.0f * info.binormal[i];
                d[i] = base[i] + 1.0f * info.normal[i];
            }

            R_AddDebugLine(&frontEndDataOut->debugGlobals, base, a, colorRed, 0);
            R_AddDebugLine(&frontEndDataOut->debugGlobals, base, b, colorGreen, 0);
            R_AddDebugLine(&frontEndDataOut->debugGlobals, base, c, colorBlue, 0);
            R_AddDebugLine(&frontEndDataOut->debugGlobals, base, d, colorYellow, 0);
        }
    }
}

#if 0
void __cdecl R_WaterSimulationTrackView(const float *eyePos)
{
    double v1; // xmm0_8
    long double v2; // [esp+0h] [ebp-54h]
    long double v3; // [esp+8h] [ebp-4Ch]
    tile_t *tile; // [esp+20h] [ebp-34h]
    int newY; // [esp+24h] [ebp-30h]
    int newX; // [esp+2Ch] [ebp-28h]
    unsigned int x; // [esp+30h] [ebp-24h]
    unsigned int y; // [esp+34h] [ebp-20h]
    unsigned int n; // [esp+38h] [ebp-1Ch]
    int v10; // [esp+3Ch] [ebp-18h]
    float d; // [esp+40h] [ebp-14h]
    int newTileY; // [esp+44h] [ebp-10h]
    int newTileX; // [esp+48h] [ebp-Ch]
    int dy; // [esp+4Ch] [ebp-8h]

    newTileX = (int)(float)(*eyePos / (float)(config.gridScale * 16.0));
    newTileY = (int)(float)(eyePos[1] / (float)(config.gridScale * 16.0));
    v10 = data.gridX / 16 - newTileX;
    dy = data.gridY / 16 - newTileY;
    if ( (int)abs(v10) < 16 && (int)abs(dy) < 16 )
    {
        *((float *)&v2 + 1) = data.localEyeHeight * 7.0 / 8.0;
        data.localEyeHeight = R_WaterSimulationGetBaseHeight(eyePos) * 1.0 / 8.0 + *((float *)&v2 + 1);
        data.localSurfaceDistance = data.localEyeHeight - eyePos[2];
        d = Vec3Distance(eyePos, data.oldEyePos);
        *(float *)&v3 = d / (float)((float)(16.0 * config.gridScale) / 16.0);
        if ( (float)(*(float *)&v3 - 1.0) < 0.0 )
            *((float *)&v3 + 1) = d / (float)((float)(16.0 * config.gridScale) / 16.0);
        else
            *((float *)&v3 + 1) = 1.0f;
        if ( (float)(0.0 - *(float *)&v3) < 0.0 )
            LODWORD(v2) = HIDWORD(v3);
        else
            LODWORD(v2) = 0;
        v1 = (float)(1.0 - *(float *)&v2);
        __libm_sse2_pow(v2, v3);
        *(float *)&v1 = v1;
        data.speedScale = (float)((float)(*(float *)&v1 * 1.0) / 32.0) + (float)((float)(data.speedScale * 31.0) / 32.0);
        data.oldEyePos[0] = *eyePos;
        data.oldEyePos[1] = eyePos[1];
        data.oldEyePos[2] = eyePos[2];
        if ( v10 || dy )
        {
            data.gridX -= 16 * v10;
            data.gridY -= 16 * dy;
            data.centerU -= 16 * v10;
            data.centerV -= 16 * dy;
            data.centerU = LOBYTE(data.centerU);
            data.centerV = LOBYTE(data.centerV);
            for ( y = 0; y < 0x10; ++y )
            {
                for ( x = 0; x < 0x10; ++x )
                {
                    tile = &data.tiles[16 * y + x];
                    newX = tile->worldX;
                    newY = tile->worldY;
                    if ( newX < data.gridX - 128 )
                        newX = tile->worldX + 256;
                    if ( tile->worldY < data.gridY - 128 )
                        newY = tile->worldY + 256;
                    if ( tile->worldX >= data.gridX + 128 )
                        newX = tile->worldX - 256;
                    if ( tile->worldY >= data.gridY + 128 )
                        newY = tile->worldY - 256;
                    if ( newX != tile->worldX || newY != tile->worldY )
                    {
                        tile->worldX = newX;
                        tile->worldY = newY;
                        ExpireAllMeshes(tile);
                        tile->occupied = R_WaterSimulationFillTile(tile);
                    }
                }
            }
        }
    }
    else
    {
        R_WaterSimulationTeleport(16 * newTileX, 16 * newTileY);
        for ( n = 0; n < 0x100; ++n )
            data.tiles[n].occupied = R_WaterSimulationFillTile(&data.tiles[n]);
    }
}
#endif

// Preserve retail smoothing, movement easing and tile relocation.
void R_WaterSimulationTrackView(const float *eyePos)
{
    int newTileX, newTileY;
    int dx, dy;
    float t;
    float oneMinusT;

    /* determine which tile the eye is in */
    const float inverseTileSize = 1.0f / (config.gridScale * 16.0f);
    newTileX = (int)(eyePos[0] * inverseTileSize);
    newTileY = (int)(eyePos[1] * inverseTileSize);

    dx = data.gridX / 16 - newTileX;
    dy = data.gridY / 16 - newTileY;

    /* normal tracking path */
    if (abs(dx) < 16 && abs(dy) < 16)
    {
        /* smooth local eye height */
        data.localEyeHeight =
            data.localEyeHeight * (7.0f / 8.0f) +
            R_WaterSimulationGetBaseHeight(eyePos) * (1.0f / 8.0f);

        data.localSurfaceDistance = data.localEyeHeight - eyePos[2];

        // Retail sums X/Z/Y, then keeps the sqrt/divide in double precision
        // until the normalized movement is stored as a float.
        const float movementX = data.oldEyePos[0] - eyePos[0];
        const float movementY = data.oldEyePos[1] - eyePos[1];
        const float movementZ = data.oldEyePos[2] - eyePos[2];
        const float distanceSquared = (movementX * movementX + movementZ * movementZ)
                                    + movementY * movementY;
        t = static_cast<float>(sqrt(static_cast<double>(distanceSquared))
                             / static_cast<double>(config.gridScale));

        /* clamp to [0, 1] */
        if (t > 1.0f)
            t = 1.0f;
        if (t < 0.0f)
            t = 0.0f;

        /* easing curve (original SSE pow) */
        oneMinusT = static_cast<float>(pow(static_cast<double>(1.0f - t), 0.5));

        /* exponential smoothing */
        data.speedScale =
            (oneMinusT * 1.0f) / 32.0f +
            (data.speedScale * 31.0f) / 32.0f;

        /* update previous eye position */
        data.oldEyePos[0] = eyePos[0];
        data.oldEyePos[1] = eyePos[1];
        data.oldEyePos[2] = eyePos[2];

        /* grid shifted */
        if (dx || dy)
        {
            unsigned int x, y;

            data.gridX -= 16 * dx;
            data.gridY -= 16 * dy;
            data.centerU -= 16 * dx;
            data.centerV -= 16 * dy;

            /* wrap to byte */
            data.centerU &= 0xFF;
            data.centerV &= 0xFF;

            for (y = 0; y < 16; ++y)
            {
                for (x = 0; x < 16; ++x)
                {
                    tile_t *tile = &data.tiles[y * 16 + x];
                    int newX = tile->worldX;
                    int newY = tile->worldY;

                    if (newX < data.gridX - 128)
                        newX += 256;
                    if (newX >= data.gridX + 128)
                        newX -= 256;

                    if (newY < data.gridY - 128)
                        newY += 256;
                    if (newY >= data.gridY + 128)
                        newY -= 256;

                    if (newX != tile->worldX || newY != tile->worldY)
                    {
                        tile->worldX = newX;
                        tile->worldY = newY;
                        ExpireAllMeshes(tile);
                        tile->occupied = R_WaterSimulationFillTile(tile);
                    }
                }
            }
        }
    }
    /* teleport / large jump */
    else
    {
        unsigned int i;

        R_WaterSimulationTeleport(16 * newTileX, 16 * newTileY);

        for (i = 0; i < 256; ++i)
            data.tiles[i].occupied =
            R_WaterSimulationFillTile(&data.tiles[i]);
    }
}

bool __cdecl R_WaterSimulationFillTile(tile_t *tile)
{
    float v2; // [esp+8h] [ebp-A0h]
    float v3; // [esp+Ch] [ebp-9Ch]
    float v4; // [esp+10h] [ebp-98h]
    float v5; // [esp+14h] [ebp-94h]
    float v6; // [esp+18h] [ebp-90h]
    float v7; // [esp+1Ch] [ebp-8Ch]
    float v8; // [esp+20h] [ebp-88h]
    int c; // [esp+28h] [ebp-80h]
    ComWaterCell cell; // [esp+30h] [ebp-78h]
    float w2; // [esp+38h] [ebp-70h]
    float f0_x; // [esp+3Ch] [ebp-6Ch]
    float w0; // [esp+40h] [ebp-68h]
    float f1_y; // [esp+44h] [ebp-64h]
    float w1; // [esp+48h] [ebp-60h]
    float f1_x; // [esp+4Ch] [ebp-5Ch]
    float f0_y; // [esp+50h] [ebp-58h]
    float w3; // [esp+54h] [ebp-54h]
    int tx; // [esp+58h] [ebp-50h]
    char *p_shoredist; // [esp+5Ch] [ebp-4Ch]
    __int16 *p_waterheight; // [esp+60h] [ebp-48h]
    float4 *p_buf1; // [esp+64h] [ebp-44h]
    GfxColor *p_colors; // [esp+68h] [ebp-40h]
    char *p_flooroffset; // [esp+6Ch] [ebp-3Ch]
    float4 *p_buf0; // [esp+70h] [ebp-38h]
    int ty; // [esp+74h] [ebp-34h]
    ComWaterCell *cells[4]; // [esp+78h] [ebp-30h] BYREF
    float4 zero; // [esp+88h] [ebp-20h] BYREF
    ComWaterCell dummy; // [esp+98h] [ebp-10h] BYREF
    bool valid; // [esp+A7h] [ebp-1h]

    valid = R_WaterFindCells(tile->worldX, tile->worldY, cells);
    if ( !valid )
    {
        cells[3] = &dummy;
        cells[2] = &dummy;
        cells[1] = &dummy;
        cells[0] = &dummy;
        dummy.color[0] = -1;
        dummy.color[1] = -1;
        dummy.color[2] = -1;
        dummy.color[3] = -1;
        dummy.shoredist = 0;
        dummy.waterheight = 0x8000;
        dummy.flooroffset = 0;
    }
    if ( config.gridScale != 25.0
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_water_sim.cpp",
                    533,
                    0,
                    "%s",
                    "config.gridScale == WATER_GRID_SPACING") )
    {
        __debugbreak();
    }
    memset(&zero, 0, sizeof(zero));
    for ( ty = 0; ty < 16; ++ty )
    {
        p_waterheight = &data.waterheight.v[256 * (ty + tile->srcV) + tile->srcU];
        p_flooroffset = &data.flooroffset.v[256 * (ty + tile->srcV) + tile->srcU];
        p_shoredist = &data.shoredist.v[256 * (ty + tile->srcV) + tile->srcU];
        p_colors = &data.colors.v[256 * (ty + tile->srcV) + tile->srcU];
        p_buf0 = &data.buffer[0].v[256 * (ty + tile->srcV) + tile->srcU];
        p_buf1 = &data.buffer[1].v[256 * (ty + tile->srcV) + tile->srcU];
        for ( tx = 0; tx < 16; ++tx )
        {
            f0_x = (float)tx / 16.0;
            f0_y = (float)ty / 16.0;
            f1_x = 1.0 - f0_x;
            f1_y = 1.0 - f0_y;
            w0 = (float)(1.0 - f0_x) * (float)(1.0 - f0_y);
            w1 = f0_x * (float)(1.0 - f0_y);
            w2 = (float)(1.0 - f0_x) * f0_y;
            w3 = f0_x * f0_y;
            cell.waterheight = (int)(float)((float)((float)((float)((float)cells[0]->waterheight * w0)
                                                                                                        + (float)((float)cells[1]->waterheight * w1))
                                                                                        + (float)((float)cells[2]->waterheight * w2))
                                                                        + (float)((float)cells[3]->waterheight * (float)(f0_x * f0_y)));
            cell.flooroffset = (int)(float)((float)((float)((float)((float)cells[0]->flooroffset * w0)
                                                                                                        + (float)((float)cells[1]->flooroffset * w1))
                                                                                        + (float)((float)cells[2]->flooroffset * w2))
                                                                        + (float)((float)cells[3]->flooroffset * (float)(f0_x * f0_y)));
            cell.shoredist = (int)(float)((float)((float)((float)((float)cells[0]->shoredist * w0)
                                                                                                    + (float)((float)cells[1]->shoredist * w1))
                                                                                    + (float)((float)cells[2]->shoredist * w2))
                                                                    + (float)((float)cells[3]->shoredist * (float)(f0_x * f0_y)));
            cell.color[0] = (int)(float)((float)((float)((float)((float)cells[0]->color[0] * w0)
                                                                                                 + (float)((float)cells[1]->color[0] * w1))
                                                                                 + (float)((float)cells[2]->color[0] * w2))
                                                                 + (float)((float)cells[3]->color[0] * (float)(f0_x * f0_y)));
            cell.color[1] = (int)(float)((float)((float)((float)((float)cells[0]->color[1] * w0)
                                                                                                 + (float)((float)cells[1]->color[1] * w1))
                                                                                 + (float)((float)cells[2]->color[1] * w2))
                                                                 + (float)((float)cells[3]->color[1] * (float)(f0_x * f0_y)));
            cell.color[2] = (int)(float)((float)((float)((float)((float)cells[0]->color[2] * w0)
                                                                                                 + (float)((float)cells[1]->color[2] * w1))
                                                                                 + (float)((float)cells[2]->color[2] * w2))
                                                                 + (float)((float)cells[3]->color[2] * (float)(f0_x * f0_y)));
            cell.color[3] = (int)(float)((float)((float)((float)((float)cells[0]->color[3] * w0)
                                                                                                 + (float)((float)cells[1]->color[3] * w1))
                                                                                 + (float)((float)cells[2]->color[3] * w2))
                                                                 + (float)((float)cells[3]->color[3] * (float)(f0_x * f0_y)));
            if ( !tx && !ty )
            {
                v4 = floor((float)((float)cell.waterheight / config.gridScale));
                tile->worldZ = (__int16)(int)v4;
            }
            *p_waterheight = cell.waterheight;
            v7 = (float)cell.flooroffset / config.gridScale;
            if ( (float)(v7 - 255.0) < 0.0 )
                v8 = (float)cell.flooroffset / config.gridScale;
            else
                v8 = 255.0f;
            if ( (float)(0.0 - v7) < 0.0 )
                v3 = v8;
            else
                v3 = 0.0f;
            *p_flooroffset = (int)v3;
            v5 = (float)(16 * cell.shoredist) * config.gridScale;
            if ( (float)(v5 - 255.0) < 0.0 )
                v6 = (float)(16 * cell.shoredist) * config.gridScale;
            else
                v6 = 255.0f;
            if ( (float)(0.0 - v5) < 0.0 )
                v2 = v6;
            else
                v2 = 0.0f;
            *p_shoredist = (int)v2;
            if ( (float)*p_flooroffset > p_buf0->v[1] || (float)*p_flooroffset > p_buf1->v[1] )
            {
                *p_buf0 = zero;
                *p_buf1 = zero;
            }
            for ( c = 0; c < 4; ++c )
                p_colors->array[c] = cell.color[c];
            ++p_colors;
            ++p_buf0;
            ++p_buf1;
            ++p_waterheight;
            ++p_flooroffset;
            ++p_shoredist;
        }
    }
    tile->minMax[0][0] = (float)(tile->worldX - 8) * config.gridScale;
    tile->minMax[0][1] = (float)(tile->worldY - 8) * config.gridScale;
    tile->minMax[0][2] = (float)(tile->worldZ - 8) * config.gridScale;
    tile->minMax[1][0] = (float)(tile->worldX + 24) * config.gridScale;
    tile->minMax[1][1] = (float)(tile->worldY + 24) * config.gridScale;
    tile->minMax[1][2] = (float)(tile->worldZ + 24) * config.gridScale;
    return valid;
}

char __cdecl R_WaterFindCells(int gridX, int gridY, ComWaterCell **cells)
{
    ComWaterCell *v4; // [esp+0h] [ebp-1Ch]
    ComWaterCell *v5; // [esp+4h] [ebp-18h]
    ComWaterCell *v6; // [esp+8h] [ebp-14h]
    int cellY; // [esp+Ch] [ebp-10h]
    int cellX; // [esp+10h] [ebp-Ch]
    int h; // [esp+14h] [ebp-8h]
    int w; // [esp+18h] [ebp-4h]

    if ( !comWorld.numWaterCells )
        return 0;
    w = comWorld.waterHeader.maxx - comWorld.waterHeader.minx;
    h = comWorld.waterHeader.maxy - comWorld.waterHeader.miny;
    cellX = (gridX - 16 * comWorld.waterHeader.minx) >> 4;
    cellY = (gridY - 16 * comWorld.waterHeader.miny) >> 4;
    if ( cellX < 0 || cellY < 0 || cellX >= w || cellY >= h )
        return 0;
    *cells = &comWorld.waterCells[cellX + w * cellY];
    if ( cellX >= w - 1 )
        v6 = *cells;
    else
        v6 = &comWorld.waterCells[w * cellY + 1 + cellX];
    cells[1] = v6;
    if ( cellY >= h - 1 )
        v5 = *cells;
    else
        v5 = &comWorld.waterCells[cellX + w * (cellY + 1)];
    cells[2] = v5;
    if ( cellX >= w - 1 || cellY >= h - 1 )
        v4 = *cells;
    else
        v4 = &comWorld.waterCells[w * (cellY + 1) + 1 + cellX];
    cells[3] = v4;
    return 1;
}

unsigned int __cdecl AllocMeshVerts()
{
    if ( numFreeVertBlocks )
    {
        return freeVertBlocks[--numFreeVertBlocks];
    }
    else
    {
        if ( !Assert_MyHandler(
                        "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_water_sim.cpp",
                        1179,
                        0,
                        "No free vertex blocks for water.") )
            __debugbreak();
        return 0;
    }
}

unsigned int __cdecl NumMeshVertsFree()
{
    return numFreeVertBlocks;
}

void __cdecl FreeMeshVerts(unsigned int baseVertex)
{
    if ( numFreeVertBlocks >= 0x320
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_water_sim.cpp",
                    1190,
                    0,
                    "%s",
                    "numFreeVertBlocks < ARRAY_COUNT( freeVertBlocks )") )
    {
        __debugbreak();
    }
    freeVertBlocks[numFreeVertBlocks++] = baseVertex;
}

// aislop
void R_WaterSimulationUpdateConfig()
{
    // Convert all dvars into the config struct
    config.waveSeedDelay = r_watersim_waveSeedDelay->current.value;
    config.curlAmount = r_watersim_curlAmount->current.value;
    config.curlMax = r_watersim_curlMax->current.value;
    config.curlReduce = r_watersim_curlReduce->current.value;
    config.minShoreHeight = r_watersim_minShoreHeight->current.value;
    config.foamAppear = r_watersim_foamAppear->current.value;
    config.foamDisappear = r_watersim_foamDisappear->current.value;
    config.windAmount = r_watersim_windAmount->current.value;
    config.windMax = r_watersim_windMax->current.value;
    config.particleGravity = r_watersim_particleGravity->current.value;
    config.particleLimit = r_watersim_particleLimit->current.value;
    config.particleLength = r_watersim_particleLength->current.value;
    config.particleWidth = r_watersim_particleWidth->current.value;

    // Convert wind direction from degrees to radians
    float windDirRadians = r_watersim_windDir->current.value * (3.14159265f / 180.0f);

    // The simulation uses vertical Y and horizontal X/Z (retail config layout).
    config.windDir[0] = cosf(windDirRadians);
    config.windDir[1] = 0.0f;
    config.windDir[2] = sinf(windDirRadians);
}


void __cdecl R_RenderWaterModel(unsigned int viewIndex)
{
    if (!s_world.waterMaterial)
        return;

    float lodOrigin[3];
    for (unsigned int axis = 0; axis < 3; ++axis)
        lodOrigin[axis] = rg.correctedLodParms[viewIndex].origin[axis];
    lodOrigin[0] -= 8.0f * config.gridScale;
    lodOrigin[1] -= 8.0f * config.gridScale;

    for (unsigned int n = 0; n < 256; ++n)
    {
        tile_t *tile = &data.tiles[n];
        if (!tile->occupied || ((tile->meshes.tail - tile->meshes.head) & 3) < 2
            || R_CullBoxCurDpvs(tile->minMax[0], viewIndex))
            continue;

        const tilemesh_t *mesh0 = &tile->meshes.data[tile->meshes.head];
        const tilemesh_t *mesh1 = &tile->meshes.data[(tile->meshes.head + 1) & 3];
        GfxModelWaterSurface *modelSurf = reinterpret_cast<GfxModelWaterSurface *>(
            R_SurfaceAlloc(sizeof(GfxModelWaterSurface)));
        if (!modelSurf)
            return;

        modelSurf->interpolate = (double)(data.frame - mesh1->lastSkinTime) / 16.0;
        modelSurf->vb0 = data.vertexBuffer;
        modelSurf->vb1 = data.vertexBuffer;
        modelSurf->baseVertex0 = mesh0->baseVertex;
        modelSurf->baseVertex1 = mesh1->baseVertex;
        modelSurf->origin[0] = tile->worldX * config.gridScale;
        modelSurf->origin[1] = tile->worldY * config.gridScale;
        modelSurf->origin[2] = tile->worldZ * config.gridScale;
        const bool lod = Vec3DistanceSq(modelSurf->origin, lodOrigin) >= 2250000.0f;
        modelSurf->ib = data.indexBuffer[lod];
        modelSurf->vertCount = 289;
        modelSurf->triCount = data.tileTriCount[lod];

        // The draw key stores a bounded arena offset in four-byte units, not a pointer.
        const unsigned int surfId = static_cast<unsigned int>(
            reinterpret_cast<char *>(modelSurf) - reinterpret_cast<char *>(frontEndDataOut));
        iassert(!(surfId & 3));
        GfxDrawSurf drawSurf;
        HIDWORD(drawSurf.packed) = ((((s_world.waterMaterial->info.drawSurf.packed >> 58) & 0x3F) - 1) << 26)
            | HIDWORD(s_world.waterMaterial->info.drawSurf.packed) & 0x38007FF | 0x500000;
        LODWORD(drawSurf.packed) = (unsigned __int16)(surfId >> 2)
            | LODWORD(s_world.waterMaterial->info.drawSurf.packed) & 0xF00F0000 | 0x100000;
        scene.drawSurfs[1][scene.drawSurfCount[1]++] = drawSurf;
    }
}

char *__cdecl R_SurfaceAlloc(unsigned int size)
{
    // Other surface producers can leave a four-byte boundary. Water surfaces contain
    // native COM pointers, so reserve padding and the object together atomically.
    const unsigned int alignment = alignof(GfxModelWaterSurface);
    const unsigned int capacity = sizeof(frontEndDataOut->surfsBuffer);
    unsigned int previous = frontEndDataOut->surfPos;
    for (;;)
    {
        if (previous > capacity || size > capacity)
            break;
        const unsigned int start = (previous + alignment - 1) & ~(alignment - 1);
        if (start > capacity || size > capacity - start)
            break;
        const unsigned int next = start + size;
        const unsigned int observed = _InterlockedCompareExchange(
            &frontEndDataOut->surfPos, next, previous);
        if (observed == previous)
            return reinterpret_cast<char *>(frontEndDataOut->surfsBuffer) + start;
        previous = observed;
    }
    R_WarnOncePerFrame(R_WARN_MAX_SCENE_SURFS_SIZE);
    return nullptr;
}

void __cdecl R_WaterSimulationSwitchOff()
{
    data.enabled = 0;
}

void FlushBuffers()
{
    for (unsigned int i = 0; i < 4; ++i)
    {
        data.buffer[0].Cache(0, false, true);
        data.buffer[1].Cache(0, false, true);
        data.waterheight.Cache(0, false, true);
        data.flooroffset.Cache(0, false, true);
        data.shoredist.Cache(0, false, true);
        data.colors.Cache(0, false, true);
    }
}

#if 0
// local variable allocation has failed, the output may be wrong!
void    R_WaterSimulationUpdateFrame(const WaterSimulationCmd *cmd)
{
    char *v2; // eax
    unsigned int v3; // xmm0_4
    float v4; // xmm0_4
    float v5; // xmm0_4
    float v6; // xmm0_4
    float v7; // xmm0_4
    const float4 *v8; // edx
    float foamadd_12; // [esp+7Ch] [ebp-4C4h]
    float foamadd_12a; // [esp+7Ch] [ebp-4C4h]
    float v11; // [esp+80h] [ebp-4C0h]
    float v12; // [esp+80h] [ebp-4C0h]
    float v13; // [esp+84h] [ebp-4BCh]
    float v14; // [esp+84h] [ebp-4BCh]
    float d; // [esp+88h] [ebp-4B8h]
    unsigned int v16[4]; // [esp+A4h] [ebp-49Ch]
    __int64 v17; // [esp+B4h] [ebp-48Ch]
    unsigned int v18; // [esp+BCh] [ebp-484h]
    unsigned int v19; // [esp+C0h] [ebp-480h]
    float *v20; // [esp+C4h] [ebp-47Ch]
    __int64 v21; // [esp+C8h] [ebp-478h]
    int v22; // [esp+D0h] [ebp-470h]
    int v23; // [esp+D4h] [ebp-46Ch]
    float v24[11]; // [esp+D8h] [ebp-468h]
    float4 tiltz; // [esp+104h] [ebp-43Ch]
    float4 tiltx; // [esp+114h] [ebp-42Ch]
    float v27; // [esp+124h] [ebp-41Ch]
    float v28; // [esp+128h] [ebp-418h]
    float v29; // [esp+12Ch] [ebp-414h]
    float v30; // [esp+130h] [ebp-410h]
    float v31; // [esp+134h] [ebp-40Ch]
    float4 tilt; // [esp+138h] [ebp-408h]
    float v33; // [esp+148h] [ebp-3F8h]
    unsigned int v34; // [esp+14Ch] [ebp-3F4h]
    float v35; // [esp+150h] [ebp-3F0h]
    float v36; // [esp+154h] [ebp-3ECh]
    unsigned int v37; // [esp+158h] [ebp-3E8h]
    float v38; // [esp+15Ch] [ebp-3E4h]
    unsigned int v39; // [esp+160h] [ebp-3E0h]
    unsigned int v40; // [esp+164h] [ebp-3DCh]
    float v41; // [esp+168h] [ebp-3D8h]
    float v42; // [esp+16Ch] [ebp-3D4h]
    float4 clampedY; // [esp+170h] [ebp-3D0h]
    float v44; // [esp+180h] [ebp-3C0h] BYREF
    float v45; // [esp+184h] [ebp-3BCh]
    float v46; // [esp+188h] [ebp-3B8h]
    float4 t; // [esp+18Ch] [ebp-3B4h]
    float v48; // [esp+19Ch] [ebp-3A4h]
    float4 h; // [esp+1A0h] [ebp-3A0h]
    __int64 v50; // [esp+1B0h] [ebp-390h]
    float wind_dist; // [esp+1B8h] [ebp-388h]
    float4 windadd; // [esp+1BCh] [ebp-384h]
    unsigned int v53; // [esp+1CCh] [ebp-374h]
    float totalArray[6]; // [esp+1D0h] [ebp-370h]
    float v55; // [esp+1E8h] [ebp-358h]
    float diffLength; // [esp+1ECh] [ebp-354h]
    float v57; // [esp+1F0h] [ebp-350h]
    float4 diff; // [esp+1F4h] [ebp-34Ch]
    float4 avg; // [esp+204h] [ebp-33Ch]
    int v60; // [esp+214h] [ebp-32Ch]
    int v61; // [esp+218h] [ebp-328h]
    unsigned int v62[4]; // [esp+21Ch] [ebp-324h]
    __int64 v63; // [esp+22Ch] [ebp-314h]
    unsigned int v64; // [esp+234h] [ebp-30Ch]
    unsigned int v65; // [esp+238h] [ebp-308h]
    float *v66; // [esp+23Ch] [ebp-304h]
    float v67; // [esp+240h] [ebp-300h] BYREF
    float v68; // [esp+244h] [ebp-2FCh]
    float v69; // [esp+248h] [ebp-2F8h]
    float4 normal; // [esp+24Ch] [ebp-2F4h]
    int v71; // [esp+25Ch] [ebp-2E4h]
    float v72[11]; // [esp+260h] [ebp-2E0h]
    float4 d1; // [esp+28Ch] [ebp-2B4h]
    float4 d0; // [esp+29Ch] [ebp-2A4h]
    float4 total; // [esp+2ACh] [ebp-294h]
    const float4 *v76; // [esp+2BCh] [ebp-284h]
    const __int16 *v77; // [esp+2C0h] [ebp-280h]
    unsigned int v78; // [esp+2C4h] [ebp-27Ch]
    unsigned __int64 v79; // [esp+2C8h] [ebp-278h]
    float4 heights; // [esp+2D0h] [ebp-270h]
    float heightdata[4]; // [esp+2E0h] [ebp-260h]
    float4 veryoldc; // [esp+2F0h] [ebp-250h]
    float v83; // [esp+300h] [ebp-240h]
    float4 oldc; // [esp+304h] [ebp-23Ch]
    float v85; // [esp+314h] [ebp-22Ch]
    float4 old3; // [esp+318h] [ebp-228h]
    float v87; // [esp+328h] [ebp-218h]
    float4 old2; // [esp+32Ch] [ebp-214h]
    float v89; // [esp+33Ch] [ebp-204h]
    float4 old1; // [esp+340h] [ebp-200h]
    float v91; // [esp+350h] [ebp-1F0h]
    float4 old0; // [esp+354h] [ebp-1ECh]
    int v93; // [esp+364h] [ebp-1DCh]
    int realV; // [esp+368h] [ebp-1D8h]
    int realU; // [esp+36Ch] [ebp-1D4h]
    int uvAdjust; // [esp+370h] [ebp-1D0h]
    int u; // [esp+374h] [ebp-1CCh]
    const char *p_shoredist; // [esp+378h] [ebp-1C8h]
    const char *p_flooroffset; // [esp+37Ch] [ebp-1C4h]
    const __int16 *p_waterheight; // [esp+380h] [ebp-1C0h]
    const float4 *p_old_pos2; // [esp+384h] [ebp-1BCh]
    const float4 *p_old_pos1; // [esp+388h] [ebp-1B8h]
    const float4 *p_old_pos0; // [esp+38Ch] [ebp-1B4h]
    float4 *p_pos; // [esp+390h] [ebp-1B0h]
    int v; // [esp+394h] [ebp-1ACh]
    int endv; // [esp+398h] [ebp-1A8h]
    int startv; // [esp+39Ch] [ebp-1A4h]
    float4 windamount; // [esp+3A0h] [ebp-1A0h]
    float v109; // [esp+3B0h] [ebp-190h]
    _BYTE winddir[36]; // [esp+3B4h] [ebp-18Ch] OVERLAPPED
    __int64 v111; // [esp+3D8h] [ebp-168h]
    _BYTE v112[24]; // [esp+3E0h] [ebp-160h] OVERLAPPED
    __int64 v113; // [esp+3F8h] [ebp-148h]
    _BYTE v114[24]; // [esp+400h] [ebp-140h] OVERLAPPED
    __int64 v115; // [esp+418h] [ebp-128h]
    _BYTE v116[24]; // [esp+420h] [ebp-120h] OVERLAPPED
    __int64 v117; // [esp+438h] [ebp-108h]
    _BYTE v118[24]; // [esp+440h] [ebp-100h] OVERLAPPED
    __int64 v119; // [esp+458h] [ebp-E8h]
    _BYTE v120[24]; // [esp+460h] [ebp-E0h] OVERLAPPED
    float v121[3]; // [esp+478h] [ebp-C8h] BYREF
    float4 normalScale; // [esp+484h] [ebp-BCh]
    float4 foamamount; // [esp+494h] [ebp-ACh]
    float curlAmount; // [esp+4A4h] [ebp-9Ch]
    float4 breakamount; // [esp+4A8h] [ebp-98h]
    float minShoreHeight; // [esp+4B8h] [ebp-88h]
    float4 heightoffset; // [esp+4BCh] [ebp-84h]
    float v128; // [esp+4CCh] [ebp-74h]
    float4 damping; // [esp+4D0h] [ebp-70h]
    float v130; // [esp+4E0h] [ebp-60h]
    float4 inv_4; // [esp+4E4h] [ebp-5Ch]
    float4 one; // [esp+4F4h] [ebp-4Ch]
    float4 half; // [esp+504h] [ebp-3Ch]
    float4 zero; // [esp+514h] [ebp-2Ch] BYREF
    float windDirArray[3]; // [esp+524h] [ebp-1Ch]
    channel_t<float4> *old_pos; // [esp+534h] [ebp-Ch]
    channel_t<float4> *pos; // [esp+538h] [ebp-8h]
    channel_t<float4> *retaddr; // [esp+540h] [ebp+0h]

    old_pos = a1;
    pos = retaddr;
    LODWORD(windDirArray[2]) = &data.buffer[cmd->update.destBuf];
    LODWORD(windDirArray[1]) = &data.buffer[cmd->update.srcBuf];
    *(_QWORD *)&zero.unitVec[2].packed = __PAIR64__(LODWORD(config.windDir[2]), LODWORD(FLOAT_N0_1));
    half.u[2] = 0;
    half.u[3] = 0;
    *(_QWORD *)zero.v = __PAIR64__(LODWORD(config.windDir[0]), 0);
    one.v[2] = 0.5f;
    one.v[3] = 0.5f;
    *(_QWORD *)half.v = __PAIR64__(0, LODWORD(0.5f));
    inv_4.v[2] = 1.0f;
    inv_4.v[3] = 1.0f;
    *(_QWORD *)one.v = __PAIR64__(LODWORD(0.5f), LODWORD(1.0f));
    damping.v[2] = 0.25f;
    damping.v[3] = 0.25f;
    v130 = 0.25f;
    *(_QWORD *)inv_4.v = __PAIR64__(LODWORD(1.0f), LODWORD(0.25f));
    damping.v[1] = 1.0 - (float)(1.0 / config.waveDamping);
    heightoffset.v[2] = damping.v[1];
    heightoffset.v[3] = damping.v[1];
    v128 = damping.v[1];
    damping.v[0] = damping.v[1];
    heightoffset.u[1] = LODWORD(config.minShoreHeight);
    breakamount.u[3] = LODWORD(config.minShoreHeight);
    minShoreHeight = config.minShoreHeight;
    heightoffset.u[0] = LODWORD(config.minShoreHeight);
    *(_QWORD *)&breakamount.unitVec[1].packed = __PAIR64__(LODWORD(config.minShoreHeight), LODWORD(config.curlAmount));
    foamamount.u[3] = LODWORD(config.curlAmount);
    curlAmount = config.curlAmount;
    breakamount.u[0] = LODWORD(config.curlAmount);
    *(_QWORD *)&foamamount.unitVec[1].packed = __PAIR64__(LODWORD(config.curlAmount), LODWORD(config.foamAppear));
    normalScale.u[1] = LODWORD(config.foamAppear);
    normalScale.u[2] = LODWORD(config.foamAppear);
    normalScale.u[3] = LODWORD(config.foamAppear);
    foamamount.u[0] = LODWORD(config.foamAppear);
    Float4LoadFloats(
        COERCE_UNSIGNED_INT(-0.5),
        COERCE_UNSIGNED_INT(1.0),
        COERCE_UNSIGNED_INT(-0.5),
        COERCE_UNSIGNED_INT(0.0),
        (float4 *)v121);
    if ( r_watersim_flatten->current.enabled )
    {
        *(_QWORD *)&heightoffset.unitVec[2].packed = *(_QWORD *)&half.unitVec[1].packed;
        v128 = half.v[3];
        damping.u[0] = zero.u[0];
    }
    *(float4 *)&v120[8] = g_swizzleYBWD;
    v119 = *(_QWORD *)g_swizzleYBWD.v;
    *(_QWORD *)v120 = *(_QWORD *)&g_swizzleYBWD.unitVec[2].packed;
    *(float4 *)&v118[8] = g_swizzleXAYA;
    v117 = *(_QWORD *)g_swizzleXAYA.v;
    *(_QWORD *)v118 = *(_QWORD *)&g_swizzleXAYA.unitVec[2].packed;
    *(float4 *)&v116[8] = g_swizzleYABA;
    v115 = *(_QWORD *)g_swizzleYABA.v;
    *(_QWORD *)v116 = *(_QWORD *)&g_swizzleYABA.unitVec[2].packed;
    *(float4 *)&v114[8] = g_swizzleXAZC;
    v113 = *(_QWORD *)g_swizzleXAZC.v;
    *(_QWORD *)v114 = *(_QWORD *)&g_swizzleXAZC.unitVec[2].packed;
    *(float4 *)&v112[8] = g_keepY;
    v111 = *(_QWORD *)g_keepY.v;
    *(_QWORD *)v112 = *(_QWORD *)&g_keepY.unitVec[2].packed;
    *(float4 *)&winddir[20] = g_keepXZ;
    *(float4 *)&winddir[4] = g_keepXZ;
    Vec3Normalize(&zero.v[1]);
    zero.u[1] ^= _mask__NegFloat_;
    zero.u[2] ^= _mask__NegFloat_;
    zero.u[3] ^= _mask__NegFloat_;
    *(_QWORD *)&windamount.unitVec[2].packed = *(_QWORD *)&zero.unitVec[1].packed;
    v109 = zero.v[3];
    *(float *)winddir = windDirArray[0];
    windamount.u[1] = LODWORD(config.windAmount);
    v = SLODWORD(config.windAmount);
    endv = SLODWORD(config.windAmount);
    startv = SLODWORD(config.windAmount);
    windamount.u[0] = LODWORD(config.windAmount);
    p_pos = (float4 *)cmd->update.startTile;
    p_old_pos0 = (const float4 *)(cmd->update.startTile + 16);
    for ( p_old_pos1 = p_pos; (int)p_old_pos1 < (int)p_old_pos0; p_old_pos1 = (const float4 *)((char *)p_old_pos1 + 1) )
    {
        p_old_pos2 = channel_t<float4>::Cache(
                                     (channel_t<float4> *)LODWORD(windDirArray[2]),
                                     (unsigned __int8)p_old_pos1,
                                     1,
                                     0);
        p_waterheight = (const __int16 *)channel_t<float4>::Cache(
                                                                             (channel_t<float4> *)LODWORD(windDirArray[1]),
                                                                             (unsigned __int8)((_BYTE)p_old_pos1 - 1),
                                                                             0,
                                                                             0);
        p_flooroffset = (const char *)channel_t<float4>::Cache(
                                                                        (channel_t<float4> *)LODWORD(windDirArray[1]),
                                                                        (unsigned __int8)p_old_pos1,
                                                                        0,
                                                                        0);
        p_shoredist = (const char *)channel_t<float4>::Cache(
                                                                    (channel_t<float4> *)LODWORD(windDirArray[1]),
                                                                    (unsigned __int8)((_BYTE)p_old_pos1 + 1),
                                                                    0,
                                                                    0);
        u = (int)channel_t<short>::Cache(&data.waterheight, (unsigned __int8)p_old_pos1, 0, 0);
        uvAdjust = (int)channel_t<char>::Cache(&data.flooroffset, (unsigned __int8)p_old_pos1, 0, 0);
        v2 = channel_t<char>::Cache(&data.shoredist, (unsigned __int8)p_old_pos1, 0, 0);
        p_old_pos2 -= 256;
        p_waterheight -= 2048;
        p_flooroffset -= 4096;
        p_shoredist -= 4096;
        u -= 512;
        uvAdjust -= 256;
        realU = (int)(v2 - 256);
        realV = 0;
        while ( realV < 256 )
        {
            v93 = 128;
            old0.u[3] = (unsigned __int8)(realV - LOBYTE(data.centerU) + 0x80) - 128;
            old0.u[2] = (unsigned __int8)((_BYTE)p_old_pos1 - LOBYTE(data.centerV) + 0x80) - 128;
            old0.u[1] = (unsigned int)(p_flooroffset + 4080);
            *(_QWORD *)&old1.unitVec[2].packed = *((_QWORD *)p_flooroffset + 510);
            v91 = *((float *)p_flooroffset + 1022);
            old0.u[0] = *((unsigned int *)p_flooroffset + 1023);
            old1.u[1] = (unsigned int)(p_flooroffset + 4112);
            *(_QWORD *)&old2.unitVec[2].packed = *((_QWORD *)p_flooroffset + 514);
            v89 = *((float *)p_flooroffset + 1030);
            old1.u[0] = *((unsigned int *)p_flooroffset + 1031);
            old2.u[1] = (unsigned int)(p_waterheight + 2048);
            *(_QWORD *)&old3.unitVec[2].packed = *((_QWORD *)p_waterheight + 512);
            v87 = *((float *)p_waterheight + 1026);
            old2.u[0] = *((unsigned int *)p_waterheight + 1027);
            old3.u[1] = (unsigned int)(p_shoredist + 4096);
            *(_QWORD *)&oldc.unitVec[2].packed = *((_QWORD *)p_shoredist + 512);
            v85 = *((float *)p_shoredist + 1026);
            old3.u[0] = *((unsigned int *)p_shoredist + 1027);
            oldc.u[1] = (unsigned int)(p_flooroffset + 4096);
            *(_QWORD *)&veryoldc.unitVec[2].packed = *((_QWORD *)p_flooroffset + 512);
            v83 = *((float *)p_flooroffset + 1026);
            oldc.u[0] = *((unsigned int *)p_flooroffset + 1027);
            veryoldc.u[1] = (unsigned int)&p_old_pos2[256];
            heightdata[1] = p_old_pos2[256].v[0];
            heightdata[2] = p_old_pos2[256].v[1];
            heightdata[3] = p_old_pos2[256].v[2];
            veryoldc.u[0] = p_old_pos2[256].u[3];
            heights.v[1] = (float)*(__int16 *)(u + 512);
            heights.u[2] = 0;
            heights.v[3] = (float)*(char *)(uvAdjust + 256);
            heightdata[0] = (float)*(char *)(realU + 256);
            v78 = heights.u[1];
            v79 = __PAIR64__(heights.u[3], 0);
            heights.v[0] = heightdata[0];
            v77 = p_waterheight + 4352;
            _mm_prefetch((const char *)p_waterheight + 8704, 1);
            v76 = p_old_pos2 + 288;
            _mm_prefetch((const char *)&p_old_pos2[288], 1);
            total.u[3] = u + 1024;
            _mm_prefetch((const char *)(u + 1024), 1);
            total.u[2] = uvAdjust + 768;
            _mm_prefetch((const char *)(uvAdjust + 768), 1);
            total.u[1] = realU + 768;
            _mm_prefetch((const char *)(realU + 768), 1);
            d0.v[1] = (float)(old1.v[2] + old2.v[2]) + old3.v[2];
            d0.v[2] = (float)(old1.v[3] + old2.v[3]) + old3.v[3];
            d0.v[3] = (float)(v91 + v89) + v87;
            total.v[0] = (float)(old0.v[0] + old1.v[0]) + old2.v[0];
            d0.v[1] = d0.v[1] + oldc.v[2];
            d0.v[2] = d0.v[2] + oldc.v[3];
            d0.v[3] = d0.v[3] + v85;
            total.v[0] = total.v[0] + old3.v[0];
            d0.v[1] = d0.v[1] * one.v[1];
            d0.v[2] = d0.v[2] * one.v[2];
            d0.v[3] = d0.v[3] * one.v[3];
            total.v[0] = total.v[0] * half.v[0];
            d0.v[1] = d0.v[1] - heightdata[1];
            d0.v[2] = d0.v[2] - heightdata[2];
            d0.v[3] = d0.v[3] - heightdata[3];
            total.v[0] = total.v[0] - veryoldc.v[0];
            d0.v[1] = d0.v[1] * heightoffset.v[2];
            d0.v[2] = d0.v[2] * heightoffset.v[3];
            d0.v[3] = d0.v[3] * v128;
            total.v[0] = total.v[0] * damping.v[0];
            d1.v[1] = old2.v[2] - old1.v[2];
            d1.v[2] = old2.v[3] - old1.v[3];
            d1.v[3] = v89 - v91;
            d0.v[0] = old1.v[0] - old0.v[0];
            v72[8] = oldc.v[2] - old3.v[2];
            v72[9] = oldc.v[3] - old3.v[3];
            v72[10] = v85 - v87;
            d1.v[0] = old3.v[0] - old2.v[0];
            v72[0] = old2.v[2] - old1.v[2];
            v72[1] = old2.v[3] - old1.v[3];
            v72[2] = v89 - v91;
            v72[3] = old1.v[0] - old0.v[0];
            v72[4] = oldc.v[2] - old3.v[2];
            v72[5] = oldc.v[3] - old3.v[3];
            v72[6] = v85 - v87;
            v72[7] = old3.v[0] - old2.v[0];
            *(_QWORD *)&normal.unitVec[1].packed = v115;
            normal.u[3] = *(unsigned int *)v116;
            v71 = *(unsigned int *)&v116[4];
            v66 = &v67;
            LOBYTE(v67) = *((_BYTE *)v72 + BYTE3(v115));
            BYTE1(v67) = *((_BYTE *)v72 + BYTE2(v115));
            BYTE2(v67) = *((_BYTE *)v72 + BYTE1(v115));
            HIBYTE(v67) = *((_BYTE *)v72 + (unsigned __int8)v115);
            LOBYTE(v68) = *((_BYTE *)v72 + HIBYTE(v115));
            BYTE1(v68) = *((_BYTE *)v72 + BYTE6(v115));
            BYTE2(v68) = *((_BYTE *)v72 + BYTE5(v115));
            HIBYTE(v68) = *((_BYTE *)v72 + BYTE4(v115));
            LOBYTE(v69) = *((_BYTE *)v72 + v116[3]);
            BYTE1(v69) = *((_BYTE *)v72 + v116[2]);
            BYTE2(v69) = *((_BYTE *)v72 + v116[1]);
            HIBYTE(v69) = *((_BYTE *)v72 + v116[0]);
            normal.unitVec[0].array[0] = *((_BYTE *)v72 + v116[7]);
            normal.unitVec[0].array[1] = *((_BYTE *)v72 + v116[6]);
            normal.unitVec[0].array[2] = *((_BYTE *)v72 + v116[5]);
            normal.unitVec[0].array[3] = *((_BYTE *)v72 + v116[4]);
            *(float *)v62 = v67;
            *(float *)&v62[1] = v68;
            *(float *)&v62[2] = v69;
            v62[3] = normal.u[0];
            v63 = *(_QWORD *)&inv_4.unitVec[1].packed;
            v64 = inv_4.u[3];
            v65 = one.u[0];
            *(_QWORD *)&avg.unitVec[2].packed = v113;
            v60 = *(unsigned int *)v114;
            v61 = *(unsigned int *)&v114[4];
            avg.u[1] = (unsigned int)&v67;
            LOBYTE(v67) = *((_BYTE *)v62 + BYTE3(v113));
            BYTE1(v67) = *((_BYTE *)v62 + BYTE2(v113));
            BYTE2(v67) = *((_BYTE *)v62 + BYTE1(v113));
            HIBYTE(v67) = *((_BYTE *)v62 + (unsigned __int8)v113);
            LOBYTE(v68) = *((_BYTE *)v62 + HIBYTE(v113));
            BYTE1(v68) = *((_BYTE *)v62 + BYTE6(v113));
            BYTE2(v68) = *((_BYTE *)v62 + BYTE5(v113));
            HIBYTE(v68) = *((_BYTE *)v62 + BYTE4(v113));
            LOBYTE(v69) = *((_BYTE *)v62 + v114[3]);
            BYTE1(v69) = *((_BYTE *)v62 + v114[2]);
            BYTE2(v69) = *((_BYTE *)v62 + v114[1]);
            HIBYTE(v69) = *((_BYTE *)v62 + v114[0]);
            normal.unitVec[0].array[0] = *((_BYTE *)v62 + v114[7]);
            normal.unitVec[0].array[1] = *((_BYTE *)v62 + v114[6]);
            normal.unitVec[0].array[2] = *((_BYTE *)v62 + v114[5]);
            normal.unitVec[0].array[3] = *((_BYTE *)v62 + v114[4]);
            v67 = v67 * v121[0];
            v68 = v68 * v121[1];
            v69 = v69 * v121[2];
            normal.v[0] = normal.v[0] * normalScale.v[0];
            diff.v[1] = (float)(old1.v[2] + old2.v[2]) + old3.v[2];
            diff.v[2] = (float)(old1.v[3] + old2.v[3]) + old3.v[3];
            diff.v[3] = (float)(v91 + v89) + v87;
            avg.v[0] = (float)(old0.v[0] + old1.v[0]) + old2.v[0];
            diff.v[1] = diff.v[1] + oldc.v[2];
            diff.v[2] = diff.v[2] + oldc.v[3];
            diff.v[3] = diff.v[3] + v85;
            avg.v[0] = avg.v[0] + old3.v[0];
            diff.v[1] = diff.v[1] * damping.v[2];
            diff.v[2] = diff.v[2] * damping.v[3];
            diff.v[3] = diff.v[3] * v130;
            avg.v[0] = avg.v[0] * inv_4.v[0];
            v55 = d0.v[1] - diff.v[1];
            diffLength = d0.v[2] - diff.v[2];
            diff.v[0] = (float)((float)(v55 * v55) + (float)(diffLength * diffLength))
                                + (float)((float)(d0.v[3] - diff.v[3]) * (float)(d0.v[3] - diff.v[3]));
            v57 = diff.v[0];
            diffLength = diff.v[0];
            totalArray[5] = diff.v[0];
            v55 = diff.v[0];
            totalArray[4] = diff.v[0];
            if ( diff.v[0] > config.curlMax )
            {
                totalArray[3] = config.curlReduce;
                d0.v[1] = config.curlReduce * d0.v[1];
                d0.v[2] = config.curlReduce * d0.v[2];
                d0.v[3] = config.curlReduce * d0.v[3];
                total.v[0] = config.curlReduce * total.v[0];
            }
            *(_QWORD *)&windadd.unitVec[2].packed = *(_QWORD *)&d0.unitVec[1].packed;
            v53 = d0.u[3];
            totalArray[0] = total.v[0];
            windadd.v[1] = (float)((float)(v67 * windamount.v[2]) + (float)(v68 * windamount.v[3])) + (float)(v69 * v109);
            *(float *)&v50 = *(float *)&v * windadd.v[1];
            *((float *)&v50 + 1) = *(float *)&endv * windadd.v[1];
            wind_dist = *(float *)&startv * windadd.v[1];
            windadd.v[0] = windamount.v[0] * windadd.v[1];
            h.v[3] = (float)((float)(int)old0.u[3] * zero.v[1]) + (float)((float)(int)old0.u[2] * zero.v[2]);
            h.u[2] = d0.u[2];
            h.u[1] = d0.u[2] & _mask__AbsFloat_;
            if ( config.windMax > fabs(d0.u[2]) && h.v[3] > 0.0 )
            {
                v50 &= v111;
                LODWORD(wind_dist) &= *(unsigned int *)v112;
                windadd.u[0] &= *(unsigned int *)&v112[4];
                d0.v[1] = d0.v[1] - *(float *)&v50;
                d0.v[2] = d0.v[2] - *((float *)&v50 + 1);
                d0.v[3] = d0.v[3] - wind_dist;
                total.v[0] = total.v[0] - windadd.v[0];
            }
            t.v[2] = heights.v[0] + breakamount.v[2];
            t.v[3] = heights.v[0] + breakamount.v[3];
            v48 = heights.v[0] + minShoreHeight;
            h.v[0] = heights.v[0] + heightoffset.v[0];
            if ( half.v[1] <= (float)(heights.v[0] + breakamount.v[2]) )
                t.u[1] = 0;
            else
                t.u[1] = -1;
            v44 = t.v[1];
            if ( half.v[2] <= t.v[3] )
                clampedY.u[3] = 0;
            else
                clampedY.u[3] = -1;
            v45 = clampedY.v[3];
            if ( half.v[3] <= v48 )
                clampedY.u[2] = 0;
            else
                clampedY.u[2] = -1;
            v46 = clampedY.v[2];
            if ( zero.v[0] <= h.v[0] )
                clampedY.u[1] = 0;
            else
                clampedY.u[1] = -1;
            v44 = v45;
            v46 = v45;
            t.v[0] = v45;
            d0.u[1] &= LODWORD(v45);
            d0.u[2] &= LODWORD(v45);
            d0.u[3] &= LODWORD(v45);
            total.u[0] &= LODWORD(v45);
            v40 = d0.u[2];
            v41 = d0.v[2];
            v42 = d0.v[2];
            clampedY.u[0] = d0.u[2];
            if ( (float)(d0.v[2] - inv_4.v[1]) < 0.0 )
                v3 = inv_4.u[1];
            else
                v3 = v40;
            v39 = v3;
            v40 = v3;
            v38 = v41;
            v37 = inv_4.u[2];
            if ( (float)(v41 - inv_4.v[2]) < 0.0 )
                v4 = *(float *)&v37;
            else
                v4 = v38;
            v36 = v4;
            v41 = v4;
            v35 = v42;
            v34 = inv_4.u[3];
            if ( (float)(v42 - inv_4.v[3]) < 0.0 )
                v5 = *(float *)&v34;
            else
                v5 = v35;
            v33 = v5;
            v42 = v5;
            *(_QWORD *)&tilt.unitVec[2].packed = __PAIR64__(clampedY.u[0], one.u[0]);
            if ( (float)(clampedY.v[0] - one.v[0]) < 0.0 )
                v6 = tilt.v[2];
            else
                v6 = tilt.v[3];
            tilt.v[1] = v6;
            clampedY.v[0] = v6;
            v29 = (float)(veryoldc.v[2] - heightdata[1]) * foamamount.v[2];
            v30 = (float)(veryoldc.v[3] - heightdata[2]) * foamamount.v[3];
            v31 = (float)(v83 - heightdata[3]) * curlAmount;
            tilt.v[0] = v6;
            if ( v6 == 0.0
                && !Assert_MyHandler(
                            "c:\\projects_pc\\cod\\codsrc\\src\\universal\\com_vector4_novec.h",
                            645,
                            0,
                            "%s",
                            "in.v[3]") )
            {
                __debugbreak();
            }
            v28 = 1.0 / tilt.v[0];
            v30 = v30 * (float)(1.0 / tilt.v[0]);
            v29 = v30;
            v31 = v30;
            tilt.v[0] = v30;
            if ( d0.v[1] <= half.v[1] )
                v27 = 0.0;
            else
                v27 = NAN;
            v44 = v27;
            if ( d0.v[2] <= half.v[2] )
                tiltx.u[3] = 0;
            else
                tiltx.u[3] = -1;
            v45 = tiltx.v[3];
            if ( d0.v[3] <= half.v[3] )
                tiltx.u[2] = 0;
            else
                tiltx.u[2] = -1;
            v46 = tiltx.v[2];
            if ( total.v[0] <= zero.v[0] )
                tiltx.u[1] = 0;
            else
                tiltx.u[1] = -1;
            v44 = v45;
            v46 = v45;
            t.v[0] = v45;
            LODWORD(v29) &= LODWORD(v45);
            LODWORD(v30) &= LODWORD(v45);
            LODWORD(v31) &= LODWORD(v45);
            tilt.u[0] &= LODWORD(v45);
            tiltz.v[1] = old1.v[2] - old2.v[2];
            tiltz.v[2] = old1.v[3] - old2.v[3];
            tiltz.v[3] = v91 - v89;
            tiltx.v[0] = old0.v[0] - old1.v[0];
            v24[8] = old3.v[2] - oldc.v[2];
            v24[9] = old3.v[3] - oldc.v[3];
            v24[10] = v87 - v85;
            tiltz.v[0] = old2.v[0] - old3.v[0];
            v24[0] = old1.v[2] - old2.v[2];
            v24[1] = old1.v[3] - old2.v[3];
            v24[2] = v91 - v89;
            v24[3] = old0.v[0] - old1.v[0];
            v24[4] = old3.v[2] - oldc.v[2];
            v24[5] = old3.v[3] - oldc.v[3];
            v24[6] = v87 - v85;
            v24[7] = old2.v[0] - old3.v[0];
            v21 = v119;
            v22 = *(unsigned int *)v120;
            v23 = *(unsigned int *)&v120[4];
            v20 = &v44;
            LOBYTE(v44) = *((_BYTE *)v24 + BYTE3(v119));
            BYTE1(v44) = *((_BYTE *)v24 + BYTE2(v119));
            BYTE2(v44) = *((_BYTE *)v24 + BYTE1(v119));
            HIBYTE(v44) = *((_BYTE *)v24 + (unsigned __int8)v119);
            LOBYTE(v45) = *((_BYTE *)v24 + HIBYTE(v119));
            BYTE1(v45) = *((_BYTE *)v24 + BYTE6(v119));
            BYTE2(v45) = *((_BYTE *)v24 + BYTE5(v119));
            HIBYTE(v45) = *((_BYTE *)v24 + BYTE4(v119));
            LOBYTE(v46) = *((_BYTE *)v24 + v120[3]);
            BYTE1(v46) = *((_BYTE *)v24 + v120[2]);
            BYTE2(v46) = *((_BYTE *)v24 + v120[1]);
            HIBYTE(v46) = *((_BYTE *)v24 + v120[0]);
            t.unitVec[0].array[0] = *((_BYTE *)v24 + v120[7]);
            t.unitVec[0].array[1] = *((_BYTE *)v24 + v120[6]);
            t.unitVec[0].array[2] = *((_BYTE *)v24 + v120[5]);
            t.unitVec[0].array[3] = *((_BYTE *)v24 + v120[4]);
            *(float *)v16 = v44;
            *(float *)&v16[1] = v45;
            *(float *)&v16[2] = v46;
            v16[3] = t.u[0];
            v17 = *(_QWORD *)&half.unitVec[1].packed;
            v18 = half.u[3];
            v19 = zero.u[0];
            LOBYTE(v44) = *((_BYTE *)v16 + BYTE3(v117));
            BYTE1(v44) = *((_BYTE *)v16 + BYTE2(v117));
            BYTE2(v44) = *((_BYTE *)v16 + BYTE1(v117));
            HIBYTE(v44) = *((_BYTE *)v16 + (unsigned __int8)v117);
            LOBYTE(v45) = *((_BYTE *)v16 + HIBYTE(v117));
            BYTE1(v45) = *((_BYTE *)v16 + BYTE6(v117));
            BYTE2(v45) = *((_BYTE *)v16 + BYTE5(v117));
            HIBYTE(v45) = *((_BYTE *)v16 + BYTE4(v117));
            LOBYTE(v46) = *((_BYTE *)v16 + v118[3]);
            BYTE1(v46) = *((_BYTE *)v16 + v118[2]);
            BYTE2(v46) = *((_BYTE *)v16 + v118[1]);
            HIBYTE(v46) = *((_BYTE *)v16 + v118[0]);
            t.unitVec[0].array[0] = *((_BYTE *)v16 + v118[7]);
            t.unitVec[0].array[1] = *((_BYTE *)v16 + v118[6]);
            t.unitVec[0].array[2] = *((_BYTE *)v16 + v118[5]);
            t.unitVec[0].array[3] = *((_BYTE *)v16 + v118[4]);
            d0.v[1] = (float)(v44 * v29) + d0.v[1];
            d0.v[2] = (float)(v45 * v30) + d0.v[2];
            d0.v[3] = (float)(v46 * v31) + d0.v[3];
            total.v[0] = (float)(t.v[0] * tilt.v[0]) + total.v[0];
            LODWORD(foamadd_12) = COERCE_UNSIGNED_INT(d0.v[1] - veryoldc.v[2]) & ~(unsigned int)v111;
            LODWORD(v11) = COERCE_UNSIGNED_INT(d0.v[2] - veryoldc.v[3]) & ~HIDWORD(v111);
            LODWORD(v13) = COERCE_UNSIGNED_INT(d0.v[3] - v83) & ~*(unsigned int *)v112;
            d = clampedY.v[0];
            if ( clampedY.v[0] == 0.0
                && !Assert_MyHandler(
                            "c:\\projects_pc\\cod\\codsrc\\src\\universal\\com_vector4_novec.h",
                            645,
                            0,
                            "%s",
                            "in.v[3]") )
            {
                __debugbreak();
            }
            LODWORD(foamadd_12a) = *(unsigned int *)&winddir[4] & COERCE_UNSIGNED_INT(foamadd_12 * (float)(1.0 / d));
            LODWORD(v12) = *(unsigned int *)&winddir[8] & COERCE_UNSIGNED_INT(v11 * (float)(1.0 / d));
            LODWORD(v14) = *(unsigned int *)&winddir[12] & COERCE_UNSIGNED_INT(v13 * (float)(1.0 / d));
            if ( (float)(one.v[0]
                                 - (float)((float)((float)((float)((float)((float)((float)(foamadd_12a * foamadd_12a)
                                                                                                                                 + (float)(v12 * v12))
                                                                                                                 + (float)(v14 * v14))
                                                                                                 + (float)(COERCE_FLOAT(*(unsigned int *)&winddir[16] & LODWORD(1.0f))
                                                                                                                 * COERCE_FLOAT(*(unsigned int *)&winddir[16] & LODWORD(1.0f))))
                                                                                 * foamamount.v[0])
                                                                 + oldc.v[0])
                                                 * (float)(1.0 - config.foamDisappear))) < 0.0 )
                v7 = one.v[0];
            else
                v7 = (float)((float)((float)((float)((float)((float)(foamadd_12a * foamadd_12a) + (float)(v12 * v12))
                                                                                     + (float)(v14 * v14))
                                                                     + (float)(COERCE_FLOAT(*(unsigned int *)&winddir[16] & LODWORD(1.0f))
                                                                                     * COERCE_FLOAT(*(unsigned int *)&winddir[16] & LODWORD(1.0f))))
                                                     * foamamount.v[0])
                                     + oldc.v[0])
                     * (float)(1.0 - config.foamDisappear);
            total.v[0] = v7;
            v8 = p_old_pos2 + 256;
            *(_QWORD *)p_old_pos2[256].v = *(_QWORD *)&d0.unitVec[1].packed;
            v8->u[2] = d0.u[3];
            v8->u[3] = total.u[0];
            ++realV;
            ++p_old_pos2;
            p_waterheight += 8;
            p_flooroffset += 16;
            p_shoredist += 16;
            u += 2;
            ++uvAdjust;
            ++realU;
        }
    }
}
#endif

void R_WaterSimulationUpdateFrame(const WaterSimulationCmd *cmd)
{
    if (!waterSimInitialized || !r_watersim_enabled->current.enabled)
        return;

    channel_t<float4> &source = data.buffer[cmd->update.srcBuf];
    channel_t<float4> &destination = data.buffer[cmd->update.destBuf];
    const float damping = r_watersim_flatten->current.enabled
                        ? 0.0f : 1.0f - 1.0f / config.waveDamping;
    // Simulation coordinates have vertical Y. Keep retail's tilted wind vector
    // and its arithmetic order rather than normalizing a world-space XYZ vector.
    float wind[3] = {config.windDir[0], -0.1f, config.windDir[2]};
    const float windLength = sqrtf((wind[0] * wind[0] + wind[1] * wind[1]) + wind[2] * wind[2]);
    const float windInverse = 1.0f / (windLength > 0.0f ? windLength : 1.0f);
    for (unsigned lane = 0; lane < 3; ++lane) wind[lane] *= -windInverse;

    for (unsigned row = cmd->update.startTile; row < cmd->update.startTile + 16; ++row)
    {
        const unsigned y = static_cast<unsigned char>(row);
        float4 *output = destination.Cache(y, true, false);
        const float4 *up = source.Cache(static_cast<unsigned char>(row - 1), false, false);
        const float4 *center = source.Cache(y, false, false);
        const float4 *down = source.Cache(static_cast<unsigned char>(row + 1), false, false);
        // Retail also caches base height and floor offset; neither contributes
        // to this update's exported values. Shore distance controls the mask.
        data.waterheight.Cache(y, false, false);
        data.flooroffset.Cache(y, false, false);
        const char *shore = data.shoredist.Cache(y, false, false);
        const int relativeV = static_cast<int>((row - data.centerV + 128u) & 255u) - 128;

        for (unsigned x = 0; x < 256; ++x)
        {
            // Wrap explicit indices instead of indexing outside the row array.
            const float4 &left = center[(x + 255) & 255];
            const float4 &right = center[(x + 1) & 255];
            const float4 history = output[x];
            float4 next{};
            float average[3];
            for (unsigned lane = 0; lane < 3; ++lane)
            {
                const float neighbors = ((right.v[lane] + left.v[lane]) + up[x].v[lane]) + down[x].v[lane];
                next.v[lane] = (neighbors * 0.5f - history.v[lane]) * damping;
                average[lane] = neighbors * 0.25f;
            }
            const float diffX = next.v[0] - average[0];
            const float diffY = next.v[1] - average[1];
            const float diffZ = next.v[2] - average[2];
            const float curlLengthSq = (diffX * diffX + diffZ * diffZ) + diffY * diffY;
            if (curlLengthSq > config.curlMax)
                for (unsigned lane = 0; lane < 3; ++lane) next.v[lane] *= config.curlReduce;

            const float normalX = (right.v[1] - left.v[1]) * -0.5f;
            const float normalZ = (down[x].v[1] - up[x].v[1]) * -0.5f;
            const float windDot = (normalX * wind[0] + normalZ * wind[2]) + wind[1];
            const int relativeU = static_cast<int>((x - data.centerU + 128u) & 255u) - 128;
            const float windSide = static_cast<float>(relativeU) * wind[0] + static_cast<float>(relativeV) * wind[1];
            if (config.windMax > fabsf(next.v[1]) && windSide > 0.0f)
                next.v[1] -= windDot * config.windAmount;

            if (!(static_cast<float>(static_cast<signed char>(shore[x])) + config.minShoreHeight < 0.0f))
                for (unsigned lane = 0; lane < 3; ++lane) next.v[lane] = 0.0f;
            const float inverseHeight = 1.0f / (next.v[1] >= 1.0f ? next.v[1] : 1.0f);
            float tilt = ((center[x].v[1] - history.v[1]) * config.curlAmount) * inverseHeight;
            if (!(next.v[1] > 0.0f)) tilt = 0.0f;
            next.v[0] += (left.v[1] - right.v[1]) * tilt;
            next.v[1] += 0.0f * tilt; // Retail adds the zero vertical tilt lane, including signed zero.
            next.v[2] += (up[x].v[1] - down[x].v[1]) * tilt;

            const float foamX = (next.v[0] - center[x].v[0]) * inverseHeight;
            const float foamZ = (next.v[2] - center[x].v[2]) * inverseHeight;
            const float foam = ((foamX * foamX + foamZ * foamZ) * config.foamAppear + center[x].v[3])
                             * (1.0f - config.foamDisappear);
            next.v[3] = foam > 1.0f ? 1.0f : foam;
            output[x] = next;
        }
    }
}

void __cdecl Float4LoadFloats(unsigned int x, unsigned int y, unsigned int z, unsigned int w, float4 *result)
{
    *(_QWORD *)result->v = __PAIR64__(y, x);
    *(_QWORD *)&result->unitVec[2].packed = __PAIR64__(w, z);
}

void *__cdecl Water_AllocVertexBuffer(IDirect3DVertexBuffer9 **vb, unsigned int buffersize)
{
    return R_AllocDynamicVertexBuffer(vb, buffersize);
}

void *__cdecl Water_AllocIndexBuffer(IDirect3DIndexBuffer9 **ib, unsigned int buffersize)
{
    return R_AllocStaticIndexBuffer(ib, buffersize);
}

void __cdecl R_InitWaterSimulationVertexBuffers()
{
    unsigned __int16 verts[10]; // [esp+4h] [ebp-44h] BYREF
    unsigned __int16 base; // [esp+1Ch] [ebp-2Ch]
    int count; // [esp+20h] [ebp-28h]
    unsigned int j; // [esp+24h] [ebp-24h]
    unsigned int i; // [esp+28h] [ebp-20h]
    unsigned int x; // [esp+2Ch] [ebp-1Ch]
    unsigned int y; // [esp+30h] [ebp-18h]
    unsigned int buffersize; // [esp+34h] [ebp-14h]
    int indexbuffersize; // [esp+38h] [ebp-10h]
    unsigned __int16 vstride; // [esp+3Ch] [ebp-Ch]
    int numTris; // [esp+40h] [ebp-8h]
    unsigned __int16 *lockedindex; // [esp+44h] [ebp-4h]

    if ( !vertexBuffersAllocated )
    {
        buffersize = 3699200;
        data.lockedData = (GfxWaterVertex *)Water_AllocVertexBuffer(&data.vertexBuffer, 0x387200u);
        numTris = 512;
        data.tileTriCount[0] = 512;
        indexbuffersize = 3072;
        vstride = 17;
        lockedindex = (unsigned __int16 *)Water_AllocIndexBuffer(data.indexBuffer, 0xC00u);
        for ( y = 0; y < 0x10; ++y )
        {
            for ( x = 0; x < 0x10; ++x )
            {
                *lockedindex++ = x + y * vstride;
                *lockedindex++ = x + vstride * (y + 1);
                *lockedindex++ = y * vstride + x + 1;
                *lockedindex++ = y * vstride + x + 1;
                *lockedindex++ = x + vstride * (y + 1);
                *lockedindex++ = vstride * (y + 1) + x + 1;
            }
        }
        R_FinishStaticIndexBuffer(data.indexBuffer[0]);
        lockedindex = (unsigned __int16 *)Water_AllocIndexBuffer(&data.indexBuffer[1], indexbuffersize);
        numTris = 0;
        for ( i = 0; i < 0x10; i += 2 )
        {
            for ( j = 0; j < 0x10; j += 2 )
            {
                base = j + i * vstride;
                verts[0] = base;
                verts[1] = base + 1;
                verts[2] = base + 2;
                verts[3] = vstride + base;
                verts[4] = base + vstride + 1;
                verts[5] = base + vstride + 2;
                verts[6] = base + 2 * vstride;
                verts[7] = verts[6] + 1;
                verts[8] = verts[6] + 2;
                count = FillIndexBuffer(lockedindex, (i != 14) | (i != 0 ? 2 : 0) | (j != 14 ? 4 : 0) | (j != 0 ? 8 : 0), verts);
                lockedindex += count;
                numTris += count / 3;
                if ( 3 * (count / 3) != count
                    && !Assert_MyHandler(
                                "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_water_sim.cpp",
                                1801,
                                0,
                                "%s",
                                "(count/3)*3 == count") )
                {
                    __debugbreak();
                }
            }
        }
        data.tileTriCount[1] = numTris;
        R_FinishStaticIndexBuffer(data.indexBuffer[1]);
        vertexBuffersAllocated = 1;
    }
}

unsigned int __cdecl FillIndexBuffer(unsigned __int16 *lockedindex, unsigned int lodMask, unsigned __int16 *verts)
{
    unsigned int count; // [esp+0h] [ebp-8h]
    unsigned __int16 *p; // [esp+4h] [ebp-4h]

    p = tessTable[lodMask];
    count = 0;
    while ( *p != 0xFFFF )
    {
        *lockedindex++ = verts[*p++];
        ++count;
    }
    return count;
}

void __cdecl R_FreeWaterSimulationVertexBuffers()
{
    int n; // [esp+0h] [ebp-4h]

    if ( vertexBuffersAllocated )
    {
        if ( data.vertexBuffer )
        {
            R_FreeStaticVertexBuffer(data.vertexBuffer);
            data.vertexBuffer = 0;
        }
        for ( n = 0; n < 2; ++n )
        {
            if ( data.indexBuffer[n] )
                R_FreeStaticIndexBuffer(data.indexBuffer[n]);
            data.indexBuffer[n] = 0;
        }
        vertexBuffersAllocated = 0;
    }
}

void __cdecl R_WaterSimulationNotifyBullet(const float *hitpos)
{
    debugpoint_t p; // [esp+0h] [ebp-Ch] BYREF

    if ( r_watersim_debug->current.enabled )
    {
        p.pos[0] = *hitpos;
        p.pos[1] = hitpos[1];
        p.pos[2] = hitpos[2];
        debugPoints.add(&p);
        //fifo_t<debugpoint_t,16>::add(&debugPoints, &p);
    }
}

void __cdecl R_WaterSimulationSetHeight(const float *center, float radius, float targetHeight, float lerpRate)
{
    float4 *v4; // ecx
    float interpolate; // [esp+8h] [ebp-28h] BYREF
    __int16 waterheight; // [esp+Ch] [ebp-24h]
    int bufId; // [esp+10h] [ebp-20h] BYREF
    int x; // [esp+14h] [ebp-1Ch]
    int y; // [esp+18h] [ebp-18h]
    int u; // [esp+1Ch] [ebp-14h]
    int v; // [esp+20h] [ebp-10h]
    int size; // [esp+24h] [ebp-Ch]
    int sx; // [esp+28h] [ebp-8h] BYREF
    int sy; // [esp+2Ch] [ebp-4h] BYREF

    if ( data.enabled )
    {
        size = (int)(float)(radius / config.gridScale);
        if ( ConvertWorldToGrid(center, &sx, &sy) )
        {
            for ( v = 0; v < size; ++v )
            {
                for ( u = 0; u < size; ++u )
                {
                    x = u + sx - size / 2;
                    y = v + sy - size / 2;
                    if ( x >= 1 && y >= 1 && x < 255 && y < 255 )
                    {
                        GetBuffersForPoint(x, y, &bufId, &interpolate);
                        waterheight = data.waterheight.v[256 * y + x];
                        if ( (float)waterheight > (float)(center[2] - 10.0) && (float)(center[2] + 200.0) > (float)waterheight )
                        {
                            v4 = &data.buffer[bufId].v[256 * y + x];
                            v4->v[1] = (float)((float)(targetHeight - v4->v[1]) * lerpRate) + v4->v[1];
                        }
                    }
                }
            }
        }
    }
}

void __cdecl GetBuffersForPoint(int u, int v, int *bufId, float *interpolate)
{
    int diff; // [esp+0h] [ebp-8h]
    int lastUpdateV; // [esp+4h] [ebp-4h]

    lastUpdateV = 16 * (data.frame & 0xF);
    diff = (unsigned __int8)(16 * (data.frame & 0xF) - v);
    *bufId = (data.frame & 0x10) == 0;
    if ( v > lastUpdateV )
        *bufId ^= 1u;
    *interpolate = (float)diff * 0.00390625;
}

char __cdecl ConvertWorldToGrid(const float *worldPos, int *u, int *v)
{
    const float inverseGridScale = 1.0f / config.gridScale;
    *u = (int)(*worldPos * inverseGridScale) - data.gridX;
    *v = (int)(worldPos[1] * inverseGridScale) - data.gridY;
    if ( (int)abs(*u) >= 128 || (int)abs(*v) >= 128 )
        return 0;
    *u = (unsigned __int8)(LOBYTE(data.centerU) + *u);
    *v = (unsigned __int8)(LOBYTE(data.centerV) + *v);
    return 1;
}

void __cdecl R_WaterSimulationSimpleDisplace(const float *hitpos, float radius, float height)
{
    float v3; // [esp+0h] [ebp-4Ch]
    float v4; // [esp+4h] [ebp-48h]
    float v5; // [esp+Ch] [ebp-40h]
    float v6; // [esp+10h] [ebp-3Ch]
    float interpolate; // [esp+18h] [ebp-34h] BYREF
    int bufId; // [esp+1Ch] [ebp-30h] BYREF
    float *value; // [esp+20h] [ebp-2Ch]
    float v10; // [esp+24h] [ebp-28h]
    float scale; // [esp+28h] [ebp-24h]
    int x; // [esp+2Ch] [ebp-20h]
    int y; // [esp+30h] [ebp-1Ch]
    float dy; // [esp+34h] [ebp-18h]
    int u; // [esp+38h] [ebp-14h]
    int v; // [esp+3Ch] [ebp-10h]
    int size; // [esp+40h] [ebp-Ch]
    int sx; // [esp+44h] [ebp-8h] BYREF
    int sy; // [esp+48h] [ebp-4h] BYREF
    float heighta; // [esp+5Ch] [ebp+10h]

    if ( data.enabled )
    {
        v6 = (float)(radius - max_plop_size) < 0.0 ? radius : max_plop_size;
        v4 = (float)(0.0 - radius) < 0.0 ? v6 : 0.0f;
        v5 = (float)(height - max_plop_height) < 0.0 ? height : max_plop_height;
        if ( (float)((-(max_plop_height)) - height) < 0.0 )
            v3 = v5;
        else
            v3 = -max_plop_height;
        size = (int)(float)(v4 / config.gridScale);
        heighta = v3 / config.gridScale;
        if ( ConvertWorldToGrid(hitpos, &sx, &sy) )
        {
            for ( v = 0; v < size; ++v )
            {
                for ( u = 0; u < size; ++u )
                {
                    v10 = (float)u - (float)((float)size / 2.0);
                    dy = (float)v - (float)((float)size / 2.0);
                    scale = 1.0 - (float)(sqrtf((float)(v10 * v10) + (float)(dy * dy)) / (float)size);
                    if ( scale < 0.0 )
                        scale = 0.0f;
                    x = u + sx - size / 2;
                    y = v + sy - size / 2;
                    if ( x >= 1 && y >= 1 && x < 255 && y < 255 )
                    {
                        GetBuffersForPoint(x, y, &bufId, &interpolate);
                        value = data.buffer[bufId].v[256 * y + x].v;
                        value[1] = (float)((float)(heighta * scale) * scale) + value[1];
                    }
                }
            }
        }
    }
}

void __cdecl R_WaterSimulationNotifyExplosionEx(const float *hitpos, int size, float height)
{
    R_WaterSimulationSimpleDisplace(hitpos, (float)size * config.gridScale, height * config.gridScale);
}

void __cdecl R_WaterSimulationNotifyExplosion(const float *hitpos)
{
    R_WaterSimulationSimpleDisplace(hitpos, (float)explodesize * config.gridScale, explodeheight * config.gridScale);
}

void R_WaterSimulationApplyWaveSeed()
{
    unsigned int v0; // eax
    int du; // [esp+8h] [ebp-18h]
    unsigned int attempts; // [esp+Ch] [ebp-14h]
    int dv; // [esp+10h] [ebp-10h]
    float v[3]; // [esp+14h] [ebp-Ch] BYREF

    data.waveTimeleft = data.waveTimeleft - 16.666666;
    if ( data.waveTimeleft < 0.0 )
    {
        data.waveTimeleft = data.waveTimeleft + config.waveSeedDelay;
        attempts = 100;
        do
        {
            du = water_rand() % 256 - 128;
            dv = water_rand() % 256 - 128;
            v0 = attempts--;
        }
        while ( v0 && dv * dv + du * du < 0x4000 );
        v[0] = (float)(data.gridX + du) * config.gridScale;
        v[1] = (float)(data.gridY + dv) * config.gridScale;
        v[2] = 0.0f;
        R_WaterSimulationSimpleDisplace(
            v,
            config.waveSeedRadius,
            (-(config.waveSeedHeight)));
    }
}

int __cdecl water_rand()
{
    water_rand_seed = 214013 * water_rand_seed + 2531011;
    return (water_rand_seed >> 16) & 0x7FFF;
}

void __cdecl R_WaterSimulationCmd(const WaterSimulationCmd *rawdata)
{
    water_rand_seed = data.frame;
    LockUpdateMutex();
    R_WaterSimulationUpdateFrame(rawdata);
    FlushBuffers();
    R_WaterSimulationApplyWaveSeed();
    FlushBuffers();
    UnlockUpdateMutex();
    R_WaterSimulationSkinMesh(rawdata);
}

unsigned int LockUpdateMutex()
{
#if defined(__ANDROID__)
    return bo1::android::LockWaterUpdateMutex(waterUpdateMutex);
#else
    return WaitForSingleObject(waterUpdateMutex, 0xFFFFFFFF);
#endif
}

bool UnlockUpdateMutex()
{
#if defined(__ANDROID__)
    return bo1::android::UnlockWaterUpdateMutex(waterUpdateMutex);
#else
    return ReleaseMutex(waterUpdateMutex);
#endif
}

// Pack the shader's byte format after rounding, with saturation at each end.
static unsigned char WaterSkinPackByte(float value)
{
    value += 0.5f;
    if (value < 0.0f) value = 0.0f;
    if (value > 255.0f) value = 255.0f;
    return static_cast<unsigned char>(static_cast<int>(value));
}

static void WaterSkinNormalize(float4 *value)
{
    // Preserve the retail summation order (X, Z, Y) before the square root.
    const float lengthSq = (value->v[0] * value->v[0] + value->v[2] * value->v[2])
                         + value->v[1] * value->v[1];
    const float length = sqrtf(lengthSq);
    const float inverseLength = 1.0f / (length > 0.0f ? length : 1.0f);
    for (unsigned c = 0; c < 3; ++c)
        value->v[c] *= inverseLength;
}

void R_WaterSimulationSkinMesh(const WaterSimulationCmd *cmd)
{
    // Each mesh is a 17x17 grid. ReadValue wraps the 256-cell channels and
    // swaps simulation Y/Z to the position convention used by the water shader.
    for (unsigned tile = 0; tile < 16; ++tile)
    {
        const tilemesh_t *mesh = cmd->skin.meshesEA[tile];
        if (!mesh) continue;
        if (!Sys_QueryD3DDeviceOKEvent()) return;
        GfxWaterVertex *output = static_cast<GfxWaterVertex *>(
            R_LockVertexBuffer(data.vertexBuffer, 0, 0, 0x1000u));
        if (!output) return;
        output += mesh->baseVertex;
        const float inverseGridScale = 1.0f / config.gridScale;

        for (unsigned v = 0; v <= 16; ++v)
        {
            const int sourceV = mesh->srcV + static_cast<int>(v);
            const int worldV = mesh->worldY + static_cast<int>(v);
            for (unsigned u = 0; u <= 16; ++u, ++output)
            {
                const int sourceU = mesh->srcU + static_cast<int>(u);
                const int worldU = mesh->worldX + static_cast<int>(u);
                float4 samples[3][3];
                for (int y = -1; y <= 1; ++y)
                    for (int x = -1; x <= 1; ++x)
                        ReadValue(&samples[y + 1][x + 1],
                            static_cast<unsigned char>(sourceU + x), sourceV + y, cmd->skin.srcBuf);
                const float4 &a = samples[0][0], &b = samples[0][1], &c = samples[0][2];
                const float4 &d = samples[1][0], &e = samples[1][1], &f = samples[1][2];
                const float4 &g = samples[2][0], &h = samples[2][1], &i = samples[2][2];
                float4 average{}, derivativeU{}, derivativeV{};
                for (unsigned lane = 0; lane < 4; ++lane)
                    average.v[lane] = (h.v[lane] + (f.v[lane] + e.v[lane]) + i.v[lane]) * 0.25f;
                for (unsigned lane = 0; lane < 3; ++lane)
                {
                    derivativeU.v[lane] = (((c.v[lane] + i.v[lane]) - a.v[lane]) - g.v[lane])
                                         * 0.5f + f.v[lane] - d.v[lane];
                    derivativeV.v[lane] = (((g.v[lane] + i.v[lane]) - a.v[lane]) - c.v[lane])
                                         * 0.5f + h.v[lane] - b.v[lane];
                }
                derivativeU.v[0] += 4.0f;
                derivativeV.v[1] += 4.0f;
                WaterSkinNormalize(&derivativeU);
                WaterSkinNormalize(&derivativeV);

                const unsigned sourceIndex = 256u * static_cast<unsigned char>(sourceV)
                                           + static_cast<unsigned char>(sourceU);
                const float heightOffset = inverseGridScale * static_cast<float>(data.waterheight.v[sourceIndex])
                                         - static_cast<float>(mesh->worldZ);
                output->pos[0] = WaterSkinPackByte((average.v[0] + static_cast<float>(u)) * 8.0f + 64.0f);
                output->pos[1] = WaterSkinPackByte((average.v[1] + static_cast<float>(v)) * 8.0f + 64.0f);
                output->pos[2] = WaterSkinPackByte((average.v[2] + heightOffset) * 8.0f + 64.0f);
                float foam = average.v[3] * 63.0f;
                if (foam < 0.0f) foam = 0.0f;
                if (foam > 62.0f) foam = 62.0f;
                const unsigned seamU = u == 16 && static_cast<unsigned char>(worldU) == 0 ? 64 : 0;
                const unsigned seamV = v == 16 && static_cast<unsigned char>(worldV) == 0 ? 128 : 0;
                output->pos[3] = WaterSkinPackByte(static_cast<float>(seamV + seamU) + foam);
                // XYAB packs the horizontal components of the two normalized derivatives.
                output->normal[0] = WaterSkinPackByte(derivativeU.v[0] * 127.0f + 128.0f);
                output->normal[1] = WaterSkinPackByte(derivativeU.v[1] * 127.0f + 128.0f);
                output->normal[2] = WaterSkinPackByte(derivativeV.v[0] * 127.0f + 128.0f);
                output->normal[3] = WaterSkinPackByte(derivativeV.v[1] * 127.0f + 128.0f);
                output->color = data.colors.v[sourceIndex];
                output->texCoord[0] = WaterSkinPackByte(static_cast<float>(static_cast<unsigned char>(worldU)));
                output->texCoord[1] = WaterSkinPackByte(static_cast<float>(static_cast<unsigned char>(worldV)));
                output->texCoord[2] = WaterSkinPackByte(static_cast<float>(static_cast<signed char>(data.shoredist.v[sourceIndex])) + 128.0f);
                output->texCoord[3] = WaterSkinPackByte(static_cast<float>(2 * v + 112));
            }
        }
        R_UnlockVertexBuffer(data.vertexBuffer);
    }
}

float4 *__cdecl ReadValue(float4 *result, unsigned __int8 u, int v, unsigned int bufnum)
{
    float4 v5[3]; // [esp+0h] [ebp-60h]
    float4 *p_value; // [esp+34h] [ebp-2Ch]
    float4 v7; // [esp+38h] [ebp-28h]
    float4 *vp; // [esp+48h] [ebp-18h]
    float4 value; // [esp+4Ch] [ebp-14h] BYREF

    vp = data.buffer[bufnum ^ (v >> 8) & 1].v;
    value = vp[256 * (unsigned __int8)v + u];
    v7 = g_swizzleXZYW;
    v5[0] = value;
    v5[1] = value;
    v5[2] = g_swizzleXZYW;
    p_value = &value;
    value.unitVec[0].array[0] = v5[0].unitVec[0].array[g_swizzleXZYW.unitVec[0].array[3]];
    value.unitVec[0].array[1] = v5[0].unitVec[0].array[g_swizzleXZYW.unitVec[0].array[2]];
    value.unitVec[0].array[2] = v5[0].unitVec[0].array[g_swizzleXZYW.unitVec[0].array[1]];
    value.unitVec[0].array[3] = v5[0].unitVec[0].array[g_swizzleXZYW.unitVec[0].array[0]];
    value.unitVec[1].array[0] = v5[0].unitVec[0].array[g_swizzleXZYW.unitVec[1].array[3]];
    value.unitVec[1].array[1] = v5[0].unitVec[0].array[g_swizzleXZYW.unitVec[1].array[2]];
    value.unitVec[1].array[2] = v5[0].unitVec[0].array[g_swizzleXZYW.unitVec[1].array[1]];
    value.unitVec[1].array[3] = v5[0].unitVec[0].array[g_swizzleXZYW.unitVec[1].array[0]];
    value.unitVec[2].array[0] = v5[0].unitVec[0].array[g_swizzleXZYW.unitVec[2].array[3]];
    value.unitVec[2].array[1] = v5[0].unitVec[0].array[g_swizzleXZYW.unitVec[2].array[2]];
    value.unitVec[2].array[2] = v5[0].unitVec[0].array[g_swizzleXZYW.unitVec[2].array[1]];
    value.unitVec[2].array[3] = v5[0].unitVec[0].array[g_swizzleXZYW.unitVec[2].array[0]];
    value.unitVec[3].array[0] = v5[0].unitVec[0].array[g_swizzleXZYW.unitVec[3].array[3]];
    value.unitVec[3].array[1] = v5[0].unitVec[0].array[g_swizzleXZYW.unitVec[3].array[2]];
    value.unitVec[3].array[2] = v5[0].unitVec[0].array[g_swizzleXZYW.unitVec[3].array[1]];
    value.unitVec[3].array[3] = v5[0].unitVec[0].array[g_swizzleXZYW.unitVec[3].array[0]];
    *result = value;
    return result;
}

double __cdecl R_WaterSimulationGetBaseHeight(const float *queryPos)
{
    int sx; // [esp+8h] [ebp-8h] BYREF
    int sy; // [esp+Ch] [ebp-4h] BYREF

    if ( data.enabled && ConvertWorldToGrid(queryPos, &sx, &sy) )
        return (double)data.waterheight.v[256 * sy + sx];
    else
        return R_WaterSimulationGetHeightGlobal(queryPos);
}

double __cdecl R_WaterSimulationGetHeight(const float *queryPos)
{
    float posc[3]; // [esp+0h] [ebp-1Ch] BYREF
    int sx; // [esp+10h] [ebp-Ch] BYREF
    int sy; // [esp+18h] [ebp-4h] BYREF

    if ( !r_watersim_enabled || !r_watersim_enabled->current.enabled )
        return R_WaterSimulationGetHeightGlobal(queryPos);
    if ( !data.enabled || !ConvertWorldToGrid(queryPos, &sx, &sy) )
        return R_WaterSimulationGetHeightGlobal(queryPos);
    R_WaterSimulationGetGridPosition(sx, sy, 0, posc);
    return (float)((float)(posc[2] * config.gridScale) + (float)data.waterheight.v[256 * sy + sx]);
}

void __cdecl R_WaterSimulationGetGridPosition(unsigned __int8 sx, unsigned __int8 sy, int timeOffset, float *v)
{
    float4 *v4; // ecx
    float4 *v5; // eax
    float v6; // [esp+Ch] [ebp-50h]
    float v7; // [esp+10h] [ebp-4Ch]
    float v8; // [esp+14h] [ebp-48h]
    float v9; // [esp+1Ch] [ebp-40h]
    float v10; // [esp+20h] [ebp-3Ch]
    float v11; // [esp+24h] [ebp-38h]
    float t; // [esp+2Ch] [ebp-30h] BYREF
    channel_t<float4> *old_pos; // [esp+30h] [ebp-2Ch]
    float oldposvalue[4]; // [esp+34h] [ebp-28h] BYREF
    float temp; // [esp+44h] [ebp-18h]
    float posvalue[4]; // [esp+48h] [ebp-14h] BYREF
    int bufId; // [esp+58h] [ebp-4h] BYREF

    GetBuffersForPoint(sx, sy, &bufId, &t);
    old_pos = &data.buffer[bufId ^ 1];
    v4 = &data.buffer[bufId].v[256 * sy + sx];
    v9 = v4->v[1];
    v10 = v4->v[2];
    v11 = v4->v[3];
    posvalue[0] = v4->v[0];
    posvalue[1] = v9;
    posvalue[2] = v10;
    posvalue[3] = v11;
    t = (float)((float)timeOffset * 0.0625) + t;
    v5 = &old_pos->v[256 * sy + sx];
    v6 = v5->v[1];
    v7 = v5->v[2];
    v8 = v5->v[3];
    oldposvalue[0] = v5->v[0];
    oldposvalue[1] = v6;
    oldposvalue[2] = v7;
    oldposvalue[3] = v8;
    Vec3Lerp(oldposvalue, posvalue, t, v);
    temp = v[1];
    v[1] = v[2];
    v[2] = temp;
}

void __cdecl R_WaterSimulationGetInfo(const float *queryPos, WaterSimInfo *info)
{
    float oldposc[3]; // [esp+48h] [ebp-78h] BYREF
    float posc[3]; // [esp+54h] [ebp-6Ch] BYREF
    float pos3[3]; // [esp+60h] [ebp-60h] BYREF
    float normal[3]; // [esp+6Ch] [ebp-54h] BYREF
    float tangent[3]; // [esp+78h] [ebp-48h] BYREF
    float pos0[3]; // [esp+84h] [ebp-3Ch] BYREF
    float binormal[3]; // [esp+90h] [ebp-30h] BYREF
    __int16 waterheight; // [esp+9Ch] [ebp-24h]
    float pos1[3]; // [esp+A0h] [ebp-20h] BYREF
    int sx; // [esp+ACh] [ebp-14h] BYREF
    int sy; // [esp+B0h] [ebp-10h] BYREF
    float pos2[3]; // [esp+B4h] [ebp-Ch] BYREF

    if ( data.enabled && ConvertWorldToGrid(queryPos, &sx, &sy) )
    {
        info->pos[0] = *queryPos;
        info->pos[1] = queryPos[1];
        info->pos[2] = queryPos[2];
        info->pos[2] = -FLT_MAX;
        info->normal[0] = 0.0f;
        info->normal[1] = 0.0f;
        info->normal[2] = 1.0f;
        info->waveVelocity[0] = 0.0f;
        info->waveVelocity[1] = 0.0f;
        info->waveVelocity[2] = 0.0f;
        waterheight = data.waterheight.v[256 * sy + sx];
        R_WaterSimulationGetGridPosition(sx, sy, 0, posc);
        R_WaterSimulationGetGridPosition(sx, sy, -1, oldposc);
        R_WaterSimulationGetGridPosition(sx - 1, sy, 0, pos0);
        R_WaterSimulationGetGridPosition(sx + 1, sy, 0, pos1);
        R_WaterSimulationGetGridPosition(sx, sy - 1, 0, pos2);
        R_WaterSimulationGetGridPosition(sx, sy + 1, 0, pos3);
        pos0[0] = pos0[0] - 1.0;
        pos1[0] = pos1[0] + 1.0;
        pos2[1] = pos2[1] - 1.0;
        pos3[1] = pos3[1] + 1.0;
        tangent[0] = pos1[0] - pos0[0];
        tangent[1] = pos1[1] - pos0[1];
        tangent[2] = pos1[2] - pos0[2];
        binormal[0] = pos3[0] - pos2[0];
        binormal[1] = pos3[1] - pos2[1];
        binormal[2] = pos3[2] - pos2[2];
        Vec3Cross(tangent, binormal, normal);
        Vec3Normalize(tangent);
        Vec3Normalize(binormal);
        Vec3Normalize(normal);
        info->waveVelocity[0] = (float)((float)(16.0 * config.gridScale) * (float)(pos0[2] - pos1[2]))
                                                    * (float)(posc[2] - oldposc[2]);
        info->waveVelocity[1] = (float)((float)(16.0 * config.gridScale) * (float)(pos2[2] - pos3[2]))
                                                    * (float)(posc[2] - oldposc[2]);
        info->pos[2] = (float)(posc[2] * config.gridScale) + (float)waterheight;
        info->tangent[0] = tangent[0];
        info->tangent[1] = tangent[1];
        info->tangent[2] = tangent[2];
        info->binormal[0] = binormal[0];
        info->binormal[1] = binormal[1];
        info->binormal[2] = binormal[2];
        info->normal[0] = normal[0];
        info->normal[1] = normal[1];
        info->normal[2] = normal[2];
    }
    else
    {
        info->pos[0] = *queryPos;
        info->pos[1] = queryPos[1];
        info->pos[2] = queryPos[2];
        info->pos[2] = R_WaterSimulationGetHeightGlobal(queryPos);
        info->tangent[0] = 1.0f;
        info->tangent[1] = 0.0f;
        info->tangent[2] = 0.0f;
        info->binormal[0] = 0.0f;
        info->binormal[1] = 1.0f;
        info->binormal[2] = 0.0f;
        info->normal[0] = 0.0f;
        info->normal[1] = 0.0f;
        info->normal[2] = 1.0f;
        info->waveVelocity[0] = 0.0f;
        info->waveVelocity[1] = 0.0f;
        info->waveVelocity[2] = 0.0f;
    }
}

double __cdecl R_WaterSimulationGetHeightGlobal(const float *queryPos)
{
    float v2; // [esp+Ch] [ebp-2Ch]
    float v3; // [esp+10h] [ebp-28h]
    ComWaterCell *cells[4]; // [esp+20h] [ebp-18h] BYREF
    int gridY; // [esp+30h] [ebp-8h]

    const float inverseGridSpacing = 1.0f / 25.0f;
    v3 = floor(*queryPos * inverseGridSpacing + 0.5f);
    v2 = floor(queryPos[1] * inverseGridSpacing + 0.5f);
    gridY = (int)v2;
    if ( R_WaterFindCells((int)v3, gridY, cells) )
        return (double)cells[0]->waterheight;
    else
        return -131072.0;
}

char __cdecl R_WaterSimulationGetAverageHeight(float *pHeight)
{
    __int16 h; // [esp+0h] [ebp-8h]
    unsigned int n; // [esp+4h] [ebp-4h]

    if ( !comWorld.numWaterCells )
        return 0;
    *pHeight = (float)comWorld.waterCells->waterheight;
    for ( n = 0; n < comWorld.numWaterCells; n += 100 )
    {
        h = comWorld.waterCells[n].waterheight;
        if ( h != -32768 )
        {
            *pHeight = (float)h;
            return 1;
        }
    }
    return 1;
}
