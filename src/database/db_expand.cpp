#include <platform/bo1_native_width.h>
#include "db_expand.hpp"

#if BO1_NATIVE_64BIT

#include "db_stream.h"
// The standalone envelope gate selects actual production bodies only.
// Normal engine builds retain the complete expander and existing includes.
#if defined(BO1_ASSETLIST_GATE)
#include "db_assetlist_types.h"
#if defined(BO1_TECHSET_GATE)
#include "db_techset_types.h"
#endif
#if defined(BO1_RAWFILE_HEADER_GATE)
#include "db_rawfile_types.h"
#endif
#if defined(BO1_STRINGTABLE_HEADER_GATE)
#include <universal/com_stringtable.h>
#endif
void __cdecl DB_LoadXFileData(unsigned __int8 *pos, int size);
#else
#include "db_stream_load.h"
#include "db_registry.h"
#include "db_file_load.h"
#include <universal/com_stringtable.h>
#include <gfx_d3d/r_material.h>
#include <xanim/xmodel.h>
#include <xanim/xanim.h>
#include <physics/phys_colgeom.h>
#include <physics/physpreset_load_obj.h>
#include <physics/physconstraints_load_obj.h>
#include <physics/rope.h>
#include <qcommon/cm_trace.h>
#include <qcommon/cm_staticmodel.h>
#include <qcommon/cm_load_obj.h>
#include <DynEntity/DynEntity_client.h>
#include <flame/flame_system.h>
#endif
#include <universal/assertive.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <stdexcept>

extern int g_dbTopAssetType;

#pragma pack(push, 1)
struct XAssetList_32
{
    int stringList_count;
    uint32_t stringList_strings;
    int assetCount;
    uint32_t assets;
};
struct XAsset_32
{
    int type;
    uint32_t header;
};
struct StringTable_32
{
    uint32_t name;
    int columnCount;
    int rowCount;
    uint32_t values;
    uint32_t cellIndex;
};
struct StringTableCell_32
{
    uint32_t string;
    int hash;
};
// Exact Win32 GfxImage (0x34). Field order matches r_material.h; pointers are uint32.
struct GfxImage_32
{
    uint32_t texture; // GfxTexture / loadDef
    uint8_t mapType;
    uint8_t semantic;
    uint8_t category;
    uint8_t delayLoadPixels;
    uint8_t picmip[2];
    uint8_t noPicmip;
    uint8_t track;
    int cardMemory[2];
    uint16_t width;
    uint16_t height;
    uint16_t depth;
    uint8_t levelCount;
    uint8_t streaming;
    uint32_t baseSize;
    uint32_t pixels;
    uint32_t loadedSize;
    uint8_t skippedMipLevels;
    uint8_t pad[3];
    uint32_t name;
    uint32_t hash;
};
struct RawFile_32
{
    uint32_t name;
    int len;
    uint32_t buffer;
};
#pragma pack(pop)

static_assert(sizeof(RawFile_32) == 12, "Win32 RawFile");
static_assert(sizeof(XAssetList_32) == 16, "Win32 XAssetList");
static_assert(sizeof(XAsset_32) == 8, "Win32 XAsset");
static_assert(sizeof(StringTable_32) == 20, "Win32 StringTable");
static_assert(sizeof(StringTableCell_32) == 8, "Win32 StringTableCell");
static_assert(sizeof(GfxImage_32) == 52, "Win32 GfxImage");

// Immediate FF->memory load used at stream index 4+ (same path as Load_Stream else-branch).
// Accumulates Win32/disk bytes so DB_Expand_Commit can remap baked stream offsets.
static int g_expandReadAccum = 0;

// Corrupt stream counts (esp. negative â†’ size_t wrap) must not throw std::_Xlength_error (0xE06D7363).
static int DB_Expand_ClampCount(int count, const char *tag)
{
    constexpr int kMax = 1 << 20;
    if (count >= 0 && count <= kMax)
        return count;
    static int s_logged;
    if (s_logged < 32)
    {
        ++s_logged;
    }
    return 0;
}

// The Win32 image currently being expanded: lets DB_ExpandPtrStore recover each pointer field's exact Win32 stream
// position (buffer start + field offset), so later alias offsets that name that field resolve to its native slot.
static const unsigned __int8 *g_expandDiskBase = nullptr;
static int g_expandDiskBytes = 0;
static unsigned int g_expandDiskW = 0;

static void DB_Expand_ReadFile(void *dst, int fileBytes)
{
    if (fileBytes > 0)
    {
        // Audited loaders stage one complete disk image before native stores.
        // Reserve metadata and snapshot ALL live TEMP slots before those stores;
        // actual native extent comes from Commit, without an expansion guess.
        try
        {
            if (g_expandReadAccum != 0)
                throw std::logic_error("Interleaved multi-read expansion is unsupported");
            DB_StreamPrepareWrite(static_cast<size_t>(fileBytes) / sizeof(uint32_t));
            // Block 1 is zero-filled; delayed-stream support remains unchanged.
            if (g_streamPosIndex == 1)
                memset(dst, 0, static_cast<size_t>(fileBytes));
            else
            {
                if (g_streamPosIndex - 1 < 3)
                    Bo1_Diag("Expand ReadFile at delayed stream index %u (%d bytes) - not supported", g_streamPosIndex, fileBytes);
                DB_LoadXFileData(static_cast<unsigned __int8 *>(dst), fileBytes);
            }
        }
        catch (...)
        {
            // Scratch failure has not overwritten owned bytes in the audited
            // single-read loaders. Do not leave stale disk provenance on retry.
            DB_StreamCancelWrite(g_expandReadAccum != 0);
            g_expandReadAccum = 0;
            g_expandDiskBase = nullptr;
            g_expandDiskBytes = 0;
            g_expandDiskW = 0;
            throw;
        }
        g_expandDiskBase = static_cast<const unsigned __int8 *>(dst);
        g_expandDiskBytes = fileBytes;
        g_expandDiskW = DB_StreamWin32Pos(g_streamPosIndex);
        g_expandReadAccum = fileBytes;
    }
}

// Widen a 32-bit FF pointer field into its native slot and record where that field lives in the Win32 stream.
template <class T>
static inline void DB_ExpandPtrStore(uintptr_t *slot, const T &src)
{
    *slot = DB_ExpandPtr32(static_cast<uint32_t>(src));
    const unsigned __int8 *p = reinterpret_cast<const unsigned __int8 *>(&src);
    if (g_expandDiskBase && p >= g_expandDiskBase && p + sizeof(uint32_t) <= g_expandDiskBase + g_expandDiskBytes)
        DB_StreamRegisterAliasSlot(g_streamPosIndex, g_expandDiskW + static_cast<unsigned int>(p - g_expandDiskBase), slot);
}

static void DB_Expand_Commit(int nativeSize, int count = 1)
{
#if BO1_NATIVE_64BIT
    // Disk bytes read == Win32 stream footprint. Empty commits keep both cursors still.
    DB_SetExpandWin32Size(g_expandReadAccum);
    // Expanded arrays: element starts inside the commit map exactly (Win32 stride != native stride).
    if (count > 1 && g_expandReadAccum > 0 && g_expandReadAccum % count == 0 && nativeSize % count == 0 &&
        g_expandReadAccum != nativeSize)
        DB_SetExpandElem(g_expandReadAccum / count, nativeSize / count);
#endif
    g_expandReadAccum = 0;
    DB_IncStreamPos(nativeSize);
}

void DB_ExpandLoad_XAssetList(XAssetList *dst)
{
    XAssetList_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    g_expandReadAccum = 0; // side buffer, not an XBlock stream commit
    DB_StreamCancelWrite(); // External envelope stores must not arm a TEMP commit.
    memset(dst, 0, sizeof(*dst));
    dst->stringList.count = disk.stringList_count;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->stringList.strings), disk.stringList_strings);
    dst->assetCount = disk.assetCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->assets), disk.assets);
}

#if !defined(BO1_ASSETLIST_GATE) || defined(BO1_FRONTEND_TABLES_GATE)
void DB_ExpandLoad_XAssetArray(XAsset *dst, int count)
{
    iassert(dst == reinterpret_cast<XAsset *>(DB_GetStreamPos()));
    iassert(count >= 0);

    count = DB_Expand_ClampCount(count, "XAsset_32");
    std::vector<XAsset_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(XAsset_32) * count));

    memset(dst, 0, sizeof(XAsset) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        dst[i].type = static_cast<XAssetType>(disk[static_cast<size_t>(i)].type);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].header), disk[static_cast<size_t>(i)].header);
    }
    DB_Expand_Commit(static_cast<int>(sizeof(XAsset) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_Ptr32Array(void *nativeDst, int count)
{
    iassert(nativeDst == DB_GetStreamPos());
    iassert(count >= 0);

    const bool misaligned = nativeDst != DB_GetStreamPos();
    if (misaligned || g_dbTopAssetType == 21 || count < 0 || count > 4096)
    {
    }
    count = DB_Expand_ClampCount(count, "uint32_t");
    std::vector<uint32_t> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(uint32_t) * count));

    auto *out = static_cast<uintptr_t *>(nativeDst);
    for (int i = 0; i < count; ++i)
        DB_ExpandPtrStore(&out[i], disk[static_cast<size_t>(i)]);
    DB_Expand_Commit(static_cast<int>(sizeof(uintptr_t) * static_cast<size_t>(count)), count);
}

#endif // normal engine or bounded frontend tables

#if !defined(BO1_ASSETLIST_GATE) || defined(BO1_STRINGTABLE_HEADER_GATE)
void DB_ExpandLoad_StringTable(StringTable *dst)
{
    iassert(dst == reinterpret_cast<StringTable *>(DB_GetStreamPos()));

    StringTable_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    if (disk.name == 0 && disk.columnCount == 0 && disk.rowCount == 0)
    {
    }
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    dst->columnCount = disk.columnCount;
    dst->rowCount = disk.rowCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->values), disk.values);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->cellIndex), disk.cellIndex);
    DB_Expand_Commit(static_cast<int>(sizeof(StringTable)));
}

#endif // normal engine or bounded first StringTable header

#if !defined(BO1_ASSETLIST_GATE) || defined(BO1_STRINGTABLE_CELLS_GATE)
void DB_ExpandLoad_StringTableCellArray(StringTableCell *dst, int count)
{
    iassert(dst == reinterpret_cast<StringTableCell *>(DB_GetStreamPos()));
    iassert(count >= 0);

    count = DB_Expand_ClampCount(count, "StringTableCell_32");
    std::vector<StringTableCell_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(StringTableCell_32) * count));

    memset(dst, 0, sizeof(StringTableCell) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].string), disk[static_cast<size_t>(i)].string);
        dst[i].hash = disk[static_cast<size_t>(i)].hash;
    }
    DB_Expand_Commit(static_cast<int>(sizeof(StringTableCell) * static_cast<size_t>(count)), count);
}

#endif // normal engine or bounded cell records

#if !defined(BO1_ASSETLIST_GATE)
void DB_ExpandLoad_GfxImage(GfxImage *dst)
{
    iassert(dst == reinterpret_cast<GfxImage *>(DB_GetStreamPos()));

    GfxImage_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));

    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->texture), disk.texture);
    dst->mapType = disk.mapType;
    dst->semantic = disk.semantic;
    dst->category = disk.category;
    dst->delayLoadPixels = disk.delayLoadPixels != 0;
    dst->picmip.platform[0] = disk.picmip[0];
    dst->picmip.platform[1] = disk.picmip[1];
    dst->noPicmip = disk.noPicmip != 0;
    dst->track = disk.track;
    dst->cardMemory.platform[0] = disk.cardMemory[0];
    dst->cardMemory.platform[1] = disk.cardMemory[1];
    dst->width = disk.width;
    dst->height = disk.height;
    dst->depth = disk.depth;
    dst->levelCount = disk.levelCount;
    dst->streaming = disk.streaming;
    dst->baseSize = disk.baseSize;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->pixels), disk.pixels);
    dst->loadedSize = disk.loadedSize;
    dst->skippedMipLevels = disk.skippedMipLevels;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    dst->hash = disk.hash;

    DB_Expand_Commit(static_cast<int>(sizeof(GfxImage)));
}

#pragma pack(push, 1)
struct MaterialInfo_32
{
    uint32_t name;
    uint32_t gameFlags;
    uint8_t pad;
    uint8_t sortKey;
    uint8_t textureAtlasRowCount;
    uint8_t textureAtlasColumnCount;
    uint8_t padDrawSurf[4];
    uint64_t drawSurf;
    uint32_t surfaceTypeBits;
    uint32_t layeredSurfaceTypes;
    uint16_t hashIndex;
    uint8_t padEnd[6];
};
struct Material_32
{
    MaterialInfo_32 info;
    uint8_t stateBitsEntry[130];
    uint8_t textureCount;
    uint8_t constantCount;
    uint8_t stateBitsCount;
    uint8_t stateFlags;
    uint8_t cameraRegion;
    uint8_t maxStreamedMips;
    uint32_t techniqueSet;
    uint32_t textureTable;
    uint32_t constantTable;
    uint32_t stateBitsTable;
};
struct MaterialTextureDef_32
{
    uint32_t nameHash;
    char nameStart;
    char nameEnd;
    uint8_t samplerState;
    uint8_t semantic;
    uint8_t isMatureContent;
    uint8_t pad[3];
    uint32_t u;
};
struct MaterialTechniqueHeader_32
{
    uint32_t name;
    uint16_t flags;
    uint16_t passCount;
};
struct MaterialPass_32
{
    uint32_t vertexDecl;
    uint32_t vertexShader;
    uint32_t pixelShader;
    uint8_t perPrimArgCount;
    uint8_t perObjArgCount;
    uint8_t stableArgCount;
    uint8_t customSamplerFlags;
    uint32_t args;
};
struct MaterialShaderArgument_32
{
    uint16_t type;
    uint16_t dest;
    uint32_t u;
};
struct MaterialVertexShader_32
{
    uint32_t name;
    uint32_t vs;
    uint32_t program;
    uint16_t programSize;
    uint16_t pad;
};
struct MaterialPixelShader_32
{
    uint32_t name;
    uint32_t ps;
    uint32_t program;
    uint16_t programSize;
    uint16_t pad;
};
struct MaterialVertexDeclaration_32
{
    uint8_t streamCount;
    uint8_t hasOptionalSource;
    uint8_t isLoaded;
    uint8_t pad;
    uint8_t routingData[32];
    uint32_t decl[18];
};
struct water_t_32
{
    float floatTime;
    uint32_t H0;
    uint32_t wTerm;
    int M;
    int N;
    float Lx;
    float Lz;
    float gravity;
    float windvel;
    float winddir[2];
    float amplitude;
    float codeConstant[4];
    uint32_t image;
};
#pragma pack(pop)

static_assert(sizeof(MaterialInfo_32) == 40, "Win32 MaterialInfo");
static_assert(sizeof(Material_32) == 192, "Win32 Material");
static_assert(sizeof(MaterialTextureDef_32) == 16, "Win32 MaterialTextureDef");
static_assert(sizeof(MaterialTechniqueHeader_32) == 8, "Win32 MaterialTechnique header");
static_assert(sizeof(MaterialPass_32) == 20, "Win32 MaterialPass");
static_assert(sizeof(MaterialShaderArgument_32) == 8, "Win32 MaterialShaderArgument");
static_assert(sizeof(MaterialVertexShader_32) == 16, "Win32 MaterialVertexShader");
static_assert(sizeof(MaterialPixelShader_32) == 16, "Win32 MaterialPixelShader");
static_assert(sizeof(MaterialVertexDeclaration_32) == 108, "Win32 MaterialVertexDeclaration");
static_assert(sizeof(water_t_32) == 68, "Win32 water_t");

static void DB_Expand_MaterialInfo(MaterialInfo *dst, const MaterialInfo_32 &disk)
{
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    dst->gameFlags = disk.gameFlags;
    dst->pad = disk.pad;
    dst->sortKey = disk.sortKey;
    dst->textureAtlasRowCount = disk.textureAtlasRowCount;
    dst->textureAtlasColumnCount = disk.textureAtlasColumnCount;
    dst->drawSurf.packed = disk.drawSurf;
    dst->surfaceTypeBits = disk.surfaceTypeBits;
    dst->layeredSurfaceTypes = disk.layeredSurfaceTypes;
    dst->hashIndex = disk.hashIndex;
}

void DB_ExpandLoad_Material(Material *dst)
{
    iassert(dst == reinterpret_cast<Material *>(DB_GetStreamPos()));

    Material_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));

    DB_Expand_MaterialInfo(&dst->info, disk.info);
    memcpy(dst->stateBitsEntry, disk.stateBitsEntry, sizeof(dst->stateBitsEntry));
    dst->textureCount = disk.textureCount;
    dst->constantCount = disk.constantCount;
    dst->stateBitsCount = disk.stateBitsCount;
    dst->stateFlags = disk.stateFlags;
    dst->cameraRegion = disk.cameraRegion;
    dst->maxStreamedMips = disk.maxStreamedMips;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->localTechniqueSet), disk.techniqueSet);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->textureTable), disk.textureTable);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->localConstantTable), disk.constantTable);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->stateBitsTable), disk.stateBitsTable);

    DB_Expand_Commit(static_cast<int>(sizeof(Material)));
}

void DB_ExpandLoad_MaterialTextureDefArray(MaterialTextureDef *dst, int count)
{
    iassert(dst == reinterpret_cast<MaterialTextureDef *>(DB_GetStreamPos()));
    iassert(count >= 0);

    count = DB_Expand_ClampCount(count, "MaterialTextureDef_32");
    std::vector<MaterialTextureDef_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(MaterialTextureDef_32) * count));

    memset(dst, 0, sizeof(MaterialTextureDef) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        const MaterialTextureDef_32 &s = disk[static_cast<size_t>(i)];
        dst[i].nameHash = s.nameHash;
        dst[i].nameStart = s.nameStart;
        dst[i].nameEnd = s.nameEnd;
        dst[i].samplerState = s.samplerState;
        dst[i].semantic = s.semantic;
        dst[i].isMatureContent = s.isMatureContent;
        memcpy(dst[i].pad, s.pad, sizeof(dst[i].pad));
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].u), s.u);
    }
    DB_Expand_Commit(static_cast<int>(sizeof(MaterialTextureDef) * static_cast<size_t>(count)), count);
}

#endif
#if !defined(BO1_ASSETLIST_GATE) || defined(BO1_TECHSET_GATE)
#pragma pack(push, 1)
struct MaterialTechniqueSet_32
{
    uint32_t name;
    uint8_t worldVertFormat;
    uint8_t unused;
    uint16_t techsetFlags;
    uint32_t techniques[130];
};
#pragma pack(pop)
static_assert(sizeof(MaterialTechniqueSet_32) == 528, "Win32 MaterialTechniqueSet");

void DB_ExpandLoad_MaterialTechniqueSet(MaterialTechniqueSet *dst)
{
    iassert(dst == reinterpret_cast<MaterialTechniqueSet *>(DB_GetStreamPos()));

    MaterialTechniqueSet_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));

    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    dst->worldVertFormat = disk.worldVertFormat;
    dst->unused[0] = disk.unused;
    dst->techsetFlags = disk.techsetFlags;
    for (int i = 0; i < 130; ++i)
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->techniques[i]), disk.techniques[i]);

    DB_Expand_Commit(static_cast<int>(sizeof(MaterialTechniqueSet)));
}

#endif
#if !defined(BO1_ASSETLIST_GATE)
void DB_ExpandLoad_MaterialTechnique(MaterialTechnique *dst)
{
    iassert(dst == reinterpret_cast<MaterialTechnique *>(DB_GetStreamPos()));

    MaterialTechniqueHeader_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, offsetof(MaterialTechnique, passArray));

    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    dst->flags = disk.flags;
    dst->passCount = disk.passCount;

    DB_Expand_Commit(static_cast<int>(offsetof(MaterialTechnique, passArray)));
}

void DB_ExpandLoad_MaterialPassArray(MaterialPass *dst, int count)
{
    iassert(dst == reinterpret_cast<MaterialPass *>(DB_GetStreamPos()));
    iassert(count >= 0);

    count = DB_Expand_ClampCount(count, "MaterialPass_32");
    std::vector<MaterialPass_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(MaterialPass_32) * count));

    memset(dst, 0, sizeof(MaterialPass) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        const MaterialPass_32 &s = disk[static_cast<size_t>(i)];
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].vertexDecl), s.vertexDecl);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].vertexShader), s.vertexShader);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].pixelShader), s.pixelShader);
        dst[i].perPrimArgCount = s.perPrimArgCount;
        dst[i].perObjArgCount = s.perObjArgCount;
        dst[i].stableArgCount = s.stableArgCount;
        dst[i].customSamplerFlags = s.customSamplerFlags;
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].args), s.args);
    }
    DB_Expand_Commit(static_cast<int>(sizeof(MaterialPass) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_MaterialShaderArgumentArray(MaterialShaderArgument *dst, int count)
{
    iassert(dst == reinterpret_cast<MaterialShaderArgument *>(DB_GetStreamPos()));
    iassert(count >= 0);

    count = DB_Expand_ClampCount(count, "MaterialShaderArgument_32");
    std::vector<MaterialShaderArgument_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(MaterialShaderArgument_32) * count));

    memset(dst, 0, sizeof(MaterialShaderArgument) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        dst[i].type = disk[static_cast<size_t>(i)].type;
        dst[i].dest = disk[static_cast<size_t>(i)].dest;
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].u), disk[static_cast<size_t>(i)].u);
    }
    DB_Expand_Commit(static_cast<int>(sizeof(MaterialShaderArgument) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_MaterialVertexShader(MaterialVertexShader *dst)
{
    iassert(dst == reinterpret_cast<MaterialVertexShader *>(DB_GetStreamPos()));

    MaterialVertexShader_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));

    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->prog.vs), disk.vs);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->prog.loadDef.program), disk.program);
    dst->prog.loadDef.programSize = disk.programSize;

    DB_Expand_Commit(static_cast<int>(sizeof(MaterialVertexShader)));
}

void DB_ExpandLoad_MaterialPixelShader(MaterialPixelShader *dst)
{
    iassert(dst == reinterpret_cast<MaterialPixelShader *>(DB_GetStreamPos()));

    MaterialPixelShader_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));

    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->prog.ps), disk.ps);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->prog.loadDef.program), disk.program);
    dst->prog.loadDef.programSize = disk.programSize;

    DB_Expand_Commit(static_cast<int>(sizeof(MaterialPixelShader)));
}

void DB_ExpandLoad_MaterialVertexDeclaration(MaterialVertexDeclaration *dst)
{
    iassert(reinterpret_cast<unsigned __int8 *>(dst) == DB_GetStreamPos());

    MaterialVertexDeclaration_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));

    dst->streamCount = disk.streamCount;
    dst->hasOptionalSource = disk.hasOptionalSource != 0;
    dst->isLoaded = disk.isLoaded != 0;
    memcpy(dst->routing.data, disk.routingData, sizeof(disk.routingData));
    for (int i = 0; i < 18; ++i)
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->routing.decl[i]), disk.decl[i]);

    DB_Expand_Commit(static_cast<int>(sizeof(MaterialVertexDeclaration)));
}

void DB_ExpandLoad_water_t(water_t *dst)
{
    iassert(dst == reinterpret_cast<water_t *>(DB_GetStreamPos()));

    water_t_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));

    dst->writable.floatTime = disk.floatTime;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->H0), disk.H0);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->wTerm), disk.wTerm);
    dst->M = disk.M;
    dst->N = disk.N;
    dst->Lx = disk.Lx;
    dst->Lz = disk.Lz;
    dst->gravity = disk.gravity;
    dst->windvel = disk.windvel;
    dst->winddir[0] = disk.winddir[0];
    dst->winddir[1] = disk.winddir[1];
    dst->amplitude = disk.amplitude;
    memcpy(dst->codeConstant, disk.codeConstant, sizeof(dst->codeConstant));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->image), disk.image);

    DB_Expand_Commit(static_cast<int>(sizeof(water_t)));
}

#pragma pack(push, 1)
struct XModelLodInfo_32
{
    float dist;
    uint16_t numsurfs;
    uint16_t surfIndex;
    int partBits[5];
    uint8_t lod;
    uint8_t smcIndexPlusOne;
    uint8_t smcAllocBits;
    uint8_t unused;
};
struct XModel_32
{
    uint32_t name;
    uint8_t numBones;
    uint8_t numRootBones;
    uint8_t numsurfs;
    uint8_t lodRampType;
    uint32_t boneNames;
    uint32_t parentList;
    uint32_t quats;
    uint32_t trans;
    uint32_t partClassification;
    uint32_t baseMat;
    uint32_t surfs;
    uint32_t materialHandles;
    XModelLodInfo_32 lodInfo[4];
    uint8_t lodDistAutoGenerated;
    uint8_t padLod[3];
    uint32_t collSurfs;
    int numCollSurfs;
    int contents;
    uint32_t boneInfo;
    float radius;
    float mins[3];
    float maxs[3];
    int16_t numLods;
    int16_t collLod;
    uint32_t highMipBounds;
    int memUsage;
    int flags;
    uint8_t bad;
    uint8_t padBad[3];
    uint32_t physPreset;
    uint8_t numCollmaps;
    uint8_t padCollmaps[3];
    uint32_t collmaps;
    uint32_t physConstraints;
};
struct XSurfaceVertexInfo_32
{
    int16_t vertCount[4];
    uint32_t vertsBlend;
    uint32_t tensionData;
};
struct XSurface_32
{
    uint8_t tileMode;
    uint8_t vertListCount;
    uint16_t flags;
    uint16_t vertCount;
    uint16_t triCount;
    uint16_t baseTriIndex;
    uint16_t baseVertIndex;
    uint32_t triIndices;
    XSurfaceVertexInfo_32 vertInfo;
    uint32_t verts0;
    uint32_t vb0;
    uint32_t vertList;
    uint32_t indexBuffer;
    int partBits[5];
};
struct XRigidVertList_32
{
    uint16_t boneOffset;
    uint16_t vertCount;
    uint16_t triOffset;
    uint16_t triCount;
    uint32_t collisionTree;
};
struct XSurfaceCollisionTree_32
{
    float trans[3];
    float scale[3];
    uint32_t nodeCount;
    uint32_t nodes;
    uint32_t leafCount;
    uint32_t leafs;
};
struct XModelCollSurf_32
{
    uint32_t collTris;
    int numCollTris;
    float mins[3];
    float maxs[3];
    int boneIdx;
    int contents;
    int surfFlags;
};
struct BrushWrapper_32
{
    float mins[3];
    int contents;
    float maxs[3];
    uint32_t numsides;
    uint32_t sides;
    int axial_cflags[2][3];
    int axial_sflags[2][3];
    uint32_t numverts;
    uint32_t verts;
    uint32_t planes;
};
struct PhysGeomInfo_32
{
    uint32_t brush;
    int type;
    float orientation[3][3];
    float offset[3];
    float halfLengths[3];
};
struct PhysGeomList_32
{
    uint32_t count;
    uint32_t geoms;
    int contents;
};
struct Collmap_32
{
    uint32_t geomList;
};
struct PhysPreset_32
{
    uint32_t name;
    int flags;
    float mass;
    float bounce;
    float friction;
    float bulletForceScale;
    float explosiveForceScale;
    uint32_t sndAliasPrefix;
    float piecesSpreadFraction;
    float piecesUpwardVelocity;
    int canFloat;
    float gravityScale;
    float centerOfMassOffset[3];
    float buoyancyBoxMin[3];
    float buoyancyBoxMax[3];
};
struct PhysConstraint_32
{
    uint16_t targetname;
    uint16_t pad0;
    int type;
    int attach_point_type1;
    int target_index1;
    uint16_t target_ent1;
    uint16_t pad1;
    uint32_t target_bone1;
    int attach_point_type2;
    int target_index2;
    uint16_t target_ent2;
    uint16_t pad2;
    uint32_t target_bone2;
    float offset[3];
    float pos[3];
    float pos2[3];
    float dir[3];
    int flags;
    int timeout;
    int min_health;
    int max_health;
    float distance;
    float damp;
    float power;
    float scale[3];
    float spin_scale;
    float minAngle;
    float maxAngle;
    uint32_t material;
    int constraintHandle;
    int rope_index;
    int centity_num[4];
};
struct PhysConstraints_32
{
    uint32_t name;
    uint32_t count;
    PhysConstraint_32 data[16];
};
struct cbrushside_t_32
{
    uint32_t plane;
    int cflags;
    int sflags;
};
#pragma pack(pop)

static_assert(sizeof(XModelLodInfo_32) == 32, "Win32 XModelLodInfo");
static_assert(sizeof(XModel_32) == 252, "Win32 XModel");
static_assert(sizeof(XSurfaceVertexInfo_32) == 16, "Win32 XSurfaceVertexInfo");
static_assert(sizeof(XSurface_32) == 68, "Win32 XSurface");
static_assert(sizeof(XRigidVertList_32) == 12, "Win32 XRigidVertList");
static_assert(sizeof(XSurfaceCollisionTree_32) == 40, "Win32 XSurfaceCollisionTree");
static_assert(sizeof(XModelCollSurf_32) == 44, "Win32 XModelCollSurf");
static_assert(sizeof(BrushWrapper_32) == 96, "Win32 BrushWrapper");
static_assert(sizeof(PhysGeomInfo_32) == 68, "Win32 PhysGeomInfo");
static_assert(sizeof(PhysGeomList_32) == 12, "Win32 PhysGeomList");
static_assert(sizeof(Collmap_32) == 4, "Win32 Collmap");
static_assert(sizeof(PhysPreset_32) == 84, "Win32 PhysPreset");
static_assert(sizeof(PhysConstraint_32) == 168, "Win32 PhysConstraint");
static_assert(sizeof(PhysConstraints_32) == 2696, "Win32 PhysConstraints");
static_assert(sizeof(cbrushside_t_32) == 12, "Win32 cbrushside_t");

static void DB_Expand_PhysConstraint(PhysConstraint *dst, const PhysConstraint_32 &s)
{
    memset(dst, 0, sizeof(*dst));
    dst->targetname = s.targetname;
    dst->type = static_cast<ConstraintType>(s.type);
    dst->attach_point_type1 = static_cast<AttachPointType>(s.attach_point_type1);
    dst->target_index1 = s.target_index1;
    dst->target_ent1 = s.target_ent1;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->target_bone1), s.target_bone1);
    dst->attach_point_type2 = static_cast<AttachPointType>(s.attach_point_type2);
    dst->target_index2 = s.target_index2;
    dst->target_ent2 = s.target_ent2;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->target_bone2), s.target_bone2);
    memcpy(dst->offset, s.offset, sizeof(dst->offset));
    memcpy(dst->pos, s.pos, sizeof(dst->pos));
    memcpy(dst->pos2, s.pos2, sizeof(dst->pos2));
    memcpy(dst->dir, s.dir, sizeof(dst->dir));
    dst->flags = s.flags;
    dst->timeout = s.timeout;
    dst->min_health = s.min_health;
    dst->max_health = s.max_health;
    dst->distance = s.distance;
    dst->damp = s.damp;
    dst->power = s.power;
    memcpy(dst->scale, s.scale, sizeof(dst->scale));
    dst->spin_scale = s.spin_scale;
    dst->minAngle = s.minAngle;
    dst->maxAngle = s.maxAngle;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->material), s.material);
    dst->constraintHandle = s.constraintHandle;
    dst->rope_index = s.rope_index;
    memcpy(dst->centity_num, s.centity_num, sizeof(dst->centity_num));
}

void DB_ExpandLoad_XModel(XModel *dst)
{
    iassert(dst == reinterpret_cast<XModel *>(DB_GetStreamPos()));
    {
        static int once;
        if (!once++)
        {
        }
    }

    XModel_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));

    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    dst->numBones = disk.numBones;
    dst->numRootBones = disk.numRootBones;
    dst->numsurfs = disk.numsurfs;
    dst->lodRampType = disk.lodRampType;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->localBoneNames), disk.boneNames);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->localParentList), disk.parentList);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->localQuats), disk.quats);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->localTrans), disk.trans);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->partClassification), disk.partClassification);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->baseMat), disk.baseMat);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->surfs), disk.surfs);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->materialHandles), disk.materialHandles);
    for (int i = 0; i < 4; ++i)
    {
        dst->lodInfo[i].dist = disk.lodInfo[i].dist;
        dst->lodInfo[i].numsurfs = disk.lodInfo[i].numsurfs;
        dst->lodInfo[i].surfIndex = disk.lodInfo[i].surfIndex;
        memcpy(dst->lodInfo[i].partBits, disk.lodInfo[i].partBits, sizeof(dst->lodInfo[i].partBits));
        dst->lodInfo[i].lod = disk.lodInfo[i].lod;
        dst->lodInfo[i].smcIndexPlusOne = disk.lodInfo[i].smcIndexPlusOne;
        dst->lodInfo[i].smcAllocBits = disk.lodInfo[i].smcAllocBits;
        dst->lodInfo[i].unused = disk.lodInfo[i].unused;
    }
    dst->lodDistAutoGenerated = disk.lodDistAutoGenerated;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->collSurfs), disk.collSurfs);
    dst->numCollSurfs = disk.numCollSurfs;
    dst->contents = disk.contents;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->localBoneInfo), disk.boneInfo);
    dst->radius = disk.radius;
    memcpy(dst->mins, disk.mins, sizeof(dst->mins));
    memcpy(dst->maxs, disk.maxs, sizeof(dst->maxs));
    dst->numLods = disk.numLods;
    dst->collLod = disk.collLod;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->streamInfo.highMipBounds), disk.highMipBounds);
    dst->memUsage = disk.memUsage;
    dst->flags = disk.flags;
    dst->bad = disk.bad != 0;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->physPreset), disk.physPreset);
    dst->numCollmaps = disk.numCollmaps;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->collmaps), disk.collmaps);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->physConstraints), disk.physConstraints);

    DB_Expand_Commit(static_cast<int>(sizeof(XModel)));
}

void DB_ExpandLoad_XSurfaceArray(XSurface *dst, int count)
{
    iassert(reinterpret_cast<unsigned __int8 *>(dst) == DB_GetStreamPos());
    iassert(count >= 0);

    count = DB_Expand_ClampCount(count, "XSurface_32");
    std::vector<XSurface_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(XSurface_32) * count));

    memset(dst, 0, sizeof(XSurface) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        const XSurface_32 &s = disk[static_cast<size_t>(i)];
        dst[i].tileMode = s.tileMode;
        dst[i].vertListCount = s.vertListCount;
        dst[i].flags = s.flags;
        dst[i].vertCount = s.vertCount;
        dst[i].triCount = s.triCount;
        dst[i].baseTriIndex = s.baseTriIndex;
        dst[i].baseVertIndex = s.baseVertIndex;
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].triIndices), s.triIndices);
        memcpy(dst[i].vertInfo.vertCount, s.vertInfo.vertCount, sizeof(dst[i].vertInfo.vertCount));
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].vertInfo.vertsBlend), s.vertInfo.vertsBlend);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].vertInfo.tensionData), s.vertInfo.tensionData);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].verts0), s.verts0);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].vb0), s.vb0);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].vertList), s.vertList);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].indexBuffer), s.indexBuffer);
        memcpy(dst[i].partBits, s.partBits, sizeof(dst[i].partBits));
    }
    DB_Expand_Commit(static_cast<int>(sizeof(XSurface) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_XRigidVertListArray(XRigidVertList *dst, int count)
{
    iassert(dst == reinterpret_cast<XRigidVertList *>(DB_GetStreamPos()));
    iassert(count >= 0);

    count = DB_Expand_ClampCount(count, "XRigidVertList_32");
    std::vector<XRigidVertList_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(XRigidVertList_32) * count));

    memset(dst, 0, sizeof(XRigidVertList) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        dst[i].boneOffset = disk[static_cast<size_t>(i)].boneOffset;
        dst[i].vertCount = disk[static_cast<size_t>(i)].vertCount;
        dst[i].triOffset = disk[static_cast<size_t>(i)].triOffset;
        dst[i].triCount = disk[static_cast<size_t>(i)].triCount;
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].collisionTree), disk[static_cast<size_t>(i)].collisionTree);
    }
    DB_Expand_Commit(static_cast<int>(sizeof(XRigidVertList) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_XSurfaceCollisionTree(XSurfaceCollisionTree *dst)
{
    iassert(dst == reinterpret_cast<XSurfaceCollisionTree *>(DB_GetStreamPos()));

    XSurfaceCollisionTree_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));

    memcpy(dst->trans, disk.trans, sizeof(dst->trans));
    memcpy(dst->scale, disk.scale, sizeof(dst->scale));
    dst->nodeCount = disk.nodeCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->nodes), disk.nodes);
    dst->leafCount = disk.leafCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->leafs), disk.leafs);

    DB_Expand_Commit(static_cast<int>(sizeof(XSurfaceCollisionTree)));
}

void DB_ExpandLoad_XModelCollSurfArray(XModelCollSurf_s *dst, int count)
{
    iassert(dst == reinterpret_cast<XModelCollSurf_s *>(DB_GetStreamPos()));
    iassert(count >= 0);

    count = DB_Expand_ClampCount(count, "XModelCollSurf_32");
    std::vector<XModelCollSurf_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(XModelCollSurf_32) * count));

    memset(dst, 0, sizeof(XModelCollSurf_s) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        const XModelCollSurf_32 &s = disk[static_cast<size_t>(i)];
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].collTris), s.collTris);
        dst[i].numCollTris = s.numCollTris;
        memcpy(dst[i].mins, s.mins, sizeof(dst[i].mins));
        memcpy(dst[i].maxs, s.maxs, sizeof(dst[i].maxs));
        dst[i].boneIdx = s.boneIdx;
        dst[i].contents = s.contents;
        dst[i].surfFlags = s.surfFlags;
    }
    DB_Expand_Commit(static_cast<int>(sizeof(XModelCollSurf_s) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_CollmapArray(Collmap *dst, int count)
{
    iassert(dst == reinterpret_cast<Collmap *>(DB_GetStreamPos()));
    iassert(count >= 0);

    count = DB_Expand_ClampCount(count, "Collmap_32");
    std::vector<Collmap_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(Collmap_32) * count));

    memset(dst, 0, sizeof(Collmap) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].geomList), disk[static_cast<size_t>(i)].geomList);
    DB_Expand_Commit(static_cast<int>(sizeof(Collmap) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_PhysGeomList(PhysGeomList *dst)
{
    iassert(dst == reinterpret_cast<PhysGeomList *>(DB_GetStreamPos()));

    PhysGeomList_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    dst->count = disk.count;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->geoms), disk.geoms);
    dst->contents = disk.contents;
    DB_Expand_Commit(static_cast<int>(sizeof(PhysGeomList)));
}

void DB_ExpandLoad_PhysGeomInfoArray(PhysGeomInfo *dst, int count)
{
    iassert(dst == reinterpret_cast<PhysGeomInfo *>(DB_GetStreamPos()));
    iassert(count >= 0);

    count = DB_Expand_ClampCount(count, "PhysGeomInfo_32");
    std::vector<PhysGeomInfo_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(PhysGeomInfo_32) * count));

    memset(dst, 0, sizeof(PhysGeomInfo) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        const PhysGeomInfo_32 &s = disk[static_cast<size_t>(i)];
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].brush), s.brush);
        dst[i].type = s.type;
        memcpy(dst[i].orientation, s.orientation, sizeof(dst[i].orientation));
        memcpy(dst[i].offset, s.offset, sizeof(dst[i].offset));
        memcpy(dst[i].halfLengths, s.halfLengths, sizeof(dst[i].halfLengths));
    }
    DB_Expand_Commit(static_cast<int>(sizeof(PhysGeomInfo) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_BrushWrapper(BrushWrapper *dst)
{
    iassert(dst == reinterpret_cast<BrushWrapper *>(DB_GetStreamPos()));

    BrushWrapper_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));

    memcpy(dst->mins, disk.mins, sizeof(dst->mins));
    dst->contents = disk.contents;
    memcpy(dst->maxs, disk.maxs, sizeof(dst->maxs));
    dst->numsides = disk.numsides;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->sides), disk.sides);
    memcpy(dst->axial_cflags, disk.axial_cflags, sizeof(dst->axial_cflags));
    memcpy(dst->axial_sflags, disk.axial_sflags, sizeof(dst->axial_sflags));
    dst->numverts = disk.numverts;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->verts), disk.verts);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->planes), disk.planes);

    DB_Expand_Commit(static_cast<int>(sizeof(BrushWrapper)));
}

void DB_ExpandLoad_cbrushside_tArray(cbrushside_t *dst, int count)
{
    iassert(dst == reinterpret_cast<cbrushside_t *>(DB_GetStreamPos()));
    iassert(count >= 0);

    count = DB_Expand_ClampCount(count, "cbrushside_t_32");
    std::vector<cbrushside_t_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(cbrushside_t_32) * count));

    memset(dst, 0, sizeof(cbrushside_t) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].plane), disk[static_cast<size_t>(i)].plane);
        dst[i].cflags = disk[static_cast<size_t>(i)].cflags;
        dst[i].sflags = disk[static_cast<size_t>(i)].sflags;
    }
    DB_Expand_Commit(static_cast<int>(sizeof(cbrushside_t) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_PhysPreset(PhysPreset *dst)
{
    iassert(dst == reinterpret_cast<PhysPreset *>(DB_GetStreamPos()));

    PhysPreset_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));

    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    dst->flags = disk.flags;
    dst->mass = disk.mass;
    dst->bounce = disk.bounce;
    dst->friction = disk.friction;
    dst->bulletForceScale = disk.bulletForceScale;
    dst->explosiveForceScale = disk.explosiveForceScale;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->sndAliasPrefix), disk.sndAliasPrefix);
    dst->piecesSpreadFraction = disk.piecesSpreadFraction;
    dst->piecesUpwardVelocity = disk.piecesUpwardVelocity;
    dst->canFloat = disk.canFloat;
    dst->gravityScale = disk.gravityScale;
    memcpy(dst->centerOfMassOffset, disk.centerOfMassOffset, sizeof(dst->centerOfMassOffset));
    memcpy(dst->buoyancyBoxMin, disk.buoyancyBoxMin, sizeof(dst->buoyancyBoxMin));
    memcpy(dst->buoyancyBoxMax, disk.buoyancyBoxMax, sizeof(dst->buoyancyBoxMax));

    DB_Expand_Commit(static_cast<int>(sizeof(PhysPreset)));
}

void DB_ExpandLoad_PhysConstraintArray(PhysConstraint *dst, int count)
{
    iassert(dst == reinterpret_cast<PhysConstraint *>(DB_GetStreamPos()));
    iassert(count >= 0);

    count = DB_Expand_ClampCount(count, "PhysConstraint_32");
    std::vector<PhysConstraint_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(PhysConstraint_32) * count));

    memset(dst, 0, sizeof(PhysConstraint) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
        DB_Expand_PhysConstraint(&dst[i], disk[static_cast<size_t>(i)]);
    DB_Expand_Commit(static_cast<int>(sizeof(PhysConstraint) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_PhysConstraints(PhysConstraints *dst)
{
    iassert(dst == reinterpret_cast<PhysConstraints *>(DB_GetStreamPos()));

    PhysConstraints_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));

    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    dst->count = disk.count;
    for (int i = 0; i < 16; ++i)
        DB_Expand_PhysConstraint(&dst->data[i], disk.data[i]);

    DB_Expand_Commit(static_cast<int>(sizeof(PhysConstraints)));
}

#pragma pack(push, 1)
struct cStaticModel_32
{
    uint16_t nextModelInWorldSector;
    uint16_t pad;
    uint32_t xmodel;
    float origin[3];
    float invScaledAxis[3][3];
    float absmin[3];
    float absmax[3];
};
struct cNode_32
{
    uint32_t plane;
    int16_t children[2];
};
struct cLeafBrushNode_32
{
    uint8_t axis;
    uint8_t pad;
    int16_t leafBrushCount;
    int contents;
    uint8_t data[12]; // leaf.brushes ptr32 or children floats
};
struct CollisionPartition_32
{
    uint8_t triCount;
    uint8_t borderCount;
    uint8_t pad[2];
    int firstTri;
    int nuinds;
    int fuind;
    uint32_t borders;
};
struct cbrush_32
{
    float mins[3];
    int contents;
    float maxs[3];
    uint32_t numsides;
    uint32_t sides;
    int axial_cflags[2][3];
    int axial_sflags[2][3];
    uint32_t numverts;
    uint32_t verts;
    uint32_t padAlign8;
};
struct cmodel_32
{
    float mins[3];
    float maxs[3];
    float radius;
    uint8_t leaf[44];
};
struct MapEnts_32
{
    uint32_t name;
    uint32_t entityString;
    int numEntityChars;
};
struct DynEntityDef_32
{
    int type;
    float pose_quat[4];
    float pose_origin[3];
    uint32_t xModel;
    uint32_t destroyedxModel;
    uint16_t brushModel;
    uint16_t physicsBrushModel;
    uint32_t destroyFx;
    uint32_t destroySound;
    uint32_t destroyPieces;
    uint32_t physPreset;
    int16_t physConstraints[4];
    int health;
    int flags;
    int contents;
    uint16_t targetname;
    uint16_t target;
};
struct clipMap_32
{
    uint32_t name;
    int isInUse;
    int planeCount;
    uint32_t planes;
    uint32_t numStaticModels;
    uint32_t staticModelList;
    uint32_t numMaterials;
    uint32_t materials;
    uint32_t numBrushSides;
    uint32_t brushsides;
    uint32_t numNodes;
    uint32_t nodes;
    uint32_t numLeafs;
    uint32_t leafs;
    uint32_t leafbrushNodesCount;
    uint32_t leafbrushNodes;
    uint32_t numLeafBrushes;
    uint32_t leafbrushes;
    uint32_t numLeafSurfaces;
    uint32_t leafsurfaces;
    uint32_t vertCount;
    uint32_t verts;
    uint32_t numBrushVerts;
    uint32_t brushVerts;
    uint32_t nuinds;
    uint32_t uinds;
    int triCount;
    uint32_t triIndices;
    uint32_t triEdgeIsWalkable;
    int borderCount;
    uint32_t borders;
    int partitionCount;
    uint32_t partitions;
    int aabbTreeCount;
    uint32_t aabbTrees;
    uint32_t numSubModels;
    uint32_t cmodels;
    uint16_t numBrushes;
    uint16_t padBrushes;
    uint32_t brushes;
    int numClusters;
    int clusterBytes;
    uint32_t visibility;
    int vised;
    uint32_t mapEnts;
    uint32_t box_brush;
    cmodel_32 box_model;
    uint16_t originalDynEntCount;
    uint16_t dynEntCount[4];
    uint16_t padDyn;
    uint32_t dynEntDefList[2];
    uint32_t dynEntPoseList[2];
    uint32_t dynEntClientList[2];
    uint32_t dynEntServerList[2];
    uint32_t dynEntCollList[4];
    int num_constraints;
    uint32_t constraints;
    int max_ropes;
    uint32_t ropes;
    uint32_t checksum;
};
#pragma pack(pop)

static_assert(sizeof(cStaticModel_32) == 80, "Win32 cStaticModel");
static_assert(sizeof(cNode_32) == 8, "Win32 cNode");
static_assert(sizeof(cLeafBrushNode_32) == 20, "Win32 cLeafBrushNode");
static_assert(sizeof(CollisionPartition_32) == 20, "Win32 CollisionPartition");
static_assert(sizeof(cbrush_32) == 96, "Win32 cbrush_t");
static_assert(sizeof(cmodel_32) == 72, "Win32 cmodel_t");
static_assert(sizeof(MapEnts_32) == 12, "Win32 MapEnts");
static_assert(sizeof(DynEntityDef_32) == 84, "Win32 DynEntityDef");
static_assert(sizeof(clipMap_32) == 332, "Win32 clipMap_t");

// Win32 rope_t: only Material* widens; expand via full blob remap.
static constexpr int kRopeWin32Size = 3188;
static constexpr int kRopeMaterialOff = 3144; // 0xC74 - 4(material) - 40(trailing)
static constexpr int kRopeTrailing = 40;

void DB_ExpandLoad_cStaticModel_tArray(cStaticModel_s *dst, int count)
{
    iassert(dst == reinterpret_cast<cStaticModel_s *>(DB_GetStreamPos()));
    iassert(count >= 0);

    count = DB_Expand_ClampCount(count, "cStaticModel_32");
    std::vector<cStaticModel_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(cStaticModel_32) * count));

    memset(dst, 0, sizeof(cStaticModel_s) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        const cStaticModel_32 &s = disk[static_cast<size_t>(i)];
        dst[i].writable.nextModelInWorldSector = s.nextModelInWorldSector;
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].xmodel), s.xmodel);
        memcpy(dst[i].origin, s.origin, sizeof(dst[i].origin));
        memcpy(dst[i].invScaledAxis, s.invScaledAxis, sizeof(dst[i].invScaledAxis));
        memcpy(dst[i].absmin, s.absmin, sizeof(dst[i].absmin));
        memcpy(dst[i].absmax, s.absmax, sizeof(dst[i].absmax));
    }
    DB_Expand_Commit(static_cast<int>(sizeof(cStaticModel_s) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_cNode_tArray(cNode_t *dst, int count)
{
    iassert(dst == reinterpret_cast<cNode_t *>(DB_GetStreamPos()));
    iassert(count >= 0);

    count = DB_Expand_ClampCount(count, "cNode_32");
    std::vector<cNode_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(cNode_32) * count));

    memset(dst, 0, sizeof(cNode_t) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].plane), disk[static_cast<size_t>(i)].plane);
        dst[i].children[0] = disk[static_cast<size_t>(i)].children[0];
        dst[i].children[1] = disk[static_cast<size_t>(i)].children[1];
    }
    DB_Expand_Commit(static_cast<int>(sizeof(cNode_t) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_cLeafBrushNode_tArray(cLeafBrushNode_s *dst, int count)
{
    iassert(reinterpret_cast<unsigned __int8 *>(dst) == DB_GetStreamPos());
    iassert(count >= 0);

    count = DB_Expand_ClampCount(count, "cLeafBrushNode_32");
    std::vector<cLeafBrushNode_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(cLeafBrushNode_32) * count));

    memset(dst, 0, sizeof(cLeafBrushNode_s) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        const cLeafBrushNode_32 &s = disk[static_cast<size_t>(i)];
        dst[i].axis = s.axis;
        dst[i].leafBrushCount = s.leafBrushCount;
        dst[i].contents = s.contents;
        if (s.leafBrushCount > 0)
        {
            uint32_t brushes32 = 0;
            memcpy(&brushes32, s.data, 4);
            DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].data.leaf.brushes), brushes32);
        }
        else
        {
            memcpy(&dst[i].data.children, s.data, sizeof(cLeafBrushNodeChildren_t));
        }
    }
    DB_Expand_Commit(static_cast<int>(sizeof(cLeafBrushNode_s) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_CollisionPartitionArray(CollisionPartition *dst, int count)
{
    iassert(reinterpret_cast<unsigned __int8 *>(dst) == DB_GetStreamPos());
    iassert(count >= 0);

    count = DB_Expand_ClampCount(count, "CollisionPartition_32");
    std::vector<CollisionPartition_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(CollisionPartition_32) * count));

    auto *outBase = reinterpret_cast<unsigned __int8 *>(dst);
    memset(outBase, 0, sizeof(CollisionPartition) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        const CollisionPartition_32 &s = disk[static_cast<size_t>(i)];
        auto *out = reinterpret_cast<CollisionPartition *>(outBase + i * sizeof(CollisionPartition));
        const_cast<unsigned __int8 &>(out->triCount) = s.triCount;
        const_cast<unsigned __int8 &>(out->borderCount) = s.borderCount;
        const_cast<int &>(out->firstTri) = s.firstTri;
        const_cast<int &>(out->nuinds) = s.nuinds;
        const_cast<int &>(out->fuind) = s.fuind;
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(const_cast<CollisionBorder **>(&out->borders)), s.borders);
    }
    DB_Expand_Commit(static_cast<int>(sizeof(CollisionPartition) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_cbrush_tArray(cbrush_t *dst, int count)
{
    iassert(dst == reinterpret_cast<cbrush_t *>(DB_GetStreamPos()));
    iassert(count >= 0);

    count = DB_Expand_ClampCount(count, "cbrush_32");
    std::vector<cbrush_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(cbrush_32) * count));

    memset(const_cast<cbrush_t *>(dst), 0, sizeof(cbrush_t) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        const cbrush_32 &s = disk[static_cast<size_t>(i)];
        auto *out = const_cast<cbrush_t *>(&dst[i]);
        memcpy(out->mins, s.mins, sizeof(out->mins));
        out->contents = s.contents;
        memcpy(out->maxs, s.maxs, sizeof(out->maxs));
        out->numsides = s.numsides;
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&out->sides), s.sides);
        memcpy(out->axial_cflags, s.axial_cflags, sizeof(out->axial_cflags));
        memcpy(out->axial_sflags, s.axial_sflags, sizeof(out->axial_sflags));
        out->numverts = s.numverts;
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&out->verts), s.verts);
    }
    DB_Expand_Commit(static_cast<int>(sizeof(cbrush_t) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_MapEnts(MapEnts *dst)
{
    iassert(dst == reinterpret_cast<MapEnts *>(DB_GetStreamPos()));

    MapEnts_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->entityString), disk.entityString);
    dst->numEntityChars = disk.numEntityChars;
    DB_Expand_Commit(static_cast<int>(sizeof(MapEnts)));
}

void DB_ExpandLoad_DynEntityDefArray(DynEntityDef *dst, int count)
{
    iassert(dst == reinterpret_cast<DynEntityDef *>(DB_GetStreamPos()));
    iassert(count >= 0);

    count = DB_Expand_ClampCount(count, "DynEntityDef_32");
    std::vector<DynEntityDef_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(DynEntityDef_32) * count));

    memset(const_cast<DynEntityDef *>(dst), 0, sizeof(DynEntityDef) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        const DynEntityDef_32 &s = disk[static_cast<size_t>(i)];
        auto *out = const_cast<DynEntityDef *>(&dst[i]);
        out->type = static_cast<DynEntityType>(s.type);
        memcpy(out->pose.quat, s.pose_quat, sizeof(out->pose.quat));
        memcpy(out->pose.origin, s.pose_origin, sizeof(out->pose.origin));
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&out->xModel), s.xModel);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&out->destroyedxModel), s.destroyedxModel);
        out->brushModel = s.brushModel;
        out->physicsBrushModel = s.physicsBrushModel;
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&out->destroyFx), s.destroyFx);
        out->destroySound = s.destroySound;
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&out->destroyPieces), s.destroyPieces);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&out->physPreset), s.physPreset);
        memcpy(out->physConstraints, s.physConstraints, sizeof(out->physConstraints));
        out->health = s.health;
        out->flags = s.flags;
        out->contents = s.contents;
        out->targetname = s.targetname;
        out->target = s.target;
    }
    DB_Expand_Commit(static_cast<int>(sizeof(DynEntityDef) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_rope_tArray(rope_t *dst, int count)
{
    iassert(dst == reinterpret_cast<rope_t *>(DB_GetStreamPos()));
    iassert(count >= 0);

    count = DB_Expand_ClampCount(count, "rope");
    std::vector<uint8_t> disk(static_cast<size_t>(count) * kRopeWin32Size);
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(disk.size()));

    memset(dst, 0, sizeof(rope_t) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        const uint8_t *src = disk.data() + static_cast<size_t>(i) * kRopeWin32Size;
        // Prefix through m_flags is pointer-free and layout-identical.
        memcpy(&dst[i], src, static_cast<size_t>(kRopeMaterialOff));
        uint32_t mat32 = 0;
        memcpy(&mat32, src + kRopeMaterialOff, 4);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(const_cast<Material **>(&dst[i].m_material)), mat32);
        memcpy(&dst[i].m_seglen, src + kRopeMaterialOff + 4, static_cast<size_t>(kRopeTrailing));
    }
    DB_Expand_Commit(static_cast<int>(sizeof(rope_t) * static_cast<size_t>(count)), count);
}

static void DB_Expand_cmodel(cmodel_t *dst, const cmodel_32 &s)
{
    memcpy(dst->mins, s.mins, sizeof(dst->mins));
    memcpy(dst->maxs, s.maxs, sizeof(dst->maxs));
    dst->radius = s.radius;
    memcpy(&dst->leaf, s.leaf, sizeof(dst->leaf));
}

void DB_ExpandLoad_clipMap_t(clipMap_t *dst)
{
    iassert(dst == reinterpret_cast<clipMap_t *>(DB_GetStreamPos()));

    clipMap_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));

    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    dst->isInUse = disk.isInUse;
    dst->planeCount = disk.planeCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->planes), disk.planes);
    dst->numStaticModels = disk.numStaticModels;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->staticModelList), disk.staticModelList);
    dst->numMaterials = disk.numMaterials;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->materials), disk.materials);
    dst->numBrushSides = disk.numBrushSides;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->brushsides), disk.brushsides);
    dst->numNodes = disk.numNodes;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->nodes), disk.nodes);
    dst->numLeafs = disk.numLeafs;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->leafs), disk.leafs);
    dst->leafbrushNodesCount = disk.leafbrushNodesCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->leafbrushNodes), disk.leafbrushNodes);
    dst->numLeafBrushes = disk.numLeafBrushes;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->leafbrushes), disk.leafbrushes);
    dst->numLeafSurfaces = disk.numLeafSurfaces;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->leafsurfaces), disk.leafsurfaces);
    dst->vertCount = disk.vertCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->verts), disk.verts);
    dst->numBrushVerts = disk.numBrushVerts;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->brushVerts), disk.brushVerts);
    dst->nuinds = disk.nuinds;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->uinds), disk.uinds);
    dst->triCount = disk.triCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->triIndices), disk.triIndices);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->triEdgeIsWalkable), disk.triEdgeIsWalkable);
    dst->borderCount = disk.borderCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->borders), disk.borders);
    dst->partitionCount = disk.partitionCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->partitions), disk.partitions);
    dst->aabbTreeCount = disk.aabbTreeCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->aabbTrees), disk.aabbTrees);
    dst->numSubModels = disk.numSubModels;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->cmodels), disk.cmodels);
    dst->numBrushes = disk.numBrushes;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->brushes), disk.brushes);
    dst->numClusters = disk.numClusters;
    dst->clusterBytes = disk.clusterBytes;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->visibility), disk.visibility);
    dst->vised = disk.vised;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->mapEnts), disk.mapEnts);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->box_brush), disk.box_brush);
    DB_Expand_cmodel(&dst->box_model, disk.box_model);
    dst->originalDynEntCount = disk.originalDynEntCount;
    memcpy(dst->dynEntCount, disk.dynEntCount, sizeof(dst->dynEntCount));
    for (int i = 0; i < 2; ++i)
    {
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->dynEntDefList[i]), disk.dynEntDefList[i]);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->dynEntPoseList[i]), disk.dynEntPoseList[i]);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->dynEntClientList[i]), disk.dynEntClientList[i]);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->dynEntServerList[i]), disk.dynEntServerList[i]);
    }
    for (int i = 0; i < 4; ++i)
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->dynEntCollList[i]), disk.dynEntCollList[i]);
    dst->num_constraints = disk.num_constraints;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->constraints), disk.constraints);
    dst->max_ropes = disk.max_ropes;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->ropes), disk.ropes);
    dst->checksum = disk.checksum;

    DB_Expand_Commit(static_cast<int>(sizeof(clipMap_t)));
}


#include <gfx_d3d/r_bsp.h>
#include <gfx_d3d/r_gfx.h>
#include <gfx_d3d/r_dpvs.h>
#include <gfx_d3d/r_dpvs_dynmodel.h>
#include <gfx_d3d/r_sky.h>
#include <gfx_d3d/r_reflection_probe.h>
#include <gfx_d3d/rb_light.h>
#include <gfx_d3d/r_primarylights.h>

#pragma pack(push, 4)
struct GfxWorldStreamInfo_32
{
    int aabbTreeCount;
    uint32_t aabbTrees;
    int leafRefCount;
    uint32_t leafRefs;
};
struct GfxWorldVertexData_32
{
    uint32_t vertices;
    uint32_t worldVb;
};
struct GfxWorldVertexLayerData_32
{
    uint32_t data;
    uint32_t layerVb;
};
struct GfxWorldDraw_32
{
    unsigned int reflectionProbeCount;
    uint32_t reflectionProbes;
    uint32_t reflectionProbeTextures;
    int lightmapCount;
    uint32_t lightmaps;
    uint32_t lightmapPrimaryTextures;
    uint32_t lightmapSecondaryTextures;
    uint32_t lightmapSecondaryTexturesB;
    uint32_t terrainScorchImages[31];
    unsigned int vertexCount;
    GfxWorldVertexData_32 vd;
    unsigned int vertexLayerDataSize;
    GfxWorldVertexLayerData_32 vld;
    unsigned int vertexStream2DataSize;
    int indexCount;
    uint32_t indices;
};
struct GfxLightGrid_32
{
    uint8_t hasLightRegions;
    uint8_t pad[3];
    unsigned int sunPrimaryLightIndex;
    uint16_t mins[3];
    uint16_t maxs[3];
    unsigned int rowAxis;
    unsigned int colAxis;
    uint32_t rowDataStart;
    unsigned int rawRowDataSize;
    uint32_t rawRowData;
    unsigned int entryCount;
    uint32_t entries;
    unsigned int colorCount;
    uint32_t colors;
};
struct GfxWorldDpvsPlanes_32
{
    int cellCount;
    uint32_t planes;
    uint32_t nodes;
    uint32_t sceneEntCellBits;
};
struct GfxWorldDpvsStatic_32
{
    unsigned int smodelCount;
    unsigned int dynamicSModelCount;
    unsigned int staticSurfaceCount;
    unsigned int litSurfsBegin;
    unsigned int litSurfsEnd;
    unsigned int decalSurfsBegin;
    unsigned int decalSurfsEnd;
    unsigned int emissiveSurfsBegin;
    unsigned int emissiveSurfsEnd;
    unsigned int smodelVisDataCount;
    unsigned int surfaceVisDataCount;
    uint32_t smodelVisData[3];
    uint32_t surfaceVisData[3];
    uint32_t smodelVisDataCameraSaved;
    uint32_t surfaceVisDataCameraSaved;
    uint32_t lodData;
    uint32_t sortedSurfIndex;
    uint32_t smodelInsts;
    uint32_t surfaces;
    uint32_t cullGroups;
    uint32_t smodelDrawInsts;
    uint32_t surfaceMaterials;
    uint32_t surfaceCastsSunShadow;
    int usageCount;
};
struct GfxWorldDpvsDynamic_32
{
    unsigned int dynEntClientWordCount[2];
    unsigned int dynEntClientCount[2];
    uint32_t dynEntCellBits[2];
    uint32_t dynEntVisData[2][3];
};
struct GfxWaterBuffer_32
{
    unsigned int bufferSize;
    uint32_t buffer;
};
struct sunflare_t_32
{
    uint8_t hasValidData;
    uint8_t pad[3];
    uint32_t spriteMaterial;
    uint32_t flareMaterial;
    float spriteSize;
    float flareMinSize;
    float flareMinDot;
    float flareMaxSize;
    float flareMaxDot;
    float flareMaxAlpha;
    int flareFadeInTime;
    int flareFadeOutTime;
    float blindMinDot;
    float blindMaxDot;
    float blindMaxDarken;
    int blindFadeInTime;
    int blindFadeOutTime;
    float glareMinDot;
    float glareMaxDot;
    float glareMaxLighten;
    int glareFadeInTime;
    int glareFadeOutTime;
    float sunFxPosition[3];
};
struct GfxPortalWritable_32
{
    uint8_t isQueued;
    uint8_t isAncestor;
    uint8_t recursionDepth;
    uint8_t hullPointCount;
    uint32_t hullPoints;
    uint32_t queuedParent;
};
struct GfxPortal_32
{
    GfxPortalWritable_32 writable;
    DpvsPlane plane;
    uint32_t cell;
    uint32_t vertices;
    uint8_t vertexCount;
    uint8_t pad[3];
    float hullAxis[2][3];
};
struct GfxAabbTree_32
{
    float mins[3];
    float maxs[3];
    uint16_t childCount;
    uint16_t surfaceCount;
    uint16_t startSurfIndex;
    uint16_t smodelIndexCount;
    uint32_t smodelIndexes;
    int childrenOffset;
};
struct GfxCell_32
{
    float mins[3];
    float maxs[3];
    int aabbTreeCount;
    uint32_t aabbTree;
    int portalCount;
    uint32_t portals;
    int cullGroupCount;
    uint32_t cullGroups;
    uint8_t reflectionProbeCount;
    uint8_t pad[3];
    uint32_t reflectionProbes;
};
struct GfxSurface_32
{
    srfTriangles_t tris;
    uint32_t material;
    uint8_t lightmapIndex;
    uint8_t reflectionProbeIndex;
    uint8_t primaryLightIndex;
    uint8_t flags;
    float bounds[2][3];
};
struct GfxStaticModelDrawInst_32
{
    float cullDist;
    GfxPackedPlacement placement;
    uint32_t model;
    int flags;
    uint16_t smodelCacheIndex[4];
    uint16_t lightingHandle;
    uint8_t reflectionProbeIndex;
    uint8_t primaryLightIndex;
};
struct GfxLightmapArray_32
{
    uint32_t primary;
    uint32_t secondary;
    uint32_t secondaryB;
};
struct GfxReflectionProbe_32
{
    float origin[3];
    uint32_t reflectionImage;
    uint32_t probeVolumes;
    unsigned int probeVolumeCount;
};
struct MaterialMemory_32
{
    uint32_t material;
    int memory;
};
struct GfxShadowGeometry_32
{
    uint16_t surfaceCount;
    uint16_t smodelCount;
    uint32_t sortedSurfIndex;
    uint32_t smodelIndex;
};
struct GfxLightRegion_32
{
    unsigned int hullCount;
    uint32_t hulls;
};
struct GfxLightRegionHull_32
{
    float kdopMidPoint[9];
    float kdopHalfSize[9];
    unsigned int axisCount;
    uint32_t axis;
};
struct GfxWorld_32
{
    uint32_t name;
    uint32_t baseName;
    int planeCount;
    int nodeCount;
    int surfaceCount;
    GfxWorldStreamInfo_32 streamInfo;
    int skySurfCount;
    uint32_t skyStartSurfs;
    uint32_t skyImage;
    uint8_t skySamplerState;
    uint8_t padSky[3];
    uint32_t skyBoxModel;
    SunLightParseParams sunParse;
    uint32_t sunLight;
    float sunColorFromBsp[3];
    unsigned int sunPrimaryLightIndex;
    unsigned int primaryLightCount;
    int cullGroupCount;
    unsigned int coronaCount;
    uint32_t coronas;
    unsigned int shadowMapVolumeCount;
    uint32_t shadowMapVolumes;
    unsigned int shadowMapVolumePlaneCount;
    uint32_t shadowMapVolumePlanes;
    unsigned int exposureVolumeCount;
    uint32_t exposureVolumes;
    unsigned int exposureVolumePlaneCount;
    uint32_t exposureVolumePlanes;
    GfxSkyDynamicIntensity skyDynIntensity;
    GfxWorldDpvsPlanes_32 dpvsPlanes;
    int cellBitsCount;
    uint32_t cells;
    GfxWorldDraw_32 draw;
    GfxLightGrid_32 lightGrid;
    int modelCount;
    uint32_t models;
    float mins[3];
    float maxs[3];
    unsigned int checksum;
    int materialMemoryCount;
    uint32_t materialMemory;
    sunflare_t_32 sun;
    float outdoorLookupMatrix[4][4];
    uint32_t outdoorImage;
    uint32_t cellCasterBits;
    uint32_t sceneDynModel;
    uint32_t sceneDynBrush;
    uint32_t primaryLightEntityShadowVis;
    uint32_t primaryLightDynEntShadowVis[2];
    uint32_t nonSunPrimaryLightForModelDynEnt;
    uint32_t shadowGeom;
    uint32_t lightRegion;
    GfxWorldDpvsStatic_32 dpvs;
    GfxWorldDpvsDynamic_32 dpvsDyn;
    unsigned int worldLodChainCount;
    uint32_t worldLodChains;
    unsigned int worldLodInfoCount;
    uint32_t worldLodInfos;
    unsigned int worldLodSurfaceCount;
    uint32_t worldLodSurfaces;
    float waterDirection;
    GfxWaterBuffer_32 waterBuffers[2];
    uint32_t waterMaterial;
    uint32_t coronaMaterial;
    uint32_t ropeMaterial;
    unsigned int numOccluders;
    uint32_t occluders;
    unsigned int numOutdoorBounds;
    uint32_t outdoorBounds;
    unsigned int heroLightCount;
    unsigned int heroLightTreeCount;
    uint32_t heroLights;
    uint32_t heroLightTree;
};
#pragma pack(pop)

static_assert(sizeof(GfxWorldStreamInfo_32) == 16, "Win32 streamInfo");
static_assert(sizeof(GfxWorldDraw_32) == 192, "Win32 draw");
static_assert(sizeof(GfxLightGrid_32) == 56, "Win32 lightGrid");
static_assert(sizeof(GfxWorldDpvsPlanes_32) == 16, "Win32 dpvsPlanes");
static_assert(sizeof(GfxWorldDpvsStatic_32) == 112, "Win32 dpvs");
static_assert(sizeof(GfxWorldDpvsDynamic_32) == 48, "Win32 dpvsDyn");
static_assert(sizeof(sunflare_t_32) == 96, "Win32 sunflare");
static_assert(sizeof(GfxPortal_32) == 68, "Win32 portal");
static_assert(sizeof(GfxAabbTree_32) == 40, "Win32 aabbTree");
static_assert(sizeof(GfxCell_32) == 56, "Win32 cell");
static_assert(sizeof(GfxSurface_32) == 80, "Win32 surface");
static_assert(sizeof(GfxStaticModelDrawInst_32) == 76, "Win32 smodelDraw");
static_assert(sizeof(GfxLightmapArray_32) == 12, "Win32 lightmap");
static_assert(sizeof(GfxReflectionProbe_32) == 24, "Win32 reflection");
static_assert(sizeof(MaterialMemory_32) == 8, "Win32 matMem");
static_assert(sizeof(GfxShadowGeometry_32) == 12, "Win32 shadowGeom");
static_assert(sizeof(GfxLightRegion_32) == 8, "Win32 lightRegion");
static_assert(sizeof(GfxLightRegionHull_32) == 80, "Win32 lightRegionHull");
static_assert(sizeof(GfxWorld_32) == 1084, "Win32 GfxWorld");

static void DB_Expand_StreamInfo(GfxWorldStreamInfo *dst, const GfxWorldStreamInfo_32 &s)
{
    dst->aabbTreeCount = s.aabbTreeCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->aabbTrees), s.aabbTrees);
    dst->leafRefCount = s.leafRefCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->leafRefs), s.leafRefs);
}

static void DB_Expand_WorldDraw(GfxWorldDraw *dst, const GfxWorldDraw_32 &s)
{
    dst->reflectionProbeCount = s.reflectionProbeCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->reflectionProbes), s.reflectionProbes);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->reflectionProbeTextures), s.reflectionProbeTextures);
    dst->lightmapCount = s.lightmapCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->lightmaps), s.lightmaps);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->lightmapPrimaryTextures), s.lightmapPrimaryTextures);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->lightmapSecondaryTextures), s.lightmapSecondaryTextures);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->lightmapSecondaryTexturesB), s.lightmapSecondaryTexturesB);
    for (int i = 0; i < 31; ++i)
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->terrainScorchImages[i]), s.terrainScorchImages[i]);
    dst->vertexCount = s.vertexCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->vd.vertices), s.vd.vertices);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->vd.worldVb), s.vd.worldVb);
    dst->vertexLayerDataSize = s.vertexLayerDataSize;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->vld.data), s.vld.data);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->vld.layerVb), s.vld.layerVb);
    dst->vertexStream2DataSize = s.vertexStream2DataSize;
    dst->indexCount = s.indexCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->indices), s.indices);
}

static void DB_Expand_LightGrid(GfxLightGrid *dst, const GfxLightGrid_32 &s)
{
    dst->hasLightRegions = s.hasLightRegions != 0;
    dst->sunPrimaryLightIndex = s.sunPrimaryLightIndex;
    memcpy(dst->mins, s.mins, sizeof(dst->mins));
    memcpy(dst->maxs, s.maxs, sizeof(dst->maxs));
    dst->rowAxis = s.rowAxis;
    dst->colAxis = s.colAxis;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->rowDataStart), s.rowDataStart);
    dst->rawRowDataSize = s.rawRowDataSize;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->rawRowData), s.rawRowData);
    dst->entryCount = s.entryCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->entries), s.entries);
    dst->colorCount = s.colorCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->colors), s.colors);
}

static void DB_Expand_DpvsPlanes(GfxWorldDpvsPlanes *dst, const GfxWorldDpvsPlanes_32 &s)
{
    dst->cellCount = s.cellCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->planes), s.planes);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->nodes), s.nodes);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->sceneEntCellBits), s.sceneEntCellBits);
}

static void DB_Expand_DpvsStatic(GfxWorldDpvsStatic *dst, const GfxWorldDpvsStatic_32 &s)
{
    dst->smodelCount = s.smodelCount;
    dst->dynamicSModelCount = s.dynamicSModelCount;
    dst->staticSurfaceCount = s.staticSurfaceCount;
    dst->litSurfsBegin = s.litSurfsBegin;
    dst->litSurfsEnd = s.litSurfsEnd;
    dst->decalSurfsBegin = s.decalSurfsBegin;
    dst->decalSurfsEnd = s.decalSurfsEnd;
    dst->emissiveSurfsBegin = s.emissiveSurfsBegin;
    dst->emissiveSurfsEnd = s.emissiveSurfsEnd;
    dst->smodelVisDataCount = s.smodelVisDataCount;
    dst->surfaceVisDataCount = s.surfaceVisDataCount;
    for (int i = 0; i < 3; ++i)
    {
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->smodelVisData[i]), s.smodelVisData[i]);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->surfaceVisData[i]), s.surfaceVisData[i]);
    }
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->smodelVisDataCameraSaved), s.smodelVisDataCameraSaved);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->surfaceVisDataCameraSaved), s.surfaceVisDataCameraSaved);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->lodData), s.lodData);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->sortedSurfIndex), s.sortedSurfIndex);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->smodelInsts), s.smodelInsts);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->surfaces), s.surfaces);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->cullGroups), s.cullGroups);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->smodelDrawInsts), s.smodelDrawInsts);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->surfaceMaterials), s.surfaceMaterials);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->surfaceCastsSunShadow), s.surfaceCastsSunShadow);
    dst->usageCount = s.usageCount;
}

static void DB_Expand_DpvsDynamic(GfxWorldDpvsDynamic *dst, const GfxWorldDpvsDynamic_32 &s)
{
    memcpy(dst->dynEntClientWordCount, s.dynEntClientWordCount, sizeof(dst->dynEntClientWordCount));
    memcpy(dst->dynEntClientCount, s.dynEntClientCount, sizeof(dst->dynEntClientCount));
    for (int i = 0; i < 2; ++i)
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->dynEntCellBits[i]), s.dynEntCellBits[i]);
    for (int i = 0; i < 2; ++i)
        for (int j = 0; j < 3; ++j)
            DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->dynEntVisData[i][j]), s.dynEntVisData[i][j]);
}

static void DB_Expand_sunflare(sunflare_t *dst, const sunflare_t_32 &s)
{
    dst->hasValidData = s.hasValidData != 0;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->spriteMaterial), s.spriteMaterial);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->flareMaterial), s.flareMaterial);
    dst->spriteSize = s.spriteSize;
    dst->flareMinSize = s.flareMinSize;
    dst->flareMinDot = s.flareMinDot;
    dst->flareMaxSize = s.flareMaxSize;
    dst->flareMaxDot = s.flareMaxDot;
    dst->flareMaxAlpha = s.flareMaxAlpha;
    dst->flareFadeInTime = s.flareFadeInTime;
    dst->flareFadeOutTime = s.flareFadeOutTime;
    dst->blindMinDot = s.blindMinDot;
    dst->blindMaxDot = s.blindMaxDot;
    dst->blindMaxDarken = s.blindMaxDarken;
    dst->blindFadeInTime = s.blindFadeInTime;
    dst->blindFadeOutTime = s.blindFadeOutTime;
    dst->glareMinDot = s.glareMinDot;
    dst->glareMaxDot = s.glareMaxDot;
    dst->glareMaxLighten = s.glareMaxLighten;
    dst->glareFadeInTime = s.glareFadeInTime;
    dst->glareFadeOutTime = s.glareFadeOutTime;
    memcpy(dst->sunFxPosition, s.sunFxPosition, sizeof(dst->sunFxPosition));
}

static void DB_Expand_WaterBuffer(GfxWaterBuffer *dst, const GfxWaterBuffer_32 &s)
{
    dst->bufferSize = s.bufferSize;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->buffer), s.buffer);
}

void DB_ExpandLoad_GfxWorld(GfxWorld *dst)
{
    iassert(dst == reinterpret_cast<GfxWorld *>(DB_GetStreamPos()));

    GfxWorld_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));

    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->baseName), disk.baseName);
    dst->planeCount = disk.planeCount;
    dst->nodeCount = disk.nodeCount;
    dst->surfaceCount = disk.surfaceCount;
    DB_Expand_StreamInfo(&dst->streamInfo, disk.streamInfo);
    dst->skySurfCount = disk.skySurfCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->skyStartSurfs), disk.skyStartSurfs);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->skyImage), disk.skyImage);
    dst->skySamplerState = disk.skySamplerState;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->skyBoxModel), disk.skyBoxModel);
    dst->sunParse = disk.sunParse;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->sunLight), disk.sunLight);
    memcpy(dst->sunColorFromBsp, disk.sunColorFromBsp, sizeof(dst->sunColorFromBsp));
    dst->sunPrimaryLightIndex = disk.sunPrimaryLightIndex;
    dst->primaryLightCount = disk.primaryLightCount;
    dst->cullGroupCount = disk.cullGroupCount;
    dst->coronaCount = disk.coronaCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->coronas), disk.coronas);
    dst->shadowMapVolumeCount = disk.shadowMapVolumeCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->shadowMapVolumes), disk.shadowMapVolumes);
    dst->shadowMapVolumePlaneCount = disk.shadowMapVolumePlaneCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->shadowMapVolumePlanes), disk.shadowMapVolumePlanes);
    dst->exposureVolumeCount = disk.exposureVolumeCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->exposureVolumes), disk.exposureVolumes);
    dst->exposureVolumePlaneCount = disk.exposureVolumePlaneCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->exposureVolumePlanes), disk.exposureVolumePlanes);
    dst->skyDynIntensity = disk.skyDynIntensity;
    DB_Expand_DpvsPlanes(&dst->dpvsPlanes, disk.dpvsPlanes);
    dst->cellBitsCount = disk.cellBitsCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->cells), disk.cells);
    DB_Expand_WorldDraw(&dst->draw, disk.draw);
    DB_Expand_LightGrid(&dst->lightGrid, disk.lightGrid);
    dst->modelCount = disk.modelCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->models), disk.models);
    memcpy(dst->mins, disk.mins, sizeof(dst->mins));
    memcpy(dst->maxs, disk.maxs, sizeof(dst->maxs));
    dst->checksum = disk.checksum;
    dst->materialMemoryCount = disk.materialMemoryCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->materialMemory), disk.materialMemory);
    DB_Expand_sunflare(&dst->sun, disk.sun);
    memcpy(dst->outdoorLookupMatrix, disk.outdoorLookupMatrix, sizeof(dst->outdoorLookupMatrix));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->outdoorImage), disk.outdoorImage);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->cellCasterBits), disk.cellCasterBits);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->sceneDynModel), disk.sceneDynModel);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->sceneDynBrush), disk.sceneDynBrush);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->primaryLightEntityShadowVis), disk.primaryLightEntityShadowVis);
    for (int i = 0; i < 2; ++i)
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->primaryLightDynEntShadowVis[i]), disk.primaryLightDynEntShadowVis[i]);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->nonSunPrimaryLightForModelDynEnt), disk.nonSunPrimaryLightForModelDynEnt);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->shadowGeom), disk.shadowGeom);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->lightRegion), disk.lightRegion);
    DB_Expand_DpvsStatic(&dst->dpvs, disk.dpvs);
    DB_Expand_DpvsDynamic(&dst->dpvsDyn, disk.dpvsDyn);
    dst->worldLodChainCount = disk.worldLodChainCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->worldLodChains), disk.worldLodChains);
    dst->worldLodInfoCount = disk.worldLodInfoCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->worldLodInfos), disk.worldLodInfos);
    dst->worldLodSurfaceCount = disk.worldLodSurfaceCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->worldLodSurfaces), disk.worldLodSurfaces);
    dst->waterDirection = disk.waterDirection;
    DB_Expand_WaterBuffer(&dst->waterBuffers[0], disk.waterBuffers[0]);
    DB_Expand_WaterBuffer(&dst->waterBuffers[1], disk.waterBuffers[1]);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(const_cast<Material **>(&dst->waterMaterial)), disk.waterMaterial);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(const_cast<Material **>(&dst->coronaMaterial)), disk.coronaMaterial);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(const_cast<Material **>(&dst->ropeMaterial)), disk.ropeMaterial);
    dst->numOccluders = disk.numOccluders;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->occluders), disk.occluders);
    dst->numOutdoorBounds = disk.numOutdoorBounds;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->outdoorBounds), disk.outdoorBounds);
    dst->heroLightCount = disk.heroLightCount;
    dst->heroLightTreeCount = disk.heroLightTreeCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->heroLights), disk.heroLights);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->heroLightTree), disk.heroLightTree);

    DB_Expand_Commit(static_cast<int>(sizeof(GfxWorld)));
}

void DB_ExpandLoad_GfxCellArray(GfxCell *dst, int count)
{
    iassert(dst == reinterpret_cast<GfxCell *>(DB_GetStreamPos()));
    iassert(count >= 0);
    count = DB_Expand_ClampCount(count, "GfxCell_32");
    std::vector<GfxCell_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(GfxCell_32) * count));
    memset(dst, 0, sizeof(GfxCell) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        const GfxCell_32 &s = disk[static_cast<size_t>(i)];
        memcpy(dst[i].mins, s.mins, sizeof(dst[i].mins));
        memcpy(dst[i].maxs, s.maxs, sizeof(dst[i].maxs));
        dst[i].aabbTreeCount = s.aabbTreeCount;
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].aabbTree), s.aabbTree);
        dst[i].portalCount = s.portalCount;
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].portals), s.portals);
        dst[i].cullGroupCount = s.cullGroupCount;
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].cullGroups), s.cullGroups);
        dst[i].reflectionProbeCount = s.reflectionProbeCount;
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].reflectionProbes), s.reflectionProbes);
    }
    DB_Expand_Commit(static_cast<int>(sizeof(GfxCell) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_GfxAabbTreeArray(GfxAabbTree *dst, int count)
{
    iassert(dst == reinterpret_cast<GfxAabbTree *>(DB_GetStreamPos()));
    iassert(count >= 0);
    count = DB_Expand_ClampCount(count, "GfxAabbTree_32");
    std::vector<GfxAabbTree_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(GfxAabbTree_32) * count));
    memset(dst, 0, sizeof(GfxAabbTree) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        const GfxAabbTree_32 &s = disk[static_cast<size_t>(i)];
        memcpy(dst[i].mins, s.mins, sizeof(dst[i].mins));
        memcpy(dst[i].maxs, s.maxs, sizeof(dst[i].maxs));
        dst[i].childCount = s.childCount;
        dst[i].surfaceCount = s.surfaceCount;
        dst[i].startSurfIndex = s.startSurfIndex;
        dst[i].smodelIndexCount = s.smodelIndexCount;
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].smodelIndexes), s.smodelIndexes);
        // The offset is in bytes from this node to its first child within the same node array, so it counts Win32
        // nodes (40 bytes); rescale it to the native node size.
        iassert(s.childrenOffset % static_cast<int>(sizeof(GfxAabbTree_32)) == 0);
        dst[i].childrenOffset = s.childrenOffset / static_cast<int>(sizeof(GfxAabbTree_32))
            * static_cast<int>(sizeof(GfxAabbTree));
    }
    DB_Expand_Commit(static_cast<int>(sizeof(GfxAabbTree) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_GfxPortalArray(GfxPortal *dst, int count)
{
    iassert(dst == reinterpret_cast<GfxPortal *>(DB_GetStreamPos()));
    iassert(count >= 0);
    count = DB_Expand_ClampCount(count, "GfxPortal_32");
    std::vector<GfxPortal_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(GfxPortal_32) * count));
    memset(dst, 0, sizeof(GfxPortal) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        const GfxPortal_32 &s = disk[static_cast<size_t>(i)];
        dst[i].writable.isQueued = s.writable.isQueued != 0;
        dst[i].writable.isAncestor = s.writable.isAncestor != 0;
        dst[i].writable.recursionDepth = s.writable.recursionDepth;
        dst[i].writable.hullPointCount = s.writable.hullPointCount;
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].writable.hullPoints), s.writable.hullPoints);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].writable.queuedParent), s.writable.queuedParent);
        dst[i].plane = s.plane;
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].cell), s.cell);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].vertices), s.vertices);
        dst[i].vertexCount = s.vertexCount;
        memcpy(dst[i].hullAxis, s.hullAxis, sizeof(dst[i].hullAxis));
    }
    DB_Expand_Commit(static_cast<int>(sizeof(GfxPortal) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_GfxSurfaceArray(GfxSurface *dst, int count)
{
    iassert(dst == reinterpret_cast<GfxSurface *>(DB_GetStreamPos()));
    iassert(count >= 0);
    count = DB_Expand_ClampCount(count, "GfxSurface_32");
    std::vector<GfxSurface_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(GfxSurface_32) * count));
    memset(dst, 0, sizeof(GfxSurface) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        const GfxSurface_32 &s = disk[static_cast<size_t>(i)];
        dst[i].tris = s.tris;
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].material), s.material);
        dst[i].lightmapIndex = s.lightmapIndex;
        dst[i].reflectionProbeIndex = s.reflectionProbeIndex;
        dst[i].primaryLightIndex = s.primaryLightIndex;
        dst[i].flags = s.flags;
        memcpy(dst[i].bounds, s.bounds, sizeof(dst[i].bounds));
    }
    DB_Expand_Commit(static_cast<int>(sizeof(GfxSurface) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_GfxStaticModelDrawInstArray(GfxStaticModelDrawInst *dst, int count)
{
    iassert(dst == reinterpret_cast<GfxStaticModelDrawInst *>(DB_GetStreamPos()));
    iassert(count >= 0);
    count = DB_Expand_ClampCount(count, "GfxStaticModelDrawInst_32");
    std::vector<GfxStaticModelDrawInst_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(GfxStaticModelDrawInst_32) * count));
    memset(dst, 0, sizeof(GfxStaticModelDrawInst) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        const GfxStaticModelDrawInst_32 &s = disk[static_cast<size_t>(i)];
        dst[i].cullDist = s.cullDist;
        dst[i].placement = s.placement;
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].model), s.model);
        dst[i].flags = s.flags;
        memcpy(dst[i].smodelCacheIndex, s.smodelCacheIndex, sizeof(dst[i].smodelCacheIndex));
        dst[i].lightingHandle = s.lightingHandle;
        dst[i].reflectionProbeIndex = s.reflectionProbeIndex;
        dst[i].primaryLightIndex = s.primaryLightIndex;
    }
    DB_Expand_Commit(static_cast<int>(sizeof(GfxStaticModelDrawInst) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_GfxLightmapArrayArray(GfxLightmapArray *dst, int count)
{
    iassert(dst == reinterpret_cast<GfxLightmapArray *>(DB_GetStreamPos()));
    iassert(count >= 0);
    count = DB_Expand_ClampCount(count, "GfxLightmapArray_32");
    std::vector<GfxLightmapArray_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(GfxLightmapArray_32) * count));
    memset(dst, 0, sizeof(GfxLightmapArray) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].primary), disk[static_cast<size_t>(i)].primary);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].secondary), disk[static_cast<size_t>(i)].secondary);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].secondaryB), disk[static_cast<size_t>(i)].secondaryB);
    }
    DB_Expand_Commit(static_cast<int>(sizeof(GfxLightmapArray) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_GfxReflectionProbeArray(GfxReflectionProbe *dst, int count)
{
    iassert(dst == reinterpret_cast<GfxReflectionProbe *>(DB_GetStreamPos()));
    iassert(count >= 0);
    count = DB_Expand_ClampCount(count, "GfxReflectionProbe_32");
    std::vector<GfxReflectionProbe_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(GfxReflectionProbe_32) * count));
    memset(dst, 0, sizeof(GfxReflectionProbe) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        const GfxReflectionProbe_32 &s = disk[static_cast<size_t>(i)];
        memcpy(dst[i].origin, s.origin, sizeof(dst[i].origin));
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].reflectionImage), s.reflectionImage);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].probeVolumes), s.probeVolumes);
        dst[i].probeVolumeCount = s.probeVolumeCount;
    }
    DB_Expand_Commit(static_cast<int>(sizeof(GfxReflectionProbe) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_MaterialMemoryArray(MaterialMemory *dst, int count)
{
    iassert(dst == reinterpret_cast<MaterialMemory *>(DB_GetStreamPos()));
    iassert(count >= 0);
    count = DB_Expand_ClampCount(count, "MaterialMemory_32");
    std::vector<MaterialMemory_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(MaterialMemory_32) * count));
    memset(dst, 0, sizeof(MaterialMemory) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].material), disk[static_cast<size_t>(i)].material);
        dst[i].memory = disk[static_cast<size_t>(i)].memory;
    }
    DB_Expand_Commit(static_cast<int>(sizeof(MaterialMemory) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_GfxShadowGeometryArray(GfxShadowGeometry *dst, int count)
{
    iassert(dst == reinterpret_cast<GfxShadowGeometry *>(DB_GetStreamPos()));
    iassert(count >= 0);
    count = DB_Expand_ClampCount(count, "GfxShadowGeometry_32");
    std::vector<GfxShadowGeometry_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(GfxShadowGeometry_32) * count));
    memset(dst, 0, sizeof(GfxShadowGeometry) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        const GfxShadowGeometry_32 &s = disk[static_cast<size_t>(i)];
        dst[i].surfaceCount = s.surfaceCount;
        dst[i].smodelCount = s.smodelCount;
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].sortedSurfIndex), s.sortedSurfIndex);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].smodelIndex), s.smodelIndex);
    }
    DB_Expand_Commit(static_cast<int>(sizeof(GfxShadowGeometry) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_GfxLightRegionArray(GfxLightRegion *dst, int count)
{
    iassert(dst == reinterpret_cast<GfxLightRegion *>(DB_GetStreamPos()));
    iassert(count >= 0);
    count = DB_Expand_ClampCount(count, "GfxLightRegion_32");
    std::vector<GfxLightRegion_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(GfxLightRegion_32) * count));
    memset(dst, 0, sizeof(GfxLightRegion) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        dst[i].hullCount = disk[static_cast<size_t>(i)].hullCount;
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].hulls), disk[static_cast<size_t>(i)].hulls);
    }
    DB_Expand_Commit(static_cast<int>(sizeof(GfxLightRegion) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_GfxLightRegionHullArray(GfxLightRegionHull *dst, int count)
{
    iassert(dst == reinterpret_cast<GfxLightRegionHull *>(DB_GetStreamPos()));
    iassert(count >= 0);
    count = DB_Expand_ClampCount(count, "GfxLightRegionHull_32");
    std::vector<GfxLightRegionHull_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(GfxLightRegionHull_32) * count));
    memset(dst, 0, sizeof(GfxLightRegionHull) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        const GfxLightRegionHull_32 &s = disk[static_cast<size_t>(i)];
        memcpy(dst[i].kdopMidPoint, s.kdopMidPoint, sizeof(dst[i].kdopMidPoint));
        memcpy(dst[i].kdopHalfSize, s.kdopHalfSize, sizeof(dst[i].kdopHalfSize));
        dst[i].axisCount = s.axisCount;
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].axis), s.axis);
    }
    DB_Expand_Commit(static_cast<int>(sizeof(GfxLightRegionHull) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_sunflare_t(sunflare_t *dst)
{
    iassert(dst == reinterpret_cast<sunflare_t *>(DB_GetStreamPos()));
    sunflare_t_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_Expand_sunflare(dst, disk);
    DB_Expand_Commit(static_cast<int>(sizeof(sunflare_t)));
}

void DB_ExpandLoad_GfxLight(GfxLight *dst)
{
    // Win32 and native sizeof are both 368; def lives at +0x160.
    uint8_t disk[368];
    DB_Expand_ReadFile(disk, 368);
    memset(dst, 0, sizeof(*dst));
    memcpy(dst, disk, 0x160);
    uint32_t def32 = 0;
    memcpy(&def32, disk + 0x160, 4);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->def), def32);
    DB_Expand_Commit(static_cast<int>(sizeof(GfxLight)));
}



#include <qcommon/com_bsp.h>
#include <game/g_bsp.h>
#include <game/pathnode.h>
#include <glass/glass.h>
#include <gfx_d3d/r_font.h>
#include <gfx_d3d/r_gfx.h>
#include <gfx_d3d/r_primarylights.h>
#include <stringed/stringed_hooks.h>
#include "db_registry.h"

#pragma pack(push, 4)
struct ComPrimaryLight_32
{
    uint8_t type;
    uint8_t canUseShadowMap;
    uint8_t exponent;
    char priority;
    int16_t cullDist;
    uint8_t pad[2];
    float color[3];
    float dir[3];
    float origin[3];
    float radius;
    float cosHalfFovOuter;
    float cosHalfFovInner;
    float cosHalfFovExpanded;
    float rotationLimit;
    float translationLimit;
    float mipDistance;
    float diffuseColor[4];
    float specularColor[4];
    float attenuation[4];
    float falloff[4];
    float angle[4];
    float aAbB[4];
    float cookieControl0[4];
    float cookieControl1[4];
    float cookieControl2[4];
    uint32_t defName;
};
struct ComBurnableCell_32
{
    int x;
    int y;
    uint32_t data;
};
struct ComWorld_32
{
    uint32_t name;
    int isInUse;
    unsigned int primaryLightCount;
    uint32_t primaryLights;
    ComWaterHeader waterHeader;
    unsigned int numWaterCells;
    uint32_t waterCells;
    ComBurnableHeader burnableHeader;
    unsigned int numBurnableCells;
    uint32_t burnableCells;
};
struct PathData_32
{
    unsigned int nodeCount;
    uint32_t nodes;
    uint32_t basenodes;
    unsigned int chainNodeCount;
    uint32_t chainNodeForNode;
    uint32_t nodeForChainNode;
    int visBytes;
    uint32_t pathVis;
    int nodeTreeCount;
    uint32_t nodeTree;
};
struct GameWorld_32
{
    uint32_t name;
    PathData_32 path;
};
struct pathnode_constant_32
{
    int type;
    uint16_t spawnflags;
    uint16_t targetname;
    uint16_t script_linkName;
    uint16_t script_noteworthy;
    uint16_t target;
    uint16_t animscript;
    int animscriptfunc;
    float vOrigin[3];
    float fAngle;
    float forward[2];
    float fRadius;
    float minUseDistSq;
    int16_t wOverlapNode[2];
    int16_t wChainId;
    int16_t wChainDepth;
    int16_t wChainParent;
    uint16_t totalLinkCount;
    uint32_t Links;
};
struct pathnode_dynamic_32
{
    uint8_t blob[32];
};
struct pathnode_transient_32
{
    int iSearchFrame;
    uint32_t pNextOpen;
    uint32_t pPrevOpen;
    uint32_t pParent;
    float fCost;
    float fHeuristic;
    float costFactor;
};
struct pathnode_32
{
    pathnode_constant_32 constant;
    pathnode_dynamic_32 dynamic;
    pathnode_transient_32 transient;
};
struct pathnode_tree_nodes_32
{
    int nodeCount;
    uint32_t nodes;
};
struct pathnode_tree_32
{
    int axis;
    float dist;
    uint32_t u0;
    uint32_t u1;
};
struct Glasses_32
{
    uint32_t name;
    unsigned int numGlasses;
    uint32_t glasses;
    uint32_t workMemory;
    unsigned int workMemorySize;
    unsigned int smallAllocatorBlocks;
    unsigned int maxGroups;
    unsigned int maxShards;
    unsigned int maxPhysics;
    unsigned int shardMemorySize;
    unsigned int maxFreeCmd;
    unsigned int numSlots;
    unsigned int numVerts;
    unsigned int numIndices;
};
struct Glass_32
{
    uint32_t glassDef;
    unsigned int index;
    unsigned int brushModel;
    float origin[3];
    float angles[3];
    float absmin[3];
    float absmax[3];
    uint8_t isPlanar;
    uint8_t numOutlineVerts;
    uint8_t pad[2];
    uint32_t outline;
    float outlineAxis[3][3];
    float outlineOrigin[3];
    float uvScale;
    float thickness;
};
struct GlassDef_32
{
    uint32_t name;
    int maxHealth;
    float thickness;
    float minShardSize;
    float maxShardSize;
    float shardLifeProbablility;
    int maxShards;
    uint32_t pristineMaterial;
    uint32_t crackedMaterial;
    uint32_t shardMaterial;
    uint32_t crackSound;
    uint32_t shatterShound;
    uint32_t autoShatterShound;
    uint32_t crackEffect;
    uint32_t shatterEffect;
};
struct GfxLightImage_32
{
    uint32_t image;
    uint8_t samplerState;
    uint8_t pad[3];
};
struct GfxLightDef_32
{
    uint32_t name;
    GfxLightImage_32 attenuation;
    int lmapLookupStart;
};
struct Font_32
{
    uint32_t fontName;
    int pixelHeight;
    int glyphCount;
    uint32_t material;
    uint32_t glowMaterial;
    uint32_t glyphs;
};
struct LocalizeEntry_32
{
    uint32_t value;
    uint32_t name;
};

#pragma pack(pop)

static_assert(sizeof(ComPrimaryLight_32) == 220, "Win32 ComPrimaryLight");
static_assert(sizeof(ComBurnableCell_32) == 12, "Win32 ComBurnableCell");
static_assert(sizeof(ComWorld_32) == 64, "Win32 ComWorld");
static_assert(sizeof(PathData_32) == 40, "Win32 PathData");
static_assert(sizeof(GameWorld_32) == 44, "Win32 GameWorld");
static_assert(sizeof(pathnode_constant_32) == 68, "Win32 pathnode_constant");
static_assert(sizeof(pathnode_32) == 128, "Win32 pathnode_t");
static_assert(sizeof(pathnode_tree_32) == 16, "Win32 pathnode_tree");
static_assert(sizeof(pathnode_tree_nodes_32) == 8, "Win32 pathnode_tree_nodes");
static_assert(sizeof(Glasses_32) == 56, "Win32 Glasses");
static_assert(sizeof(Glass_32) == 124, "Win32 Glass");
static_assert(sizeof(GlassDef_32) == 60, "Win32 GlassDef");
static_assert(sizeof(GfxLightDef_32) == 16, "Win32 GfxLightDef");
static_assert(sizeof(Font_32) == 24, "Win32 Font");
static_assert(sizeof(LocalizeEntry_32) == 8, "Win32 LocalizeEntry");


static void DB_Expand_PathData(PathData *dst, const PathData_32 &s)
{
    dst->nodeCount = s.nodeCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->nodes), s.nodes);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->basenodes), s.basenodes);
    dst->chainNodeCount = s.chainNodeCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->chainNodeForNode), s.chainNodeForNode);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->nodeForChainNode), s.nodeForChainNode);
    dst->visBytes = s.visBytes;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->pathVis), s.pathVis);
    dst->nodeTreeCount = s.nodeTreeCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->nodeTree), s.nodeTree);
}

void DB_ExpandLoad_ComWorld(ComWorld *dst)
{
    iassert(dst == reinterpret_cast<ComWorld *>(DB_GetStreamPos()));
    ComWorld_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    dst->isInUse = disk.isInUse;
    dst->primaryLightCount = disk.primaryLightCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->primaryLights), disk.primaryLights);
    dst->waterHeader = disk.waterHeader;
    dst->numWaterCells = disk.numWaterCells;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->waterCells), disk.waterCells);
    dst->burnableHeader = disk.burnableHeader;
    dst->numBurnableCells = disk.numBurnableCells;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->burnableCells), disk.burnableCells);
    DB_Expand_Commit(static_cast<int>(sizeof(ComWorld)));
}

void DB_ExpandLoad_ComPrimaryLightArray(ComPrimaryLight *dst, int count)
{
    iassert(reinterpret_cast<unsigned __int8 *>(&dst->type) == DB_GetStreamPos() ||
            reinterpret_cast<unsigned __int8 *>(dst) == DB_GetStreamPos());
    iassert(count >= 0);
    count = DB_Expand_ClampCount(count, "ComPrimaryLight_32");
    std::vector<ComPrimaryLight_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(ComPrimaryLight_32) * count));
    memset(dst, 0, sizeof(ComPrimaryLight) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        const ComPrimaryLight_32 &s = disk[static_cast<size_t>(i)];
        // Scalars through cookieControl2 match; only defName widens.
        memcpy(&dst[i], &s, offsetof(ComPrimaryLight_32, defName));
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(const_cast<char **>(&dst[i].defName)), s.defName);
    }
    DB_Expand_Commit(static_cast<int>(sizeof(ComPrimaryLight) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_ComBurnableCellArray(ComBurnableCell *dst, int count)
{
    iassert(dst == reinterpret_cast<ComBurnableCell *>(DB_GetStreamPos()));
    iassert(count >= 0);
    count = DB_Expand_ClampCount(count, "ComBurnableCell_32");
    std::vector<ComBurnableCell_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(ComBurnableCell_32) * count));
    memset(dst, 0, sizeof(ComBurnableCell) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        dst[i].x = disk[static_cast<size_t>(i)].x;
        dst[i].y = disk[static_cast<size_t>(i)].y;
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].data), disk[static_cast<size_t>(i)].data);
    }
    DB_Expand_Commit(static_cast<int>(sizeof(ComBurnableCell) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_GameWorldSp(GameWorldSp *dst)
{
    iassert(dst == reinterpret_cast<GameWorldSp *>(DB_GetStreamPos()));
    GameWorld_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    DB_Expand_PathData(&dst->path, disk.path);
    DB_Expand_Commit(static_cast<int>(sizeof(GameWorldSp)));
}

void DB_ExpandLoad_GameWorldMp(GameWorldMp *dst)
{
    iassert(dst == reinterpret_cast<GameWorldMp *>(DB_GetStreamPos()));
    GameWorld_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    DB_Expand_PathData(&dst->path, disk.path);
    DB_Expand_Commit(static_cast<int>(sizeof(GameWorldMp)));
}

void DB_ExpandLoad_pathnode_tArray(pathnode_t *dst, int count)
{
    iassert(dst == reinterpret_cast<pathnode_t *>(DB_GetStreamPos()));
    iassert(count >= 0);
    count = DB_Expand_ClampCount(count, "pathnode_32");
    std::vector<pathnode_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(pathnode_32) * count));
    memset(dst, 0, sizeof(pathnode_t) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        const pathnode_32 &s = disk[static_cast<size_t>(i)];
        dst[i].constant.type = static_cast<nodeType>(s.constant.type);
        dst[i].constant.spawnflags = s.constant.spawnflags;
        dst[i].constant.targetname = s.constant.targetname;
        dst[i].constant.script_linkName = s.constant.script_linkName;
        dst[i].constant.script_noteworthy = s.constant.script_noteworthy;
        dst[i].constant.target = s.constant.target;
        dst[i].constant.animscript = s.constant.animscript;
        dst[i].constant.animscriptfunc = s.constant.animscriptfunc;
        memcpy(dst[i].constant.vOrigin, s.constant.vOrigin, sizeof(dst[i].constant.vOrigin));
        dst[i].constant.fAngle = s.constant.fAngle;
        memcpy(dst[i].constant.forward, s.constant.forward, sizeof(dst[i].constant.forward));
        dst[i].constant.fRadius = s.constant.fRadius;
        dst[i].constant.minUseDistSq = s.constant.minUseDistSq;
        memcpy(dst[i].constant.wOverlapNode, s.constant.wOverlapNode, sizeof(dst[i].constant.wOverlapNode));
        dst[i].constant.wChainId = s.constant.wChainId;
        dst[i].constant.wChainDepth = s.constant.wChainDepth;
        dst[i].constant.wChainParent = s.constant.wChainParent;
        dst[i].constant.totalLinkCount = s.constant.totalLinkCount;
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].constant.Links), s.constant.Links);
        memcpy(&dst[i].dynamic, s.dynamic.blob, sizeof(dst[i].dynamic));
        dst[i].transient.iSearchFrame = s.transient.iSearchFrame;
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].transient.pNextOpen), s.transient.pNextOpen);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].transient.pPrevOpen), s.transient.pPrevOpen);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].transient.pParent), s.transient.pParent);
        dst[i].transient.fCost = s.transient.fCost;
        dst[i].transient.fHeuristic = s.transient.fHeuristic;
        dst[i].transient.costFactor = s.transient.costFactor;
    }
    DB_Expand_Commit(static_cast<int>(sizeof(pathnode_t) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_pathnode_tree_tArray(pathnode_tree_t *dst, int count)
{
    iassert(dst == reinterpret_cast<pathnode_tree_t *>(DB_GetStreamPos()));
    iassert(count >= 0);
    count = DB_Expand_ClampCount(count, "pathnode_tree_32");
    std::vector<pathnode_tree_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(pathnode_tree_32) * count));
    memset(dst, 0, sizeof(pathnode_tree_t) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        const pathnode_tree_32 &s = disk[static_cast<size_t>(i)];
        dst[i].axis = s.axis;
        dst[i].dist = s.dist;
        if (s.axis < 0)
        {
            dst[i].u.s.nodeCount = static_cast<int>(s.u0);
            DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].u.s.nodes), s.u1);
        }
        else
        {
            DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].u.child[0]), s.u0);
            DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].u.child[1]), s.u1);
        }
    }
    DB_Expand_Commit(static_cast<int>(sizeof(pathnode_tree_t) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_pathnode_tree_nodes_t(pathnode_tree_nodes_t *dst)
{
    iassert(dst == reinterpret_cast<pathnode_tree_nodes_t *>(DB_GetStreamPos()));
    pathnode_tree_nodes_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    dst->nodeCount = disk.nodeCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->nodes), disk.nodes);
    DB_Expand_Commit(static_cast<int>(sizeof(pathnode_tree_nodes_t)));
}

void DB_ExpandLoad_Glasses(Glasses *dst)
{
    iassert(dst == reinterpret_cast<Glasses *>(DB_GetStreamPos()));
    Glasses_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    dst->numGlasses = disk.numGlasses;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->glasses), disk.glasses);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->workMemory), disk.workMemory);
    dst->workMemorySize = disk.workMemorySize;
    dst->smallAllocatorBlocks = disk.smallAllocatorBlocks;
    dst->maxGroups = disk.maxGroups;
    dst->maxShards = disk.maxShards;
    dst->maxPhysics = disk.maxPhysics;
    dst->shardMemorySize = disk.shardMemorySize;
    dst->maxFreeCmd = disk.maxFreeCmd;
    dst->numSlots = disk.numSlots;
    dst->numVerts = disk.numVerts;
    dst->numIndices = disk.numIndices;
    DB_Expand_Commit(static_cast<int>(sizeof(Glasses)));
}

void DB_ExpandLoad_GlassArray(Glass *dst, int count)
{
    iassert(dst == reinterpret_cast<Glass *>(DB_GetStreamPos()));
    iassert(count >= 0);
    count = DB_Expand_ClampCount(count, "Glass_32");
    std::vector<Glass_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(Glass_32) * count));
    memset(const_cast<Glass *>(dst), 0, sizeof(Glass) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        const Glass_32 &s = disk[static_cast<size_t>(i)];
        auto *out = const_cast<Glass *>(&dst[i]);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&out->glassDef), s.glassDef);
        out->index = s.index;
        out->brushModel = s.brushModel;
        memcpy(out->origin, s.origin, sizeof(out->origin));
        memcpy(out->angles, s.angles, sizeof(out->angles));
        memcpy(out->absmin, s.absmin, sizeof(out->absmin));
        memcpy(out->absmax, s.absmax, sizeof(out->absmax));
        out->isPlanar = s.isPlanar != 0;
        out->numOutlineVerts = s.numOutlineVerts;
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&out->outline), s.outline);
        memcpy(out->outlineAxis, s.outlineAxis, sizeof(out->outlineAxis));
        memcpy(out->outlineOrigin, s.outlineOrigin, sizeof(out->outlineOrigin));
        out->uvScale = s.uvScale;
        out->thickness = s.thickness;
    }
    DB_Expand_Commit(static_cast<int>(sizeof(Glass) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_GlassDef(GlassDef *dst)
{
    iassert(dst == reinterpret_cast<GlassDef *>(DB_GetStreamPos()));
    GlassDef_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    dst->maxHealth = disk.maxHealth;
    dst->thickness = disk.thickness;
    dst->minShardSize = disk.minShardSize;
    dst->maxShardSize = disk.maxShardSize;
    dst->shardLifeProbablility = disk.shardLifeProbablility;
    dst->maxShards = disk.maxShards;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->pristineMaterial), disk.pristineMaterial);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->crackedMaterial), disk.crackedMaterial);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->shardMaterial), disk.shardMaterial);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->crackSound), disk.crackSound);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->shatterShound), disk.shatterShound);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->autoShatterShound), disk.autoShatterShound);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(const_cast<FxEffectDef **>(&dst->crackEffect)), disk.crackEffect);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(const_cast<FxEffectDef **>(&dst->shatterEffect)), disk.shatterEffect);
    DB_Expand_Commit(static_cast<int>(sizeof(GlassDef)));
}

void DB_ExpandLoad_GfxLightDef(GfxLightDef *dst)
{
    iassert(dst == reinterpret_cast<GfxLightDef *>(DB_GetStreamPos()));
    GfxLightDef_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->attenuation.image), disk.attenuation.image);
    dst->attenuation.samplerState = disk.attenuation.samplerState;
    dst->lmapLookupStart = disk.lmapLookupStart;
    DB_Expand_Commit(static_cast<int>(sizeof(GfxLightDef)));
}

void DB_ExpandLoad_Font(Font_s *dst)
{
    iassert(dst == reinterpret_cast<Font_s *>(DB_GetStreamPos()));
    Font_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->fontName), disk.fontName);
    dst->pixelHeight = disk.pixelHeight;
    dst->glyphCount = disk.glyphCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->material), disk.material);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->glowMaterial), disk.glowMaterial);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->glyphs), disk.glyphs);
    DB_Expand_Commit(static_cast<int>(sizeof(Font_s)));
}

void DB_ExpandLoad_LocalizeEntry(LocalizeEntry *dst)
{
    iassert(dst == reinterpret_cast<LocalizeEntry *>(DB_GetStreamPos()));
    LocalizeEntry_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->value), disk.value);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    DB_Expand_Commit(static_cast<int>(sizeof(LocalizeEntry)));
}

#endif // normal engine preceding RawFile body
#if !defined(BO1_ASSETLIST_GATE) || defined(BO1_RAWFILE_HEADER_GATE)
void DB_ExpandLoad_RawFile(RawFile *dst)
{
    iassert(dst == reinterpret_cast<RawFile *>(DB_GetStreamPos()));
    RawFile_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    dst->len = disk.len;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->buffer), disk.buffer);
    DB_Expand_Commit(static_cast<int>(sizeof(RawFile)));
}
#endif // normal engine or bounded RawFile numeric header
#if !defined(BO1_ASSETLIST_GATE)




#include <sound/snd_bank.h>
#include <gfx_d3d/fxprimitives.h>
#include <physics/destructible.h>

#pragma pack(push, 1)
struct XAnimParts_32
{
    uint32_t name;
    uint16_t dataByteCount;
    uint16_t dataShortCount;
    uint16_t dataIntCount;
    uint16_t randomDataByteCount;
    uint16_t randomDataIntCount;
    uint16_t numframes;
    uint8_t bLoop;
    uint8_t bDelta;
    uint8_t bLeftHandGripIK;
    uint8_t bStreamable;
    uint32_t streamedFileSize;
    uint8_t boneCount[10];
    uint8_t notifyCount;
    uint8_t assetType;
    uint8_t isDefault;
    uint8_t pad[3];
    uint32_t randomDataShortCount;
    uint32_t indexCount;
    float framerate;
    float frequency;
    float primedLength;
    float loopEntryTime;
    uint32_t names;
    uint32_t dataByte;
    uint32_t dataShort;
    uint32_t dataInt;
    uint32_t randomDataShort;
    uint32_t randomDataByte;
    uint32_t randomDataInt;
    uint32_t indices;
    uint32_t notify;
    uint32_t deltaPart;
};
struct XAnimDeltaPart_32
{
    uint32_t trans;
    uint32_t quat;
};
struct XAnimPartTrans_32
{
    uint16_t size;
    uint8_t smallTrans;
    uint8_t pad;
};
struct XAnimDeltaPartQuat_32
{
    uint16_t size;
    uint16_t pad;
};
struct XAnimPartTransFrames_32
{
    float mins[3];
    float size[3];
    uint32_t frames;
};
struct XAnimDeltaPartQuatDataFrames_32
{
    uint32_t frames;
};
struct SndBank_32
{
    uint32_t name;
    uint32_t aliasCount;
    uint32_t alias;
    uint32_t aliasIndex;
    uint32_t packHash;
    uint32_t packLocation;
    uint32_t radverbCount;
    uint32_t radverbs;
    uint32_t snapshotCount;
    uint32_t snapshots;
};
struct SndPatch_32
{
    uint32_t name;
    uint32_t elementCount;
    uint32_t elements;
    uint32_t fileCount;
    uint32_t files;
};
struct snd_alias_list_t_32
{
    uint32_t name;
    uint32_t id;
    uint32_t head;
    int count;
    int sequence;
};
struct snd_alias_t_32
{
    uint32_t name;
    uint32_t id;
    uint32_t subtitle;
    uint32_t secondaryname;
    uint32_t soundFile;
    uint32_t flags;
    uint32_t duck;
    uint32_t contextType;
    uint32_t contextValue;
    uint16_t fluxTime;
    uint16_t startDelay;
    uint16_t reverbSend;
    uint16_t centerSend;
    uint16_t volMin;
    uint16_t volMax;
    uint16_t teamVolMod;
    uint16_t pitchMin;
    uint16_t pitchMax;
    uint16_t teamPitchMod;
    uint16_t distMin;
    uint16_t distMax;
    uint16_t distReverbMax;
    uint16_t envelopMin;
    uint16_t envelopMax;
    uint16_t envelopPercentage;
    uint8_t minPriorityThreshold;
    uint8_t maxPriorityThreshold;
    uint8_t probability;
    uint8_t occlusionLevel;
    uint8_t occlusionWetDry;
    uint8_t minPriority;
    uint8_t maxPriority;
    uint8_t pan;
    uint8_t dryCurve;
    uint8_t wetCurve;
    uint8_t dryMinCurve;
    uint8_t wetMinCurve;
    uint8_t limitCount;
    uint8_t entityLimitCount;
    uint8_t snapshotGroup;
    uint8_t pad;
};
struct SoundFile_32
{
    uint32_t u;
    uint8_t type;
    uint8_t exists;
    uint8_t pad[2];
};
struct snd_asset_32
{
    uint32_t version;
    uint32_t frame_count;
    uint32_t frame_rate;
    uint32_t channel_count;
    uint32_t header_size;
    uint32_t block_size;
    uint32_t buffer_size;
    uint32_t format;
    uint32_t channel_flags;
    uint32_t flags;
    uint32_t seek_table_count;
    uint32_t seek_table;
    uint32_t data_size;
    uint32_t data;
};
struct LoadedSound_32
{
    uint32_t name;
    snd_asset_32 sound;
};
struct StreamedSound_32
{
    uint32_t filename;
    uint32_t primeSnd;
};
struct PrimedSound_32
{
    uint32_t name;
    uint32_t buffer;
    uint32_t size;
};
struct FxEffectDef_32
{
    uint32_t name;
    uint8_t flags;
    uint8_t efPriority;
    uint8_t reserved[2];
    int totalSize;
    int msecLoopingLife;
    int elemDefCountLooping;
    int elemDefCountOneShot;
    int elemDefCountEmission;
    uint32_t elemDefsEA;
    float boundingBoxDim[3];
    float boundingSphere[4];
};
struct FxElemDef_32
{
    int flags;
    uint8_t spawn[8];
    float spawnRange[2];
    float fadeInRange[2];
    float fadeOutRange[2];
    float spawnFrustumCullRadius;
    int spawnDelayMsec[2];
    int lifeSpanMsec[2];
    float spawnOrigin[6];
    float spawnOffsetRadius[2];
    float spawnOffsetHeight[2];
    float spawnAngles[6];
    float angularVelocity[6];
    float initialRotation[2];
    uint32_t rotationAxis;
    float gravity[2];
    float reflectionFactor[2];
    uint8_t atlas[8];
    float windInfluence;
    uint8_t elemType;
    uint8_t visualCount;
    uint8_t velIntervalCount;
    uint8_t visStateIntervalCount;
    uint32_t velSamples;
    uint32_t visSamples;
    uint32_t visuals;
    float collMins[3];
    float collMaxs[3];
    uint32_t effectOnImpact;
    uint32_t effectOnDeath;
    uint32_t effectEmitted;
    float emitDist[2];
    float emitDistVariance[2];
    uint32_t effectAttached;
    uint32_t localTrailDef;
    uint8_t sortOrder;
    uint8_t lightingFrac;
    uint8_t unused[2];
    uint16_t alphaFadeTimeMsec;
    uint16_t maxWindStrength;
    uint16_t spawnIntervalAtMaxWind;
    uint16_t lifespanAtMaxWind;
    uint8_t u[8];
    uint32_t spawnSound;
    float billboardPivot[2];
};
struct FxTrailDef_32
{
    int scrollTimeMsec;
    int repeatDist;
    int splitDist;
    int vertCount;
    uint32_t vertsEA;
    int indCount;
    uint32_t indsEA;
};
struct FxElemMarkVisuals_32
{
    uint32_t materials[2];
};
struct DestructibleStage_32
{
    uint16_t showBone;
    uint16_t pad0;
    float breakHealth;
    float maxTime;
    uint32_t flags;
    uint32_t breakEffect;
    uint32_t breakSound;
    uint32_t breakNotify;
    uint32_t loopSound;
    uint32_t spawnModel[3];
    uint32_t physPreset;
};
struct DestructiblePiece_32
{
    DestructibleStage_32 stages[5];
    uint8_t parentPiece;
    uint8_t pad[3];
    float parentDamagePercent;
    float bulletDamageScale;
    float explosiveDamageScale;
    float meleeDamageScale;
    float impactDamageScale;
    float entityDamageTransfer;
    uint32_t physConstraints;
    int health;
    uint32_t damageSound;
    uint32_t burnEffect;
    uint32_t burnSound;
    uint16_t enableLabel;
    uint16_t pad1;
    int hideBones[5];
};
struct DestructibleDef_32
{
    uint32_t name;
    uint32_t model;
    uint32_t pristineModel;
    int numPieces;
    uint32_t pieces;
    int clientOnly;
};
#pragma pack(pop)

static_assert(sizeof(XAnimParts_32) == 104, "Win32 XAnimParts");
static_assert(sizeof(XAnimDeltaPart_32) == 8, "Win32 XAnimDeltaPart");
static_assert(sizeof(XAnimPartTrans_32) == 4, "Win32 XAnimPartTrans header");
static_assert(sizeof(XAnimDeltaPartQuat_32) == 4, "Win32 XAnimDeltaPartQuat header");
static_assert(sizeof(XAnimPartTransFrames_32) == 28, "Win32 XAnimPartTransFrames header");
static_assert(sizeof(XAnimDeltaPartQuatDataFrames_32) == 4, "Win32 XAnimDeltaPartQuatDataFrames header");
static_assert(sizeof(SndBank_32) == 40, "Win32 SndBank");
static_assert(sizeof(SndPatch_32) == 20, "Win32 SndPatch");
static_assert(sizeof(snd_alias_list_t_32) == 20, "Win32 snd_alias_list_t");
static_assert(sizeof(snd_alias_t_32) == 84, "Win32 snd_alias_t");
static_assert(sizeof(SoundFile_32) == 8, "Win32 SoundFile");
static_assert(sizeof(snd_asset_32) == 56, "Win32 snd_asset");
static_assert(sizeof(LoadedSound_32) == 60, "Win32 LoadedSound");
static_assert(sizeof(StreamedSound_32) == 8, "Win32 StreamedSound");
static_assert(sizeof(PrimedSound_32) == 12, "Win32 PrimedSound");
static_assert(sizeof(FxEffectDef_32) == 60, "Win32 FxEffectDef");
static_assert(sizeof(FxElemDef_32) == 292, "Win32 FxElemDef");
static_assert(sizeof(FxTrailDef_32) == 28, "Win32 FxTrailDef");
static_assert(sizeof(FxElemMarkVisuals_32) == 8, "Win32 FxElemMarkVisuals");
static_assert(sizeof(DestructibleStage_32) == 48, "Win32 DestructibleStage");
static_assert(sizeof(DestructiblePiece_32) == 312, "Win32 DestructiblePiece");
static_assert(sizeof(DestructibleDef_32) == 24, "Win32 DestructibleDef");

static void DB_Expand_snd_asset(snd_asset *dst, const snd_asset_32 &s)
{
    dst->version = s.version;
    dst->frame_count = s.frame_count;
    dst->frame_rate = s.frame_rate;
    dst->channel_count = s.channel_count;
    dst->header_size = s.header_size;
    dst->block_size = s.block_size;
    dst->buffer_size = s.buffer_size;
    dst->format = static_cast<snd_asset_format>(s.format);
    dst->channel_flags = static_cast<snd_asset_channel>(s.channel_flags);
    dst->flags = static_cast<snd_asset_flags>(s.flags);
    dst->seek_table_count = s.seek_table_count;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->seek_table), s.seek_table);
    dst->data_size = s.data_size;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->data), s.data);
}

static void DB_Expand_DestructibleStage(DestructibleStage *dst, const DestructibleStage_32 &s)
{
    dst->showBone = s.showBone;
    dst->breakHealth = s.breakHealth;
    dst->maxTime = s.maxTime;
    dst->flags = s.flags;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(const_cast<FxEffectDef **>(&dst->breakEffect)), s.breakEffect);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->breakSound), s.breakSound);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->breakNotify), s.breakNotify);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->loopSound), s.loopSound);
    for (int i = 0; i < 3; ++i)
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->spawnModel[i]), s.spawnModel[i]);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->physPreset), s.physPreset);
}

void DB_ExpandLoad_XAnimParts(XAnimParts *dst)
{
    iassert(dst == reinterpret_cast<XAnimParts *>(DB_GetStreamPos()));
    XAnimParts_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    dst->dataByteCount = disk.dataByteCount;
    dst->dataShortCount = disk.dataShortCount;
    dst->dataIntCount = disk.dataIntCount;
    dst->randomDataByteCount = disk.randomDataByteCount;
    dst->randomDataIntCount = disk.randomDataIntCount;
    dst->numframes = disk.numframes;
    dst->bLoop = disk.bLoop != 0;
    dst->bDelta = disk.bDelta != 0;
    dst->bLeftHandGripIK = disk.bLeftHandGripIK != 0;
    dst->bStreamable = disk.bStreamable != 0;
    dst->streamedFileSize = disk.streamedFileSize;
    memcpy(dst->boneCount, disk.boneCount, sizeof(dst->boneCount));
    dst->notifyCount = disk.notifyCount;
    dst->assetType = disk.assetType;
    dst->isDefault = disk.isDefault != 0;
    dst->randomDataShortCount = disk.randomDataShortCount;
    dst->indexCount = disk.indexCount;
    dst->framerate = disk.framerate;
    dst->frequency = disk.frequency;
    dst->primedLength = disk.primedLength;
    dst->loopEntryTime = disk.loopEntryTime;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->names), disk.names);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->dataByte), disk.dataByte);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->dataShort), disk.dataShort);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->dataInt), disk.dataInt);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->randomDataShort), disk.randomDataShort);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->randomDataByte), disk.randomDataByte);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->randomDataInt), disk.randomDataInt);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->indices.data), disk.indices);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->notify), disk.notify);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->deltaPart), disk.deltaPart);
    DB_Expand_Commit(static_cast<int>(sizeof(XAnimParts)));
}

void DB_ExpandLoad_XAnimDeltaPart(XAnimDeltaPart *dst)
{
    iassert(dst == reinterpret_cast<XAnimDeltaPart *>(DB_GetStreamPos()));
    XAnimDeltaPart_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->trans), disk.trans);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->quat), disk.quat);
    DB_Expand_Commit(static_cast<int>(sizeof(XAnimDeltaPart)));
}

// Win32 header is 4 bytes; native `u` is 8-aligned (union holds a pointer), so the stream advances to offsetof(u).
void DB_ExpandLoad_XAnimPartTrans(XAnimPartTrans *dst)
{
    iassert(dst == reinterpret_cast<XAnimPartTrans *>(DB_GetStreamPos()));
    XAnimPartTrans_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, offsetof(XAnimPartTrans, u));
    dst->size = disk.size;
    dst->smallTrans = disk.smallTrans;
    DB_Expand_Commit(static_cast<int>(offsetof(XAnimPartTrans, u)));
}

void DB_ExpandLoad_XAnimDeltaPartQuat(XAnimDeltaPartQuat *dst)
{
    iassert(dst == reinterpret_cast<XAnimDeltaPartQuat *>(DB_GetStreamPos()));
    XAnimDeltaPartQuat_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, offsetof(XAnimDeltaPartQuat, u));
    dst->size = disk.size;
    DB_Expand_Commit(static_cast<int>(offsetof(XAnimDeltaPartQuat, u)));
}

void DB_ExpandLoad_XAnimPartTransFrames(XAnimPartTransFrames *dst)
{
    iassert(dst == reinterpret_cast<XAnimPartTransFrames *>(DB_GetStreamPos()));
    XAnimPartTransFrames_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    memcpy(dst->mins, disk.mins, sizeof(dst->mins));
    memcpy(dst->size, disk.size, sizeof(dst->size));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->frames._1), disk.frames);
    DB_Expand_Commit(static_cast<int>(offsetof(XAnimPartTransFrames, indices)));
}

void DB_ExpandLoad_XAnimDeltaPartQuatDataFrames(XAnimDeltaPartQuatDataFrames *dst)
{
    iassert(dst == reinterpret_cast<XAnimDeltaPartQuatDataFrames *>(DB_GetStreamPos()));
    XAnimDeltaPartQuatDataFrames_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->frames), disk.frames);
    DB_Expand_Commit(static_cast<int>(offsetof(XAnimDeltaPartQuatDataFrames, indices)));
}

void DB_ExpandLoad_SndBank(SndBank *dst)
{
    iassert(dst == reinterpret_cast<SndBank *>(DB_GetStreamPos()));
    SndBank_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    dst->aliasCount = disk.aliasCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->alias), disk.alias);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->aliasIndex), disk.aliasIndex);
    dst->packHash = disk.packHash;
    dst->packLocation = disk.packLocation;
    dst->radverbCount = disk.radverbCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->radverbs), disk.radverbs);
    dst->snapshotCount = disk.snapshotCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->snapshots), disk.snapshots);
    DB_Expand_Commit(static_cast<int>(sizeof(SndBank)));
}

void DB_ExpandLoad_SndPatch(SndPatch *dst)
{
    iassert(dst == reinterpret_cast<SndPatch *>(DB_GetStreamPos()));
    SndPatch_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    dst->elementCount = disk.elementCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->elements), disk.elements);
    dst->fileCount = disk.fileCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->files), disk.files);
    DB_Expand_Commit(static_cast<int>(sizeof(SndPatch)));
}

void DB_ExpandLoad_snd_alias_t(snd_alias_t *dst)
{
    iassert(dst == reinterpret_cast<snd_alias_t *>(DB_GetStreamPos()));
    snd_alias_t_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    dst->id = disk.id;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->subtitle), disk.subtitle);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->secondaryname), disk.secondaryname);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->soundFile), disk.soundFile);
    dst->flags = disk.flags;
    dst->duck = disk.duck;
    dst->contextType = disk.contextType;
    dst->contextValue = disk.contextValue;
    dst->fluxTime = disk.fluxTime;
    dst->startDelay = disk.startDelay;
    dst->reverbSend = disk.reverbSend;
    dst->centerSend = disk.centerSend;
    dst->volMin = disk.volMin;
    dst->volMax = disk.volMax;
    dst->teamVolMod = disk.teamVolMod;
    dst->pitchMin = disk.pitchMin;
    dst->pitchMax = disk.pitchMax;
    dst->teamPitchMod = disk.teamPitchMod;
    dst->distMin = disk.distMin;
    dst->distMax = disk.distMax;
    dst->distReverbMax = disk.distReverbMax;
    dst->envelopMin = disk.envelopMin;
    dst->envelopMax = disk.envelopMax;
    dst->envelopPercentage = disk.envelopPercentage;
    dst->minPriorityThreshold = disk.minPriorityThreshold;
    dst->maxPriorityThreshold = disk.maxPriorityThreshold;
    dst->probability = disk.probability;
    dst->occlusionLevel = disk.occlusionLevel;
    dst->occlusionWetDry = disk.occlusionWetDry;
    dst->minPriority = disk.minPriority;
    dst->maxPriority = disk.maxPriority;
    dst->pan = disk.pan;
    dst->dryCurve = disk.dryCurve;
    dst->wetCurve = disk.wetCurve;
    dst->dryMinCurve = disk.dryMinCurve;
    dst->wetMinCurve = disk.wetMinCurve;
    dst->limitCount = disk.limitCount;
    dst->entityLimitCount = disk.entityLimitCount;
    dst->snapshotGroup = disk.snapshotGroup;
    DB_Expand_Commit(static_cast<int>(sizeof(snd_alias_t)));
}

void DB_ExpandLoad_snd_alias_tArray(snd_alias_t *dst, int count)
{
    iassert(dst == reinterpret_cast<snd_alias_t *>(DB_GetStreamPos()));
    iassert(count >= 0);
    count = DB_Expand_ClampCount(count, "snd_alias_t_32");
    std::vector<snd_alias_t_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(snd_alias_t_32) * count));
    memset(dst, 0, sizeof(snd_alias_t) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        const snd_alias_t_32 &s = disk[static_cast<size_t>(i)];
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].name), s.name);
        dst[i].id = s.id;
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].subtitle), s.subtitle);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].secondaryname), s.secondaryname);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].soundFile), s.soundFile);
        dst[i].flags = s.flags;
        dst[i].duck = s.duck;
        dst[i].contextType = s.contextType;
        dst[i].contextValue = s.contextValue;
        dst[i].fluxTime = s.fluxTime;
        dst[i].startDelay = s.startDelay;
        dst[i].reverbSend = s.reverbSend;
        dst[i].centerSend = s.centerSend;
        dst[i].volMin = s.volMin;
        dst[i].volMax = s.volMax;
        dst[i].teamVolMod = s.teamVolMod;
        dst[i].pitchMin = s.pitchMin;
        dst[i].pitchMax = s.pitchMax;
        dst[i].teamPitchMod = s.teamPitchMod;
        dst[i].distMin = s.distMin;
        dst[i].distMax = s.distMax;
        dst[i].distReverbMax = s.distReverbMax;
        dst[i].envelopMin = s.envelopMin;
        dst[i].envelopMax = s.envelopMax;
        dst[i].envelopPercentage = s.envelopPercentage;
        dst[i].minPriorityThreshold = s.minPriorityThreshold;
        dst[i].maxPriorityThreshold = s.maxPriorityThreshold;
        dst[i].probability = s.probability;
        dst[i].occlusionLevel = s.occlusionLevel;
        dst[i].occlusionWetDry = s.occlusionWetDry;
        dst[i].minPriority = s.minPriority;
        dst[i].maxPriority = s.maxPriority;
        dst[i].pan = s.pan;
        dst[i].dryCurve = s.dryCurve;
        dst[i].wetCurve = s.wetCurve;
        dst[i].dryMinCurve = s.dryMinCurve;
        dst[i].wetMinCurve = s.wetMinCurve;
        dst[i].limitCount = s.limitCount;
        dst[i].entityLimitCount = s.entityLimitCount;
        dst[i].snapshotGroup = s.snapshotGroup;
    }
    DB_Expand_Commit(static_cast<int>(sizeof(snd_alias_t) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_snd_alias_list_t(snd_alias_list_t *dst)
{
    iassert(dst == reinterpret_cast<snd_alias_list_t *>(DB_GetStreamPos()));
    snd_alias_list_t_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    dst->id = disk.id;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->head), disk.head);
    dst->count = disk.count;
    dst->sequence = disk.sequence;
    DB_Expand_Commit(static_cast<int>(sizeof(snd_alias_list_t)));
}

void DB_ExpandLoad_snd_alias_list_tArray(snd_alias_list_t *dst, int count)
{
    iassert(dst == reinterpret_cast<snd_alias_list_t *>(DB_GetStreamPos()));
    iassert(count >= 0);
    count = DB_Expand_ClampCount(count, "snd_alias_list_t_32");
    std::vector<snd_alias_list_t_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(snd_alias_list_t_32) * count));
    memset(dst, 0, sizeof(snd_alias_list_t) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].name), disk[static_cast<size_t>(i)].name);
        dst[i].id = disk[static_cast<size_t>(i)].id;
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].head), disk[static_cast<size_t>(i)].head);
        dst[i].count = disk[static_cast<size_t>(i)].count;
        dst[i].sequence = disk[static_cast<size_t>(i)].sequence;
    }
    DB_Expand_Commit(static_cast<int>(sizeof(snd_alias_list_t) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_SoundFile(SoundFile *dst)
{
    iassert(dst == reinterpret_cast<SoundFile *>(DB_GetStreamPos()));
    SoundFile_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->u.loadSnd), disk.u);
    dst->type = disk.type;
    dst->exists = disk.exists;
    DB_Expand_Commit(static_cast<int>(sizeof(SoundFile)));
}

void DB_ExpandLoad_SoundFileArray(SoundFile *dst, int count)
{
    iassert(dst == reinterpret_cast<SoundFile *>(DB_GetStreamPos()));
    iassert(count >= 0);
    count = DB_Expand_ClampCount(count, "SoundFile_32");
    std::vector<SoundFile_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(SoundFile_32) * count));
    memset(dst, 0, sizeof(SoundFile) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].u.loadSnd), disk[static_cast<size_t>(i)].u);
        dst[i].type = disk[static_cast<size_t>(i)].type;
        dst[i].exists = disk[static_cast<size_t>(i)].exists;
    }
    DB_Expand_Commit(static_cast<int>(sizeof(SoundFile) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_snd_asset(snd_asset *dst)
{
    iassert(dst == reinterpret_cast<snd_asset *>(DB_GetStreamPos()));
    snd_asset_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_Expand_snd_asset(dst, disk);
    DB_Expand_Commit(static_cast<int>(sizeof(snd_asset)));
}

void DB_ExpandLoad_LoadedSound(LoadedSound *dst)
{
    iassert(dst == reinterpret_cast<LoadedSound *>(DB_GetStreamPos()));
    LoadedSound_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    DB_Expand_snd_asset(&dst->sound, disk.sound);
    DB_Expand_Commit(static_cast<int>(sizeof(LoadedSound)));
}

void DB_ExpandLoad_StreamedSound(StreamedSound *dst)
{
    iassert(dst == reinterpret_cast<StreamedSound *>(DB_GetStreamPos()));
    StreamedSound_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->filename), disk.filename);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->primeSnd), disk.primeSnd);
    DB_Expand_Commit(static_cast<int>(sizeof(StreamedSound)));
}

void DB_ExpandLoad_PrimedSound(PrimedSound *dst)
{
    iassert(dst == reinterpret_cast<PrimedSound *>(DB_GetStreamPos()));
    PrimedSound_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->buffer), disk.buffer);
    dst->size = disk.size;
    DB_Expand_Commit(static_cast<int>(sizeof(PrimedSound)));
}

void DB_ExpandLoad_FxEffectDef(FxEffectDef *dst)
{
    iassert(dst == reinterpret_cast<FxEffectDef *>(DB_GetStreamPos()));
    FxEffectDef_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(const_cast<FxEffectDef *>(dst), 0, sizeof(*dst));
    auto *out = const_cast<FxEffectDef *>(dst);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&out->name), disk.name);
    out->flags = disk.flags;
    out->efPriority = disk.efPriority;
    out->reserved[0] = disk.reserved[0];
    out->reserved[1] = disk.reserved[1];
    out->totalSize = disk.totalSize;
    out->msecLoopingLife = disk.msecLoopingLife;
    out->elemDefCountLooping = disk.elemDefCountLooping;
    out->elemDefCountOneShot = disk.elemDefCountOneShot;
    out->elemDefCountEmission = disk.elemDefCountEmission;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&out->elemDefsEA), disk.elemDefsEA);
    memcpy(out->boundingBoxDim, disk.boundingBoxDim, sizeof(out->boundingBoxDim));
    memcpy(out->boundingSphere, disk.boundingSphere, sizeof(out->boundingSphere));
    DB_Expand_Commit(static_cast<int>(sizeof(FxEffectDef)));
}

static void DB_Expand_FxElemDef(FxElemDef *dst, const FxElemDef_32 &s)
{
    dst->flags = s.flags;
    memcpy(&dst->spawn, s.spawn, sizeof(dst->spawn));
    memcpy(&dst->spawnRange, s.spawnRange, sizeof(dst->spawnRange));
    memcpy(&dst->fadeInRange, s.fadeInRange, sizeof(dst->fadeInRange));
    memcpy(&dst->fadeOutRange, s.fadeOutRange, sizeof(dst->fadeOutRange));
    dst->spawnFrustumCullRadius = s.spawnFrustumCullRadius;
    memcpy(&dst->spawnDelayMsec, s.spawnDelayMsec, sizeof(dst->spawnDelayMsec));
    memcpy(&dst->lifeSpanMsec, s.lifeSpanMsec, sizeof(dst->lifeSpanMsec));
    memcpy(dst->spawnOrigin, s.spawnOrigin, sizeof(dst->spawnOrigin));
    memcpy(&dst->spawnOffsetRadius, s.spawnOffsetRadius, sizeof(dst->spawnOffsetRadius));
    memcpy(&dst->spawnOffsetHeight, s.spawnOffsetHeight, sizeof(dst->spawnOffsetHeight));
    memcpy(dst->spawnAngles, s.spawnAngles, sizeof(dst->spawnAngles));
    memcpy(dst->angularVelocity, s.angularVelocity, sizeof(dst->angularVelocity));
    memcpy(&dst->initialRotation, s.initialRotation, sizeof(dst->initialRotation));
    dst->rotationAxis = s.rotationAxis;
    memcpy(&dst->gravity, s.gravity, sizeof(dst->gravity));
    memcpy(&dst->reflectionFactor, s.reflectionFactor, sizeof(dst->reflectionFactor));
    memcpy(&dst->atlas, s.atlas, sizeof(dst->atlas));
    dst->windInfluence = s.windInfluence;
    dst->elemType = s.elemType;
    dst->visualCount = s.visualCount;
    dst->velIntervalCount = s.velIntervalCount;
    dst->visStateIntervalCount = s.visStateIntervalCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->velSamples), s.velSamples);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->visSamples), s.visSamples);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->visuals.markArray), s.visuals);
    memcpy(dst->collMins, s.collMins, sizeof(dst->collMins));
    memcpy(dst->collMaxs, s.collMaxs, sizeof(dst->collMaxs));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->effectOnImpact.handle), s.effectOnImpact);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->effectOnDeath.handle), s.effectOnDeath);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->effectEmitted.handle), s.effectEmitted);
    memcpy(&dst->emitDist, s.emitDist, sizeof(dst->emitDist));
    memcpy(&dst->emitDistVariance, s.emitDistVariance, sizeof(dst->emitDistVariance));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->effectAttached.handle), s.effectAttached);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->localTrailDef), s.localTrailDef);
    dst->sortOrder = s.sortOrder;
    dst->lightingFrac = s.lightingFrac;
    dst->unused[0] = s.unused[0];
    dst->unused[1] = s.unused[1];
    dst->alphaFadeTimeMsec = s.alphaFadeTimeMsec;
    dst->maxWindStrength = s.maxWindStrength;
    dst->spawnIntervalAtMaxWind = s.spawnIntervalAtMaxWind;
    dst->lifespanAtMaxWind = s.lifespanAtMaxWind;
    memcpy(&dst->u, s.u, sizeof(dst->u));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->spawnSound.spawnSound), s.spawnSound);
    memcpy(dst->billboardPivot, s.billboardPivot, sizeof(dst->billboardPivot));
}

void DB_ExpandLoad_FxElemDef(FxElemDef *dst)
{
    iassert(dst == reinterpret_cast<FxElemDef *>(DB_GetStreamPos()));
    FxElemDef_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(const_cast<FxElemDef *>(dst), 0, sizeof(*dst));
    DB_Expand_FxElemDef(const_cast<FxElemDef *>(dst), disk);
    DB_Expand_Commit(static_cast<int>(sizeof(FxElemDef)));
}

void DB_ExpandLoad_FxElemDefArray(FxElemDef *dst, int count)
{
    iassert(dst == reinterpret_cast<FxElemDef *>(DB_GetStreamPos()));
    iassert(count >= 0);
    count = DB_Expand_ClampCount(count, "FxElemDef_32");
    std::vector<FxElemDef_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(FxElemDef_32) * count));
    memset(const_cast<FxElemDef *>(dst), 0, sizeof(FxElemDef) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
        DB_Expand_FxElemDef(const_cast<FxElemDef *>(&dst[i]), disk[static_cast<size_t>(i)]);
    DB_Expand_Commit(static_cast<int>(sizeof(FxElemDef) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_FxTrailDef(FxTrailDef *dst)
{
    iassert(dst == reinterpret_cast<FxTrailDef *>(DB_GetStreamPos()));
    FxTrailDef_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    dst->scrollTimeMsec = disk.scrollTimeMsec;
    dst->repeatDist = disk.repeatDist;
    dst->splitDist = disk.splitDist;
    dst->vertCount = disk.vertCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->vertsEA), disk.vertsEA);
    dst->indCount = disk.indCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->indsEA), disk.indsEA);
    DB_Expand_Commit(static_cast<int>(sizeof(FxTrailDef)));
}

void DB_ExpandLoad_FxElemMarkVisuals(FxElemMarkVisuals *dst)
{
    iassert(dst == reinterpret_cast<FxElemMarkVisuals *>(DB_GetStreamPos()));
    FxElemMarkVisuals_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->materials[0]), disk.materials[0]);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->materials[1]), disk.materials[1]);
    DB_Expand_Commit(static_cast<int>(sizeof(FxElemMarkVisuals)));
}

void DB_ExpandLoad_FxElemMarkVisualsArray(FxElemMarkVisuals *dst, int count)
{
    iassert(dst == reinterpret_cast<FxElemMarkVisuals *>(DB_GetStreamPos()));
    iassert(count >= 0);
    count = DB_Expand_ClampCount(count, "FxElemMarkVisuals_32");
    std::vector<FxElemMarkVisuals_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(FxElemMarkVisuals_32) * count));
    memset(dst, 0, sizeof(FxElemMarkVisuals) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].materials[0]), disk[static_cast<size_t>(i)].materials[0]);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].materials[1]), disk[static_cast<size_t>(i)].materials[1]);
    }
    DB_Expand_Commit(static_cast<int>(sizeof(FxElemMarkVisuals) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_DestructibleStageArray(DestructibleStage *dst, int count)
{
    iassert(dst == reinterpret_cast<DestructibleStage *>(DB_GetStreamPos()));
    iassert(count >= 0);
    count = DB_Expand_ClampCount(count, "DestructibleStage_32");
    std::vector<DestructibleStage_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(DestructibleStage_32) * count));
    memset(dst, 0, sizeof(DestructibleStage) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
        DB_Expand_DestructibleStage(&dst[i], disk[static_cast<size_t>(i)]);
    DB_Expand_Commit(static_cast<int>(sizeof(DestructibleStage) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_DestructiblePieceArray(DestructiblePiece *dst, int count)
{
    iassert(dst == reinterpret_cast<DestructiblePiece *>(DB_GetStreamPos()));
    iassert(count >= 0);
    count = DB_Expand_ClampCount(count, "DestructiblePiece_32");
    std::vector<DestructiblePiece_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(DestructiblePiece_32) * count));
    memset(dst, 0, sizeof(DestructiblePiece) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        const DestructiblePiece_32 &s = disk[static_cast<size_t>(i)];
        for (int st = 0; st < 5; ++st)
            DB_Expand_DestructibleStage(&dst[i].stages[st], s.stages[st]);
        dst[i].parentPiece = s.parentPiece;
        dst[i].parentDamagePercent = s.parentDamagePercent;
        dst[i].bulletDamageScale = s.bulletDamageScale;
        dst[i].explosiveDamageScale = s.explosiveDamageScale;
        dst[i].meleeDamageScale = s.meleeDamageScale;
        dst[i].impactDamageScale = s.impactDamageScale;
        dst[i].entityDamageTransfer = s.entityDamageTransfer;
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].physConstraints), s.physConstraints);
        dst[i].health = s.health;
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].damageSound), s.damageSound);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(const_cast<FxEffectDef **>(&dst[i].burnEffect)), s.burnEffect);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].burnSound), s.burnSound);
        dst[i].enableLabel = s.enableLabel;
        memcpy(dst[i].hideBones, s.hideBones, sizeof(dst[i].hideBones));
    }
    DB_Expand_Commit(static_cast<int>(sizeof(DestructiblePiece) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_DestructibleDef(DestructibleDef *dst)
{
    iassert(dst == reinterpret_cast<DestructibleDef *>(DB_GetStreamPos()));
    DestructibleDef_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->model), disk.model);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->pristineModel), disk.pristineModel);
    dst->numPieces = disk.numPieces;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->pieces), disk.pieces);
    dst->clientOnly = disk.clientOnly;
    DB_Expand_Commit(static_cast<int>(sizeof(DestructibleDef)));
}



#include <bgame/bg_weapons.h>
#include <ui/ui_shared.h>
#include <universal/com_expressions.h>
#include <sound/snd.h>
#include <database/db_load.h>
#include <gfx_d3d/r_extracam.h>
#include <ddl/ddl_api.h>
#include <bgame/bg_emblems.h>
#include <cgame/cg_effects_load_obj.h>

#pragma pack(push, 1)
struct WeaponVariantDef_32
{
    uint32_t szInternalName;
    int iVariantCount;
    uint32_t weapDef;
    uint32_t szDisplayName;
    uint32_t szXAnims;
    uint32_t szAltWeaponName;
    uint32_t hideTags;
    uint32_t altWeaponIndex;
    int iClipSize;
    int iReloadTime;
    int iReloadEmptyTime;
    int iReloadQuickTime;
    int iReloadQuickEmptyTime;
    int iAdsTransInTime;
    int iAdsTransOutTime;
    int iAltRaiseTime;
    uint32_t szAmmoName;
    int iAmmoIndex;
    uint32_t szClipName;
    int iClipIndex;
    float fAimAssistRangeAds;
    float fAdsSwayHorizScale;
    float fAdsSwayVertScale;
    float fAdsViewKickCenterSpeed;
    float fHipViewKickCenterSpeed;
    float fAdsZoomFov1;
    float fAdsZoomFov2;
    float fAdsZoomFov3;
    float fAdsZoomInFrac;
    float fAdsZoomOutFrac;
    float fOverlayAlphaScale;
    float fOOPosAnimLength[2];
    uint8_t bSilenced;
    uint8_t bDualMag;
    uint8_t bFullMetalJacket;
    uint8_t bHollowPoint;
    uint8_t bRapidFire;
    uint8_t pad[3];
    uint32_t overlayMaterial;
    uint32_t overlayMaterialLowRes;
    uint32_t dpadIcon;
    int dpadIconRatio;
    float ikLeftHandOffset[3];
    float ikLeftHandRotation[3];
    float ikLeftHandProneOffset[3];
    float ikLeftHandProneRotation[3];
    float ikLeftHandUiViewerOffset[3];
    float ikLeftHandUiViewerRotation[3];
};
struct ExpressionStatement_32
{
    uint32_t filename;
    int line;
    int numRpn;
    uint32_t rpn;
};
struct Operand_32
{
    int dataType;
    uint32_t internals;
};
struct expressionRpn_32
{
    int type;
    union
    {
        Operand_32 constant;
        uint32_t cmd;
        int cmdIdx;
    } data;
};
struct windowDef_t_32
{
    uint32_t name;
    float rect[6];
    float rectClient[6];
    uint32_t group;
    uint8_t style;
    uint8_t border;
    uint8_t modal;
    uint8_t frameSides;
    float frameTexSize;
    float frameSize;
    int ownerDraw;
    int ownerDrawFlags;
    float borderSize;
    int staticFlags;
    int dynamicFlags[1];
    int nextTime;
    float foreColor[4];
    float backColor[4];
    float borderColor[4];
    float outlineColor[4];
    float rotation;
    uint32_t background;
};
struct MenuList_32
{
    uint32_t name;
    int menuCount;
    uint32_t menus;
};
struct menuDef_t_32
{
    windowDef_t_32 window;
    uint32_t font;
    int fullScreen;
    int ui3dWindowId;
    int itemCount;
    int fontIndex;
    int cursorItem[1];
    int fadeCycle;
    int priority;
    float fadeClamp;
    float fadeAmount;
    float fadeInAmount;
    float blurRadius;
    int openSlideSpeed;
    int closeSlideSpeed;
    int openSlideDirection;
    int closeSlideDirection;
    float initialRectInfo[6];
    int openFadingTime;
    int closeFadingTime;
    int fadeTimeCounter;
    int slideTimeCounter;
    uint32_t onEvent;
    uint32_t onKey;
    ExpressionStatement_32 visibleExp;
    uint8_t pad_after_vis[4];
    uint64_t showBits;
    uint64_t hideBits;
    uint32_t allowedBinding;
    uint32_t soundName;
    int imageTrack;
    int control;
    float focusColor[4];
    float disableColor[4];
    ExpressionStatement_32 rectXExp;
    ExpressionStatement_32 rectYExp;
    uint32_t items;
    uint8_t pad_end[4];
};
struct itemDef_s_32
{
    windowDef_t_32 window;
    int type;
    int dataType;
    int imageTrack;
    uint32_t dvar;
    uint32_t dvarTest;
    uint32_t enableDvar;
    int dvarFlags;
    uint32_t typeData;
    uint32_t parent;
    uint32_t rectExpData;
    ExpressionStatement_32 visibleExp;
    uint8_t pad_after_vis[4];
    uint64_t showBits;
    uint64_t hideBits;
    ExpressionStatement_32 forecolorAExp;
    int ui3dWindowId;
    uint32_t onEvent;
    uint32_t animInfo;
    uint8_t pad_end[4];
};
struct MenuCell_32
{
    int type;
    int maxChars;
    uint32_t stringValue;
};
struct MenuRow_32
{
    uint32_t cells;
    uint32_t eventName;
    uint32_t onFocusEventName;
    uint8_t disableArg;
    uint8_t pad[3];
    int status;
    int name;
};
struct ScriptCondition_32
{
    uint8_t fireOnTrue;
    uint8_t pad[3];
    int constructID;
    int blockID;
    uint32_t next;
};
struct GenericEventHandler_32
{
    uint32_t name;
    uint32_t eventScript;
    uint32_t next;
};
struct ItemKeyHandler_32
{
    int key;
    uint32_t keyScript;
    uint32_t next;
};
struct GenericEventScript_32
{
    uint32_t prerequisites;
    ExpressionStatement_32 condition;
    int type;
    uint8_t fireOnTrue;
    uint8_t pad[3];
    uint32_t action;
    int blockID;
    int constructID;
    uint32_t next;
};
struct SndDriverGlobals_32
{
    uint32_t name;
    uint32_t groupCount;
    uint32_t groups;
    uint32_t curveCount;
    uint32_t curves;
    uint32_t panCount;
    uint32_t pans;
    uint32_t snapshotGroupCount;
    uint32_t snapshotGroups;
    uint32_t contextCount;
    uint32_t contexts;
    uint32_t masterCount;
    uint32_t masters;
};
struct PackIndex_32
{
    uint32_t name;
    uint32_t magic;
    uint32_t timestamp;
    uint32_t count;
    uint32_t alignment;
    uint32_t data_start;
    uint32_t entries;
};
struct XGlobals_32
{
    uint32_t name;
    int xanimStreamBufferSize;
    int cinematicMaxWidth;
    int cinematicMaxHeight;
    int extracamResolution;
    int gumpReserve;
    float screenClearColor[4];
};
struct ddlMemberDef_t_32
{
    uint32_t name;
    int size;
    int offset;
    int type;
    int externalIndex;
    uint32_t min;
    uint32_t max;
    uint32_t serverDelta;
    uint32_t clientDelta;
    int arraySize;
    int enumIndex;
    int permission;
};
struct ddlStructDef_t_32
{
    uint32_t name;
    int size;
    int memberCount;
    uint32_t members;
};
struct ddlEnumDef_t_32
{
    uint32_t name;
    int memberCount;
    uint32_t members;
};
struct ddlDef_t_32
{
    int version;
    int size;
    uint32_t structList;
    int structCount;
    uint32_t enumList;
    int enumCount;
    uint32_t next;
};
struct ddlRoot_t_32
{
    uint32_t name;
    uint32_t ddlDef;
};
struct EmblemCategory_32
{
    uint32_t name;
    uint32_t description;
};
struct EmblemIcon_32
{
    uint32_t image;
    uint32_t description;
    float outlineSize;
    int defaultColor;
    int cost;
    int unlockLevel;
    int unlockPLevel;
    int unclassifyAt;
    int sortKey;
    uint32_t category;
};
struct EmblemBackground_32
{
    uint32_t material;
    uint32_t description;
    int cost;
    int unlockLevel;
    int unlockPLevel;
    int unclassifyAt;
};
struct EmblemSet_32
{
    int colorCount;
    int layerCount;
    uint32_t layers;
    int categoryCount;
    uint32_t categories;
    int iconCount;
    uint32_t icons;
    int backgroundCount;
    uint32_t backgrounds;
    int backgroundLookupCount;
    uint32_t backgroundLookup;
};
struct FxImpactTable_32
{
    uint32_t name;
    uint32_t table;
};
struct FxImpactEntry_32
{
    uint32_t nonflesh[31];
    uint32_t flesh[4];
};
#pragma pack(pop)

static_assert(sizeof(WeaponVariantDef_32) == 228, "Win32 WeaponVariantDef");
static_assert(sizeof(ExpressionStatement_32) == 16, "Win32 ExpressionStatement");
static_assert(sizeof(Operand_32) == 8, "Win32 Operand");
static_assert(sizeof(expressionRpn_32) == 12, "Win32 expressionRpn");
static_assert(sizeof(windowDef_t_32) == 164, "Win32 windowDef_t");
static_assert(sizeof(MenuList_32) == 12, "Win32 MenuList");
static_assert(sizeof(menuDef_t_32) == 400, "Win32 menuDef_t");
static_assert(sizeof(itemDef_s_32) == 272, "Win32 itemDef_s");
static_assert(sizeof(MenuCell_32) == 12, "Win32 MenuCell");
static_assert(sizeof(MenuRow_32) == 24, "Win32 MenuRow");
static_assert(sizeof(ScriptCondition_32) == 16, "Win32 ScriptCondition");
static_assert(sizeof(GenericEventHandler_32) == 12, "Win32 GenericEventHandler");
static_assert(sizeof(ItemKeyHandler_32) == 12, "Win32 ItemKeyHandler");
static_assert(sizeof(GenericEventScript_32) == 44, "Win32 GenericEventScript");
static_assert(sizeof(SndDriverGlobals_32) == 52, "Win32 SndDriverGlobals");
static_assert(sizeof(PackIndex_32) == 28, "Win32 PackIndex");
static_assert(sizeof(XGlobals_32) == 40, "Win32 XGlobals");
static_assert(sizeof(ddlMemberDef_t_32) == 48, "Win32 ddlMemberDef");
static_assert(sizeof(ddlStructDef_t_32) == 16, "Win32 ddlStructDef");
static_assert(sizeof(ddlEnumDef_t_32) == 12, "Win32 ddlEnumDef");
static_assert(sizeof(ddlDef_t_32) == 28, "Win32 ddlDef");
static_assert(sizeof(ddlRoot_t_32) == 8, "Win32 ddlRoot");
static_assert(sizeof(EmblemCategory_32) == 8, "Win32 EmblemCategory");
static_assert(sizeof(EmblemIcon_32) == 40, "Win32 EmblemIcon");
static_assert(sizeof(EmblemBackground_32) == 24, "Win32 EmblemBackground");
static_assert(sizeof(EmblemSet_32) == 44, "Win32 EmblemSet");
static_assert(sizeof(FxImpactTable_32) == 8, "Win32 FxImpactTable");
static_assert(sizeof(FxImpactEntry_32) == 140, "Win32 FxImpactEntry");

static void DB_Expand_ExpressionStatement(ExpressionStatement *dst, const ExpressionStatement_32 &s)
{
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->filename), s.filename);
    dst->line = s.line;
    dst->numRpn = s.numRpn;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->rpn), s.rpn);
}

static void DB_Expand_windowDef(windowDef_t *dst, const windowDef_t_32 &s)
{
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), s.name);
    memcpy(&dst->rect, s.rect, sizeof(dst->rect));
    memcpy(&dst->rectClient, s.rectClient, sizeof(dst->rectClient));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->group), s.group);
    dst->style = s.style;
    dst->border = s.border;
    dst->modal = s.modal;
    dst->frameSides = s.frameSides;
    dst->frameTexSize = s.frameTexSize;
    dst->frameSize = s.frameSize;
    dst->ownerDraw = s.ownerDraw;
    dst->ownerDrawFlags = s.ownerDrawFlags;
    dst->borderSize = s.borderSize;
    dst->staticFlags = s.staticFlags;
    dst->dynamicFlags[0] = s.dynamicFlags[0];
    dst->nextTime = s.nextTime;
    memcpy(dst->foreColor, s.foreColor, sizeof(dst->foreColor));
    memcpy(dst->backColor, s.backColor, sizeof(dst->backColor));
    memcpy(dst->borderColor, s.borderColor, sizeof(dst->borderColor));
    memcpy(dst->outlineColor, s.outlineColor, sizeof(dst->outlineColor));
    dst->rotation = s.rotation;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->background), s.background);
}

void DB_ExpandLoad_WeaponVariantDef(WeaponVariantDef *dst)
{
    iassert(dst == reinterpret_cast<WeaponVariantDef *>(DB_GetStreamPos()));
    WeaponVariantDef_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->szInternalName), disk.szInternalName);
    dst->iVariantCount = disk.iVariantCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->weapDef), disk.weapDef);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->szDisplayName), disk.szDisplayName);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->szXAnims), disk.szXAnims);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->szAltWeaponName), disk.szAltWeaponName);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->hideTags), disk.hideTags);
    dst->altWeaponIndex = disk.altWeaponIndex;
    dst->iClipSize = disk.iClipSize;
    dst->iReloadTime = disk.iReloadTime;
    dst->iReloadEmptyTime = disk.iReloadEmptyTime;
    dst->iReloadQuickTime = disk.iReloadQuickTime;
    dst->iReloadQuickEmptyTime = disk.iReloadQuickEmptyTime;
    dst->iAdsTransInTime = disk.iAdsTransInTime;
    dst->iAdsTransOutTime = disk.iAdsTransOutTime;
    dst->iAltRaiseTime = disk.iAltRaiseTime;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->szAmmoName), disk.szAmmoName);
    dst->iAmmoIndex = disk.iAmmoIndex;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->szClipName), disk.szClipName);
    dst->iClipIndex = disk.iClipIndex;
    dst->fAimAssistRangeAds = disk.fAimAssistRangeAds;
    dst->fAdsSwayHorizScale = disk.fAdsSwayHorizScale;
    dst->fAdsSwayVertScale = disk.fAdsSwayVertScale;
    dst->fAdsViewKickCenterSpeed = disk.fAdsViewKickCenterSpeed;
    dst->fHipViewKickCenterSpeed = disk.fHipViewKickCenterSpeed;
    dst->fAdsZoomFov1 = disk.fAdsZoomFov1;
    dst->fAdsZoomFov2 = disk.fAdsZoomFov2;
    dst->fAdsZoomFov3 = disk.fAdsZoomFov3;
    dst->fAdsZoomInFrac = disk.fAdsZoomInFrac;
    dst->fAdsZoomOutFrac = disk.fAdsZoomOutFrac;
    dst->fOverlayAlphaScale = disk.fOverlayAlphaScale;
    memcpy(dst->fOOPosAnimLength, disk.fOOPosAnimLength, sizeof(dst->fOOPosAnimLength));
    dst->bSilenced = disk.bSilenced != 0;
    dst->bDualMag = disk.bDualMag != 0;
    dst->bFullMetalJacket = disk.bFullMetalJacket != 0;
    dst->bHollowPoint = disk.bHollowPoint != 0;
    dst->bRapidFire = disk.bRapidFire != 0;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->overlayMaterial), disk.overlayMaterial);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->overlayMaterialLowRes), disk.overlayMaterialLowRes);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->dpadIcon), disk.dpadIcon);
    dst->dpadIconRatio = static_cast<weaponIconRatioType_t>(disk.dpadIconRatio);
    memcpy(dst->ikLeftHandOffset, disk.ikLeftHandOffset, sizeof(dst->ikLeftHandOffset));
    memcpy(dst->ikLeftHandRotation, disk.ikLeftHandRotation, sizeof(dst->ikLeftHandRotation));
    memcpy(dst->ikLeftHandProneOffset, disk.ikLeftHandProneOffset, sizeof(dst->ikLeftHandProneOffset));
    memcpy(dst->ikLeftHandProneRotation, disk.ikLeftHandProneRotation, sizeof(dst->ikLeftHandProneRotation));
    memcpy(dst->ikLeftHandUiViewerOffset, disk.ikLeftHandUiViewerOffset, sizeof(dst->ikLeftHandUiViewerOffset));
    memcpy(dst->ikLeftHandUiViewerRotation, disk.ikLeftHandUiViewerRotation, sizeof(dst->ikLeftHandUiViewerRotation));
    DB_Expand_Commit(static_cast<int>(sizeof(WeaponVariantDef)));
}

void DB_ExpandLoad_ExpressionStatement(ExpressionStatement *dst)
{
    iassert(dst == reinterpret_cast<ExpressionStatement *>(DB_GetStreamPos()));
    ExpressionStatement_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_Expand_ExpressionStatement(dst, disk);
    DB_Expand_Commit(static_cast<int>(sizeof(ExpressionStatement)));
}

void DB_ExpandLoad_expressionRpnArray(expressionRpn *dst, int count)
{
    iassert(reinterpret_cast<unsigned __int8 *>(dst) == DB_GetStreamPos());
    // Corrupt / negative numRpn must not size_t-wrap into std::vector (throws 0xE06D7363).
    if (count < 0 || count > 65536)
        count = 0;

    // expressionRpn / Operand have non-trivial unions; write via raw bytes.
    // x64 layout: type@0 (int), pad, data@8 (16-byte union). Operand: dataType@0, pad, internals@8.
    static_assert(sizeof(expressionRpn) == 24, "x64 expressionRpn");
    if (count)
    {
        expressionRpn_32 *disk = static_cast<expressionRpn_32 *>(
            malloc(sizeof(expressionRpn_32) * static_cast<size_t>(count)));
        if (!disk)
        {
            DB_Expand_Commit(0);
            return;
        }
        DB_Expand_ReadFile(disk, static_cast<int>(sizeof(expressionRpn_32) * count));
        memset(dst, 0, sizeof(expressionRpn) * static_cast<size_t>(count));
        for (int i = 0; i < count; ++i)
        {
            const expressionRpn_32 &s = disk[static_cast<size_t>(i)];
            unsigned __int8 *raw = reinterpret_cast<unsigned __int8 *>(&dst[i]);
            memcpy(raw + 0, &s.type, sizeof(int));
            if (s.type == 0)
            {
                memcpy(raw + 8, &s.data.constant.dataType, sizeof(int));
                uintptr_t internals = (s.data.constant.dataType == VAL_STRING)
                                         ? DB_ExpandPtr32(s.data.constant.internals)
                                         : static_cast<uintptr_t>(s.data.constant.internals);
                memcpy(raw + 16, &internals, sizeof(uintptr_t));
            }
            else if (s.type == 1)
            {
                int cmdIdx = s.data.cmdIdx;
                memcpy(raw + 8, &cmdIdx, sizeof(int));
            }
            else
            {
                uintptr_t cmd = DB_ExpandPtr32(s.data.cmd);
                memcpy(raw + 8, &cmd, sizeof(uintptr_t));
            }
        }
        free(disk);
    }
    DB_Expand_Commit(static_cast<int>(sizeof(expressionRpn) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_windowDef_t(windowDef_t *dst)
{
    iassert(dst == reinterpret_cast<windowDef_t *>(DB_GetStreamPos()));
    windowDef_t_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_Expand_windowDef(dst, disk);
    DB_Expand_Commit(static_cast<int>(sizeof(windowDef_t)));
}

void DB_ExpandLoad_MenuList(MenuList *dst)
{
    static_assert(sizeof(MenuList) == 24, "x64 MenuList");
    static_assert(offsetof(MenuList, menuCount) == 8, "MenuList.menuCount");
    static_assert(offsetof(MenuList, menus) == 16, "MenuList.menus");
    iassert(dst == reinterpret_cast<MenuList *>(DB_GetStreamPos()));
    MenuList_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    dst->menuCount = disk.menuCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->menus), disk.menus);
    {
    }
    DB_Expand_Commit(static_cast<int>(sizeof(MenuList)));
}

void DB_ExpandLoad_menuDef_t(menuDef_t *dst)
{
    iassert(dst == reinterpret_cast<menuDef_t *>(DB_GetStreamPos()));
    menuDef_t_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_Expand_windowDef(&dst->window, disk.window);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->font), disk.font);
    dst->fullScreen = disk.fullScreen;
    dst->ui3dWindowId = disk.ui3dWindowId;
    dst->itemCount = disk.itemCount;
    if (disk.itemCount < 0 || disk.itemCount > 4096)
    {
        dst->itemCount = 0;
    }
    dst->fontIndex = disk.fontIndex;
    dst->cursorItem[0] = disk.cursorItem[0];
    dst->fadeCycle = disk.fadeCycle;
    dst->priority = disk.priority;
    dst->fadeClamp = disk.fadeClamp;
    dst->fadeAmount = disk.fadeAmount;
    dst->fadeInAmount = disk.fadeInAmount;
    dst->blurRadius = disk.blurRadius;
    dst->openSlideSpeed = disk.openSlideSpeed;
    dst->closeSlideSpeed = disk.closeSlideSpeed;
    dst->openSlideDirection = disk.openSlideDirection;
    dst->closeSlideDirection = disk.closeSlideDirection;
    memcpy(&dst->initialRectInfo, disk.initialRectInfo, sizeof(dst->initialRectInfo));
    dst->openFadingTime = disk.openFadingTime;
    dst->closeFadingTime = disk.closeFadingTime;
    dst->fadeTimeCounter = disk.fadeTimeCounter;
    dst->slideTimeCounter = disk.slideTimeCounter;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->onEvent), disk.onEvent);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->onKey), disk.onKey);
    DB_Expand_ExpressionStatement(&dst->visibleExp, disk.visibleExp);
    dst->showBits = disk.showBits;
    dst->hideBits = disk.hideBits;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->allowedBinding), disk.allowedBinding);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->soundName), disk.soundName);
    dst->imageTrack = disk.imageTrack;
    dst->control = disk.control;
    memcpy(dst->focusColor, disk.focusColor, sizeof(dst->focusColor));
    memcpy(dst->disableColor, disk.disableColor, sizeof(dst->disableColor));
    DB_Expand_ExpressionStatement(&dst->rectXExp, disk.rectXExp);
    DB_Expand_ExpressionStatement(&dst->rectYExp, disk.rectYExp);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->items), disk.items);
    DB_Expand_Commit(static_cast<int>(sizeof(menuDef_t)));
}

void DB_ExpandLoad_itemDef_t(itemDef_s *dst)
{
    iassert(dst == reinterpret_cast<itemDef_s *>(DB_GetStreamPos()));
    itemDef_s_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_Expand_windowDef(&dst->window, disk.window);
    dst->type = disk.type;
    dst->dataType = disk.dataType;
    {
        static int s_itemLog;
        if (s_itemLog < 16)
        {
            ++s_itemLog;
        }
    }
    dst->imageTrack = disk.imageTrack;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->dvar), disk.dvar);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->dvarTest), disk.dvarTest);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->enableDvar), disk.enableDvar);
    dst->dvarFlags = disk.dvarFlags;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->typeData.data), disk.typeData);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->parent), disk.parent);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->rectExpData), disk.rectExpData);
    DB_Expand_ExpressionStatement(&dst->visibleExp, disk.visibleExp);
    dst->showBits = disk.showBits;
    dst->hideBits = disk.hideBits;
    DB_Expand_ExpressionStatement(&dst->forecolorAExp, disk.forecolorAExp);
    dst->ui3dWindowId = disk.ui3dWindowId;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->onEvent), disk.onEvent);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->animInfo), disk.animInfo);
    DB_Expand_Commit(static_cast<int>(sizeof(itemDef_s)));
}

void DB_ExpandLoad_MenuCell(MenuCell *dst)
{
    iassert(dst == reinterpret_cast<MenuCell *>(DB_GetStreamPos()));
    MenuCell_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    dst->type = disk.type;
    dst->maxChars = disk.maxChars;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->stringValue), disk.stringValue);
    DB_Expand_Commit(static_cast<int>(sizeof(MenuCell)));
}

void DB_ExpandLoad_MenuCellArray(MenuCell *dst, int count)
{
    iassert(dst == reinterpret_cast<MenuCell *>(DB_GetStreamPos()));
    iassert(count >= 0);
    count = DB_Expand_ClampCount(count, "MenuCell_32");
    std::vector<MenuCell_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(MenuCell_32) * count));
    memset(dst, 0, sizeof(MenuCell) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        dst[i].type = disk[static_cast<size_t>(i)].type;
        dst[i].maxChars = disk[static_cast<size_t>(i)].maxChars;
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].stringValue), disk[static_cast<size_t>(i)].stringValue);
    }
    DB_Expand_Commit(static_cast<int>(sizeof(MenuCell) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_MenuRow(MenuRow *dst)
{
    iassert(dst == reinterpret_cast<MenuRow *>(DB_GetStreamPos()));
    MenuRow_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->cells), disk.cells);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->eventName), disk.eventName);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->onFocusEventName), disk.onFocusEventName);
    dst->disableArg = disk.disableArg != 0;
    dst->status = disk.status;
    dst->name = disk.name;
    DB_Expand_Commit(static_cast<int>(sizeof(MenuRow)));
}

void DB_ExpandLoad_MenuRowArray(MenuRow *dst, int count)
{
    iassert(dst == reinterpret_cast<MenuRow *>(DB_GetStreamPos()));
    iassert(count >= 0);
    count = DB_Expand_ClampCount(count, "MenuRow_32");
    std::vector<MenuRow_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(MenuRow_32) * count));
    memset(dst, 0, sizeof(MenuRow) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].cells), disk[static_cast<size_t>(i)].cells);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].eventName), disk[static_cast<size_t>(i)].eventName);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].onFocusEventName), disk[static_cast<size_t>(i)].onFocusEventName);
        dst[i].disableArg = disk[static_cast<size_t>(i)].disableArg != 0;
        dst[i].status = disk[static_cast<size_t>(i)].status;
        dst[i].name = disk[static_cast<size_t>(i)].name;
    }
    DB_Expand_Commit(static_cast<int>(sizeof(MenuRow) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_ScriptCondition(ScriptCondition *dst)
{
    iassert(dst == reinterpret_cast<ScriptCondition *>(DB_GetStreamPos()));
    ScriptCondition_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    dst->fireOnTrue = disk.fireOnTrue != 0;
    dst->constructID = disk.constructID;
    dst->blockID = disk.blockID;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->next), disk.next);
    DB_Expand_Commit(static_cast<int>(sizeof(ScriptCondition)));
}

void DB_ExpandLoad_GenericEventHandler(GenericEventHandler *dst)
{
    iassert(dst == reinterpret_cast<GenericEventHandler *>(DB_GetStreamPos()));
    GenericEventHandler_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->eventScript), disk.eventScript);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->next), disk.next);
    DB_Expand_Commit(static_cast<int>(sizeof(GenericEventHandler)));
}

void DB_ExpandLoad_ItemKeyHandler(ItemKeyHandler *dst)
{
    iassert(dst == reinterpret_cast<ItemKeyHandler *>(DB_GetStreamPos()));
    ItemKeyHandler_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    dst->key = disk.key;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->keyScript), disk.keyScript);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->next), disk.next);
    DB_Expand_Commit(static_cast<int>(sizeof(ItemKeyHandler)));
}

void DB_ExpandLoad_GenericEventScript(GenericEventScript *dst)
{
    iassert(dst == reinterpret_cast<GenericEventScript *>(DB_GetStreamPos()));
    GenericEventScript_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->prerequisites), disk.prerequisites);
    DB_Expand_ExpressionStatement(&dst->condition, disk.condition);
    dst->type = disk.type;
    dst->fireOnTrue = disk.fireOnTrue != 0;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->action), disk.action);
    dst->blockID = disk.blockID;
    dst->constructID = disk.constructID;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->next), disk.next);
    DB_Expand_Commit(static_cast<int>(sizeof(GenericEventScript)));
}

void DB_ExpandLoad_SndDriverGlobals(SndDriverGlobals *dst)
{
    iassert(dst == reinterpret_cast<SndDriverGlobals *>(DB_GetStreamPos()));
    SndDriverGlobals_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    dst->groupCount = disk.groupCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->groups), disk.groups);
    dst->curveCount = disk.curveCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->curves), disk.curves);
    dst->panCount = disk.panCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->pans), disk.pans);
    dst->snapshotGroupCount = disk.snapshotGroupCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->snapshotGroups), disk.snapshotGroups);
    dst->contextCount = disk.contextCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->contexts), disk.contexts);
    dst->masterCount = disk.masterCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->masters), disk.masters);
    DB_Expand_Commit(static_cast<int>(sizeof(SndDriverGlobals)));
}

void DB_ExpandLoad_PackIndex(PackIndex *dst)
{
    iassert(dst == reinterpret_cast<PackIndex *>(DB_GetStreamPos()));
    PackIndex_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    dst->header.magic = disk.magic;
    dst->header.timestamp = disk.timestamp;
    dst->header.count = disk.count;
    dst->header.alignment = disk.alignment;
    dst->header.data_start = disk.data_start;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->entries), disk.entries);
    DB_Expand_Commit(static_cast<int>(sizeof(PackIndex)));
}

void DB_ExpandLoad_XGlobals(XGlobals *dst)
{
    iassert(dst == reinterpret_cast<XGlobals *>(DB_GetStreamPos()));
    XGlobals_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    dst->xanimStreamBufferSize = disk.xanimStreamBufferSize;
    dst->cinematicMaxWidth = disk.cinematicMaxWidth;
    dst->cinematicMaxHeight = disk.cinematicMaxHeight;
    dst->extracamResolution = disk.extracamResolution;
    dst->gumpReserve = disk.gumpReserve;
    memcpy(dst->screenClearColor, disk.screenClearColor, sizeof(dst->screenClearColor));
    DB_Expand_Commit(static_cast<int>(sizeof(XGlobals)));
}

void DB_ExpandLoad_ddlMemberDef_tArray(ddlMemberDef_t *dst, int count)
{
    iassert(dst == reinterpret_cast<ddlMemberDef_t *>(DB_GetStreamPos()));
    iassert(count >= 0);
    count = DB_Expand_ClampCount(count, "ddlMemberDef_t_32");
    std::vector<ddlMemberDef_t_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(ddlMemberDef_t_32) * count));
    memset(dst, 0, sizeof(ddlMemberDef_t) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        const ddlMemberDef_t_32 &s = disk[static_cast<size_t>(i)];
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].name), s.name);
        dst[i].size = s.size;
        dst[i].offset = s.offset;
        dst[i].type = s.type;
        dst[i].externalIndex = s.externalIndex;
        dst[i].min = s.min;
        dst[i].max = s.max;
        dst[i].serverDelta = s.serverDelta;
        dst[i].clientDelta = s.clientDelta;
        dst[i].arraySize = s.arraySize;
        dst[i].enumIndex = s.enumIndex;
        dst[i].permission = s.permission;
    }
    DB_Expand_Commit(static_cast<int>(sizeof(ddlMemberDef_t) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_ddlStructDef_t(ddlStructDef_t *dst)
{
    iassert(dst == reinterpret_cast<ddlStructDef_t *>(DB_GetStreamPos()));
    ddlStructDef_t_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    dst->size = disk.size;
    dst->memberCount = disk.memberCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->members), disk.members);
    DB_Expand_Commit(static_cast<int>(sizeof(ddlStructDef_t)));
}

void DB_ExpandLoad_ddlStructDef_tArray(ddlStructDef_t *dst, int count)
{
    iassert(dst == reinterpret_cast<ddlStructDef_t *>(DB_GetStreamPos()));
    iassert(count >= 0);
    count = DB_Expand_ClampCount(count, "ddlStructDef_t_32");
    std::vector<ddlStructDef_t_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(ddlStructDef_t_32) * count));
    memset(dst, 0, sizeof(ddlStructDef_t) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].name), disk[static_cast<size_t>(i)].name);
        dst[i].size = disk[static_cast<size_t>(i)].size;
        dst[i].memberCount = disk[static_cast<size_t>(i)].memberCount;
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].members), disk[static_cast<size_t>(i)].members);
    }
    DB_Expand_Commit(static_cast<int>(sizeof(ddlStructDef_t) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_ddlEnumDef_t(ddlEnumDef_t *dst)
{
    iassert(dst == reinterpret_cast<ddlEnumDef_t *>(DB_GetStreamPos()));
    ddlEnumDef_t_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    dst->memberCount = disk.memberCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->members), disk.members);
    DB_Expand_Commit(static_cast<int>(sizeof(ddlEnumDef_t)));
}

void DB_ExpandLoad_ddlEnumDef_tArray(ddlEnumDef_t *dst, int count)
{
    iassert(dst == reinterpret_cast<ddlEnumDef_t *>(DB_GetStreamPos()));
    iassert(count >= 0);
    count = DB_Expand_ClampCount(count, "ddlEnumDef_t_32");
    std::vector<ddlEnumDef_t_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(ddlEnumDef_t_32) * count));
    memset(dst, 0, sizeof(ddlEnumDef_t) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].name), disk[static_cast<size_t>(i)].name);
        dst[i].memberCount = disk[static_cast<size_t>(i)].memberCount;
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].members), disk[static_cast<size_t>(i)].members);
    }
    DB_Expand_Commit(static_cast<int>(sizeof(ddlEnumDef_t) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_ddlDef_t(ddlDef_t *dst)
{
    iassert(dst == reinterpret_cast<ddlDef_t *>(DB_GetStreamPos()));
    ddlDef_t_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    dst->version = disk.version;
    dst->size = disk.size;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->structList), disk.structList);
    dst->structCount = disk.structCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->enumList), disk.enumList);
    dst->enumCount = disk.enumCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->next), disk.next);
    DB_Expand_Commit(static_cast<int>(sizeof(ddlDef_t)));
}

void DB_ExpandLoad_ddlRoot_t(ddlRoot_t *dst)
{
    iassert(dst == reinterpret_cast<ddlRoot_t *>(DB_GetStreamPos()));
    ddlRoot_t_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->ddlDef), disk.ddlDef);
    DB_Expand_Commit(static_cast<int>(sizeof(ddlRoot_t)));
}

void DB_ExpandLoad_EmblemCategory(EmblemCategory *dst)
{
    iassert(dst == reinterpret_cast<EmblemCategory *>(DB_GetStreamPos()));
    EmblemCategory_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->description), disk.description);
    DB_Expand_Commit(static_cast<int>(sizeof(EmblemCategory)));
}

void DB_ExpandLoad_EmblemCategoryArray(EmblemCategory *dst, int count)
{
    iassert(dst == reinterpret_cast<EmblemCategory *>(DB_GetStreamPos()));
    iassert(count >= 0);
    count = DB_Expand_ClampCount(count, "EmblemCategory_32");
    std::vector<EmblemCategory_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(EmblemCategory_32) * count));
    memset(dst, 0, sizeof(EmblemCategory) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].name), disk[static_cast<size_t>(i)].name);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].description), disk[static_cast<size_t>(i)].description);
    }
    DB_Expand_Commit(static_cast<int>(sizeof(EmblemCategory) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_EmblemIcon(EmblemIcon *dst)
{
    iassert(dst == reinterpret_cast<EmblemIcon *>(DB_GetStreamPos()));
    EmblemIcon_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->image), disk.image);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->description), disk.description);
    dst->outlineSize = disk.outlineSize;
    dst->defaultColor = disk.defaultColor;
    dst->cost = disk.cost;
    dst->unlockLevel = disk.unlockLevel;
    dst->unlockPLevel = disk.unlockPLevel;
    dst->unclassifyAt = disk.unclassifyAt;
    dst->sortKey = disk.sortKey;
    dst->category = disk.category;
    DB_Expand_Commit(static_cast<int>(sizeof(EmblemIcon)));
}

void DB_ExpandLoad_EmblemIconArray(EmblemIcon *dst, int count)
{
    iassert(dst == reinterpret_cast<EmblemIcon *>(DB_GetStreamPos()));
    iassert(count >= 0);
    count = DB_Expand_ClampCount(count, "EmblemIcon_32");
    std::vector<EmblemIcon_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(EmblemIcon_32) * count));
    memset(dst, 0, sizeof(EmblemIcon) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        const EmblemIcon_32 &s = disk[static_cast<size_t>(i)];
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].image), s.image);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].description), s.description);
        dst[i].outlineSize = s.outlineSize;
        dst[i].defaultColor = s.defaultColor;
        dst[i].cost = s.cost;
        dst[i].unlockLevel = s.unlockLevel;
        dst[i].unlockPLevel = s.unlockPLevel;
        dst[i].unclassifyAt = s.unclassifyAt;
        dst[i].sortKey = s.sortKey;
        dst[i].category = s.category;
    }
    DB_Expand_Commit(static_cast<int>(sizeof(EmblemIcon) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_EmblemBackground(EmblemBackground *dst)
{
    iassert(dst == reinterpret_cast<EmblemBackground *>(DB_GetStreamPos()));
    EmblemBackground_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->material), disk.material);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->description), disk.description);
    dst->cost = disk.cost;
    dst->unlockLevel = disk.unlockLevel;
    dst->unlockPLevel = disk.unlockPLevel;
    dst->unclassifyAt = disk.unclassifyAt;
    DB_Expand_Commit(static_cast<int>(sizeof(EmblemBackground)));
}

void DB_ExpandLoad_EmblemBackgroundArray(EmblemBackground *dst, int count)
{
    iassert(dst == reinterpret_cast<EmblemBackground *>(DB_GetStreamPos()));
    iassert(count >= 0);
    count = DB_Expand_ClampCount(count, "EmblemBackground_32");
    std::vector<EmblemBackground_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(EmblemBackground_32) * count));
    memset(dst, 0, sizeof(EmblemBackground) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        const EmblemBackground_32 &s = disk[static_cast<size_t>(i)];
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].material), s.material);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].description), s.description);
        dst[i].cost = s.cost;
        dst[i].unlockLevel = s.unlockLevel;
        dst[i].unlockPLevel = s.unlockPLevel;
        dst[i].unclassifyAt = s.unclassifyAt;
    }
    DB_Expand_Commit(static_cast<int>(sizeof(EmblemBackground) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_EmblemSet(EmblemSet *dst)
{
    iassert(dst == reinterpret_cast<EmblemSet *>(DB_GetStreamPos()));
    EmblemSet_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    dst->colorCount = disk.colorCount;
    dst->layerCount = disk.layerCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->layers), disk.layers);
    dst->categoryCount = disk.categoryCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->categories), disk.categories);
    dst->iconCount = disk.iconCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->icons), disk.icons);
    dst->backgroundCount = disk.backgroundCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->backgrounds), disk.backgrounds);
    dst->backgroundLookupCount = disk.backgroundLookupCount;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->backgroundLookup), disk.backgroundLookup);
    DB_Expand_Commit(static_cast<int>(sizeof(EmblemSet)));
}

void DB_ExpandLoad_FxImpactTable(FxImpactTable *dst)
{
    iassert(dst == reinterpret_cast<FxImpactTable *>(DB_GetStreamPos()));
    FxImpactTable_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), disk.name);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->table), disk.table);
    DB_Expand_Commit(static_cast<int>(sizeof(FxImpactTable)));
}

void DB_ExpandLoad_FxImpactEntry(FxImpactEntry *dst)
{
    iassert(dst == reinterpret_cast<FxImpactEntry *>(DB_GetStreamPos()));
    FxImpactEntry_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    for (int i = 0; i < 31; ++i)
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->nonflesh[i]), disk.nonflesh[i]);
    for (int i = 0; i < 4; ++i)
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->flesh[i]), disk.flesh[i]);
    DB_Expand_Commit(static_cast<int>(sizeof(FxImpactEntry)));
}

void DB_ExpandLoad_FxImpactEntryArray(FxImpactEntry *dst, int count)
{
    iassert(dst == reinterpret_cast<FxImpactEntry *>(DB_GetStreamPos()));
    iassert(count >= 0);
    count = DB_Expand_ClampCount(count, "FxImpactEntry_32");
    std::vector<FxImpactEntry_32> disk(static_cast<size_t>(count));
    if (count)
        DB_Expand_ReadFile(disk.data(), static_cast<int>(sizeof(FxImpactEntry_32) * count));
    memset(dst, 0, sizeof(FxImpactEntry) * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        for (int j = 0; j < 31; ++j)
            DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].nonflesh[j]), disk[static_cast<size_t>(i)].nonflesh[j]);
        for (int j = 0; j < 4; ++j)
            DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst[i].flesh[j]), disk[static_cast<size_t>(i)].flesh[j]);
    }
    DB_Expand_Commit(static_cast<int>(sizeof(FxImpactEntry) * static_cast<size_t>(count)), count);
}

void DB_ExpandLoad_flameTable(flameTable *dst)
{
    // Disk: 106 floats followed by 13 four-byte pointer tokens (476 bytes).
    struct Disk { float values[106]; uint32_t pointers[13]; };
    static_assert(sizeof(Disk)==476, "flame table disk layout");
    static_assert(offsetof(flameTable,name)==424, "flame table scalar prefix");
    iassert(dst == reinterpret_cast<flameTable *>(DB_GetStreamPos()));
    Disk disk{};
    DB_Expand_ReadFile(&disk,sizeof(disk));
    memset(dst,0,sizeof(*dst));
    memcpy(dst,disk.values,sizeof(disk.values));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name),disk.pointers[0]);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->fire),disk.pointers[1]);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->smoke),disk.pointers[2]);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->heat),disk.pointers[3]);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->drips),disk.pointers[4]);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->streamFuel),disk.pointers[5]);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->streamFuel2),disk.pointers[6]);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->streamFlame),disk.pointers[7]);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->streamFlame2),disk.pointers[8]);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->flameOffLoopSound),disk.pointers[9]);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->flameIgniteSound),disk.pointers[10]);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->flameOnLoopSound),disk.pointers[11]);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->flameCooldownSound),disk.pointers[12]);
    DB_Expand_Commit(static_cast<int>(sizeof(*dst)));
}

void DB_ExpandLoad_WeaponDef(WeaponDef *dst)
{
    iassert(dst == reinterpret_cast<WeaponDef *>(DB_GetStreamPos()));
    static_assert(sizeof(WeaponDef) >= 2056, "x64 WeaponDef must be at least Win32 size");
    {
    }
    uint8_t disk[2056];
    DB_Expand_ReadFile(disk, 2056);
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->szOverlayName), *reinterpret_cast<uint32_t *>(disk + 0));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->gunXModel), *reinterpret_cast<uint32_t *>(disk + 4));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->handXModel), *reinterpret_cast<uint32_t *>(disk + 8));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->szModeName), *reinterpret_cast<uint32_t *>(disk + 12));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->notetrackSoundMapKeys), *reinterpret_cast<uint32_t *>(disk + 16));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->notetrackSoundMapValues), *reinterpret_cast<uint32_t *>(disk + 20));
    memcpy(&dst->playerAnimType, disk + 24, 4);
    memcpy(&dst->weapType, disk + 28, 4);
    memcpy(&dst->weapClass, disk + 32, 4);
    memcpy(&dst->penetrateType, disk + 36, 4);
    memcpy(&dst->impactType, disk + 40, 4);
    memcpy(&dst->inventoryType, disk + 44, 4);
    memcpy(&dst->fireType, disk + 48, 4);
    memcpy(&dst->clipType, disk + 52, 4);
    memcpy(&dst->itemIndex, disk + 56, 4);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->parentWeaponName), *reinterpret_cast<uint32_t *>(disk + 60));
    memcpy(&dst->iJamFireTime, disk + 64, 4);
    memcpy(&dst->tracerFrequency, disk + 68, 4);
    memcpy(&dst->tracerWidth, disk + 72, 4);
    memcpy(&dst->tracerLength, disk + 76, 4);
    memcpy(&dst->overheatWeapon, disk + 80, 4);
    memcpy(&dst->overheatRate, disk + 84, 4);
    memcpy(&dst->cooldownRate, disk + 88, 4);
    memcpy(&dst->overheatEndVal, disk + 92, 4);
    dst->coolWhileFiring = disk[96] != 0;
    dst->fuelTankWeapon = disk[97] != 0;
    memcpy(&dst->iTankLifeTime, disk + 100, 4);
    memcpy(&dst->offhandClass, disk + 104, 4);
    memcpy(&dst->offhandSlot, disk + 108, 4);
    memcpy(&dst->stance, disk + 112, 4);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->viewFlashEffect), *reinterpret_cast<uint32_t *>(disk + 116));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->worldFlashEffect), *reinterpret_cast<uint32_t *>(disk + 120));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->pickupSound), *reinterpret_cast<uint32_t *>(disk + 124));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->pickupSoundPlayer), *reinterpret_cast<uint32_t *>(disk + 128));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->ammoPickupSound), *reinterpret_cast<uint32_t *>(disk + 132));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->ammoPickupSoundPlayer), *reinterpret_cast<uint32_t *>(disk + 136));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->projectileSound), *reinterpret_cast<uint32_t *>(disk + 140));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->pullbackSound), *reinterpret_cast<uint32_t *>(disk + 144));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->pullbackSoundPlayer), *reinterpret_cast<uint32_t *>(disk + 148));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->fireSound), *reinterpret_cast<uint32_t *>(disk + 152));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->fireSoundPlayer), *reinterpret_cast<uint32_t *>(disk + 156));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->fireLoopSound), *reinterpret_cast<uint32_t *>(disk + 160));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->fireLoopSoundPlayer), *reinterpret_cast<uint32_t *>(disk + 164));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->fireLoopEndSound), *reinterpret_cast<uint32_t *>(disk + 168));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->fireLoopEndSoundPlayer), *reinterpret_cast<uint32_t *>(disk + 172));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->fireStopSound), *reinterpret_cast<uint32_t *>(disk + 176));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->fireStopSoundPlayer), *reinterpret_cast<uint32_t *>(disk + 180));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->fireLastSound), *reinterpret_cast<uint32_t *>(disk + 184));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->fireLastSoundPlayer), *reinterpret_cast<uint32_t *>(disk + 188));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->emptyFireSound), *reinterpret_cast<uint32_t *>(disk + 192));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->emptyFireSoundPlayer), *reinterpret_cast<uint32_t *>(disk + 196));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->crackSound), *reinterpret_cast<uint32_t *>(disk + 200));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->whizbySound), *reinterpret_cast<uint32_t *>(disk + 204));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->meleeSwipeSound), *reinterpret_cast<uint32_t *>(disk + 208));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->meleeSwipeSoundPlayer), *reinterpret_cast<uint32_t *>(disk + 212));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->meleeHitSound), *reinterpret_cast<uint32_t *>(disk + 216));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->meleeMissSound), *reinterpret_cast<uint32_t *>(disk + 220));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->rechamberSound), *reinterpret_cast<uint32_t *>(disk + 224));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->rechamberSoundPlayer), *reinterpret_cast<uint32_t *>(disk + 228));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->reloadSound), *reinterpret_cast<uint32_t *>(disk + 232));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->reloadSoundPlayer), *reinterpret_cast<uint32_t *>(disk + 236));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->reloadEmptySound), *reinterpret_cast<uint32_t *>(disk + 240));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->reloadEmptySoundPlayer), *reinterpret_cast<uint32_t *>(disk + 244));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->reloadStartSound), *reinterpret_cast<uint32_t *>(disk + 248));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->reloadStartSoundPlayer), *reinterpret_cast<uint32_t *>(disk + 252));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->reloadEndSound), *reinterpret_cast<uint32_t *>(disk + 256));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->reloadEndSoundPlayer), *reinterpret_cast<uint32_t *>(disk + 260));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->rotateLoopSound), *reinterpret_cast<uint32_t *>(disk + 264));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->rotateLoopSoundPlayer), *reinterpret_cast<uint32_t *>(disk + 268));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->deploySound), *reinterpret_cast<uint32_t *>(disk + 272));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->deploySoundPlayer), *reinterpret_cast<uint32_t *>(disk + 276));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->finishDeploySound), *reinterpret_cast<uint32_t *>(disk + 280));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->finishDeploySoundPlayer), *reinterpret_cast<uint32_t *>(disk + 284));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->breakdownSound), *reinterpret_cast<uint32_t *>(disk + 288));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->breakdownSoundPlayer), *reinterpret_cast<uint32_t *>(disk + 292));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->finishBreakdownSound), *reinterpret_cast<uint32_t *>(disk + 296));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->finishBreakdownSoundPlayer), *reinterpret_cast<uint32_t *>(disk + 300));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->detonateSound), *reinterpret_cast<uint32_t *>(disk + 304));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->detonateSoundPlayer), *reinterpret_cast<uint32_t *>(disk + 308));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->nightVisionWearSound), *reinterpret_cast<uint32_t *>(disk + 312));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->nightVisionWearSoundPlayer), *reinterpret_cast<uint32_t *>(disk + 316));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->nightVisionRemoveSound), *reinterpret_cast<uint32_t *>(disk + 320));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->nightVisionRemoveSoundPlayer), *reinterpret_cast<uint32_t *>(disk + 324));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->altSwitchSound), *reinterpret_cast<uint32_t *>(disk + 328));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->altSwitchSoundPlayer), *reinterpret_cast<uint32_t *>(disk + 332));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->raiseSound), *reinterpret_cast<uint32_t *>(disk + 336));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->raiseSoundPlayer), *reinterpret_cast<uint32_t *>(disk + 340));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->firstRaiseSound), *reinterpret_cast<uint32_t *>(disk + 344));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->firstRaiseSoundPlayer), *reinterpret_cast<uint32_t *>(disk + 348));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->putawaySound), *reinterpret_cast<uint32_t *>(disk + 352));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->putawaySoundPlayer), *reinterpret_cast<uint32_t *>(disk + 356));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->overheatSound), *reinterpret_cast<uint32_t *>(disk + 360));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->overheatSoundPlayer), *reinterpret_cast<uint32_t *>(disk + 364));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->adsZoomSound), *reinterpret_cast<uint32_t *>(disk + 368));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->bounceSound), *reinterpret_cast<uint32_t *>(disk + 372));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->standMountedWeapdef), *reinterpret_cast<uint32_t *>(disk + 376));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->crouchMountedWeapdef), *reinterpret_cast<uint32_t *>(disk + 380));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->proneMountedWeapdef), *reinterpret_cast<uint32_t *>(disk + 384));
    memcpy(&dst->standMountedIndex, disk + 388, 4);
    memcpy(&dst->crouchMountedIndex, disk + 392, 4);
    memcpy(&dst->proneMountedIndex, disk + 396, 4);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->viewShellEjectEffect), *reinterpret_cast<uint32_t *>(disk + 400));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->worldShellEjectEffect), *reinterpret_cast<uint32_t *>(disk + 404));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->viewLastShotEjectEffect), *reinterpret_cast<uint32_t *>(disk + 408));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->worldLastShotEjectEffect), *reinterpret_cast<uint32_t *>(disk + 412));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->reticleCenter), *reinterpret_cast<uint32_t *>(disk + 416));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->reticleSide), *reinterpret_cast<uint32_t *>(disk + 420));
    memcpy(&dst->iReticleCenterSize, disk + 424, 4);
    memcpy(&dst->iReticleSideSize, disk + 428, 4);
    memcpy(&dst->iReticleMinOfs, disk + 432, 4);
    memcpy(&dst->activeReticleType, disk + 436, 4);
    memcpy(&dst->vStandMove, disk + 440, 12);
    memcpy(&dst->vStandRot, disk + 452, 12);
    memcpy(&dst->vDuckedOfs, disk + 464, 12);
    memcpy(&dst->vDuckedMove, disk + 476, 12);
    memcpy(&dst->vDuckedSprintOfs, disk + 488, 12);
    memcpy(&dst->vDuckedSprintRot, disk + 500, 12);
    memcpy(&dst->vDuckedSprintBob, disk + 512, 8);
    memcpy(&dst->fDuckedSprintCycleScale, disk + 520, 4);
    memcpy(&dst->vSprintOfs, disk + 524, 12);
    memcpy(&dst->vSprintRot, disk + 536, 12);
    memcpy(&dst->vSprintBob, disk + 548, 8);
    memcpy(&dst->fSprintCycleScale, disk + 556, 4);
    memcpy(&dst->vLowReadyOfs, disk + 560, 12);
    memcpy(&dst->vLowReadyRot, disk + 572, 12);
    memcpy(&dst->vDtpOfs, disk + 584, 12);
    memcpy(&dst->vDtpRot, disk + 596, 12);
    memcpy(&dst->vDtpBob, disk + 608, 8);
    memcpy(&dst->fDtpCycleScale, disk + 616, 4);
    memcpy(&dst->vMantleOfs, disk + 620, 12);
    memcpy(&dst->vMantleRot, disk + 632, 12);
    memcpy(&dst->vSlideOfs, disk + 644, 12);
    memcpy(&dst->vSlideRot, disk + 656, 12);
    memcpy(&dst->vDuckedRot, disk + 668, 12);
    memcpy(&dst->vProneOfs, disk + 680, 12);
    memcpy(&dst->vProneMove, disk + 692, 12);
    memcpy(&dst->vProneRot, disk + 704, 12);
    memcpy(&dst->vStrafeMove, disk + 716, 12);
    memcpy(&dst->vStrafeRot, disk + 728, 12);
    memcpy(&dst->fPosMoveRate, disk + 740, 4);
    memcpy(&dst->fPosProneMoveRate, disk + 744, 4);
    memcpy(&dst->fStandMoveMinSpeed, disk + 748, 4);
    memcpy(&dst->fDuckedMoveMinSpeed, disk + 752, 4);
    memcpy(&dst->fProneMoveMinSpeed, disk + 756, 4);
    memcpy(&dst->fPosRotRate, disk + 760, 4);
    memcpy(&dst->fPosProneRotRate, disk + 764, 4);
    memcpy(&dst->fStandRotMinSpeed, disk + 768, 4);
    memcpy(&dst->fDuckedRotMinSpeed, disk + 772, 4);
    memcpy(&dst->fProneRotMinSpeed, disk + 776, 4);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->worldModel), *reinterpret_cast<uint32_t *>(disk + 780));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->worldClipModel), *reinterpret_cast<uint32_t *>(disk + 784));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->rocketModel), *reinterpret_cast<uint32_t *>(disk + 788));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->mountedModel), *reinterpret_cast<uint32_t *>(disk + 792));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->additionalMeleeModel), *reinterpret_cast<uint32_t *>(disk + 796));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->hudIcon), *reinterpret_cast<uint32_t *>(disk + 800));
    memcpy(&dst->hudIconRatio, disk + 804, 4);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->indicatorIcon), *reinterpret_cast<uint32_t *>(disk + 808));
    memcpy(&dst->indicatorIconRatio, disk + 812, 4);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->ammoCounterIcon), *reinterpret_cast<uint32_t *>(disk + 816));
    memcpy(&dst->ammoCounterIconRatio, disk + 820, 4);
    memcpy(&dst->ammoCounterClip, disk + 824, 4);
    memcpy(&dst->iStartAmmo, disk + 828, 4);
    memcpy(&dst->iHeatIndex, disk + 832, 4);
    memcpy(&dst->iMaxAmmo, disk + 836, 4);
    memcpy(&dst->shotCount, disk + 840, 4);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->szSharedAmmoCapName), *reinterpret_cast<uint32_t *>(disk + 844));
    memcpy(&dst->iSharedAmmoCapIndex, disk + 848, 4);
    memcpy(&dst->iSharedAmmoCap, disk + 852, 4);
    dst->unlimitedAmmo = disk[856] != 0;
    dst->ammoCountClipRelative = disk[857] != 0;
    memcpy(&dst->damage, disk + 860, 4);
    memcpy(&dst->damageDuration, disk + 864, 4);
    memcpy(&dst->damageInterval, disk + 868, 4);
    memcpy(&dst->playerDamage, disk + 872, 4);
    memcpy(&dst->iMeleeDamage, disk + 876, 4);
    memcpy(&dst->iDamageType, disk + 880, 4);
    dst->explosionTag = *reinterpret_cast<uint16_t *>(disk + 884);
    memcpy(&dst->iFireDelay, disk + 888, 4);
    memcpy(&dst->iMeleeDelay, disk + 892, 4);
    memcpy(&dst->meleeChargeDelay, disk + 896, 4);
    memcpy(&dst->iDetonateDelay, disk + 900, 4);
    memcpy(&dst->iSpinUpTime, disk + 904, 4);
    memcpy(&dst->iSpinDownTime, disk + 908, 4);
    memcpy(&dst->spinRate, disk + 912, 4);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->spinLoopSound), *reinterpret_cast<uint32_t *>(disk + 916));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->spinLoopSoundPlayer), *reinterpret_cast<uint32_t *>(disk + 920));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->startSpinSound), *reinterpret_cast<uint32_t *>(disk + 924));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->startSpinSoundPlayer), *reinterpret_cast<uint32_t *>(disk + 928));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->stopSpinSound), *reinterpret_cast<uint32_t *>(disk + 932));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->stopSpinSoundPlayer), *reinterpret_cast<uint32_t *>(disk + 936));
    memcpy(&dst->iFireTime, disk + 940, 4);
    memcpy(&dst->iLastFireTime, disk + 944, 4);
    memcpy(&dst->iRechamberTime, disk + 948, 4);
    memcpy(&dst->iRechamberBoltTime, disk + 952, 4);
    memcpy(&dst->iHoldFireTime, disk + 956, 4);
    memcpy(&dst->iDetonateTime, disk + 960, 4);
    memcpy(&dst->iMeleeTime, disk + 964, 4);
    memcpy(&dst->meleeChargeTime, disk + 968, 4);
    memcpy(&dst->iReloadTimeRight, disk + 972, 4);
    memcpy(&dst->iReloadTimeLeft, disk + 976, 4);
    memcpy(&dst->reloadShowRocketTime, disk + 980, 4);
    memcpy(&dst->iReloadEmptyTimeLeft, disk + 984, 4);
    memcpy(&dst->iReloadAddTime, disk + 988, 4);
    memcpy(&dst->iReloadEmptyAddTime, disk + 992, 4);
    memcpy(&dst->iReloadQuickAddTime, disk + 996, 4);
    memcpy(&dst->iReloadQuickEmptyAddTime, disk + 1000, 4);
    memcpy(&dst->iReloadStartTime, disk + 1004, 4);
    memcpy(&dst->iReloadStartAddTime, disk + 1008, 4);
    memcpy(&dst->iReloadEndTime, disk + 1012, 4);
    memcpy(&dst->iDropTime, disk + 1016, 4);
    memcpy(&dst->iRaiseTime, disk + 1020, 4);
    memcpy(&dst->iAltDropTime, disk + 1024, 4);
    memcpy(&dst->quickDropTime, disk + 1028, 4);
    memcpy(&dst->quickRaiseTime, disk + 1032, 4);
    memcpy(&dst->iFirstRaiseTime, disk + 1036, 4);
    memcpy(&dst->iEmptyRaiseTime, disk + 1040, 4);
    memcpy(&dst->iEmptyDropTime, disk + 1044, 4);
    memcpy(&dst->sprintInTime, disk + 1048, 4);
    memcpy(&dst->sprintLoopTime, disk + 1052, 4);
    memcpy(&dst->sprintOutTime, disk + 1056, 4);
    memcpy(&dst->lowReadyInTime, disk + 1060, 4);
    memcpy(&dst->lowReadyLoopTime, disk + 1064, 4);
    memcpy(&dst->lowReadyOutTime, disk + 1068, 4);
    memcpy(&dst->contFireInTime, disk + 1072, 4);
    memcpy(&dst->contFireLoopTime, disk + 1076, 4);
    memcpy(&dst->contFireOutTime, disk + 1080, 4);
    memcpy(&dst->dtpInTime, disk + 1084, 4);
    memcpy(&dst->dtpLoopTime, disk + 1088, 4);
    memcpy(&dst->dtpOutTime, disk + 1092, 4);
    memcpy(&dst->slideInTime, disk + 1096, 4);
    memcpy(&dst->deployTime, disk + 1100, 4);
    memcpy(&dst->breakdownTime, disk + 1104, 4);
    memcpy(&dst->nightVisionWearTime, disk + 1108, 4);
    memcpy(&dst->nightVisionWearTimeFadeOutEnd, disk + 1112, 4);
    memcpy(&dst->nightVisionWearTimePowerUp, disk + 1116, 4);
    memcpy(&dst->nightVisionRemoveTime, disk + 1120, 4);
    memcpy(&dst->nightVisionRemoveTimePowerDown, disk + 1124, 4);
    memcpy(&dst->nightVisionRemoveTimeFadeInStart, disk + 1128, 4);
    memcpy(&dst->fuseTime, disk + 1132, 4);
    memcpy(&dst->aiFuseTime, disk + 1136, 4);
    memcpy(&dst->lockOnRadius, disk + 1140, 4);
    memcpy(&dst->lockOnSpeed, disk + 1144, 4);
    dst->requireLockonToFire = disk[1148] != 0;
    dst->noAdsWhenMagEmpty = disk[1149] != 0;
    dst->avoidDropCleanup = disk[1150] != 0;
    memcpy(&dst->stackFire, disk + 1152, 4);
    memcpy(&dst->stackFireSpread, disk + 1156, 4);
    memcpy(&dst->stackFireAccuracyDecay, disk + 1160, 4);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->stackSound), *reinterpret_cast<uint32_t *>(disk + 1164));
    memcpy(&dst->autoAimRange, disk + 1168, 4);
    memcpy(&dst->aimAssistRange, disk + 1172, 4);
    dst->mountableWeapon = disk[1176] != 0;
    memcpy(&dst->aimPadding, disk + 1180, 4);
    memcpy(&dst->enemyCrosshairRange, disk + 1184, 4);
    dst->crosshairColorChange = disk[1188] != 0;
    memcpy(&dst->moveSpeedScale, disk + 1192, 4);
    memcpy(&dst->adsMoveSpeedScale, disk + 1196, 4);
    memcpy(&dst->sprintDurationScale, disk + 1200, 4);
    memcpy(&dst->overlayReticle, disk + 1204, 4);
    memcpy(&dst->overlayInterface, disk + 1208, 4);
    memcpy(&dst->overlayWidth, disk + 1212, 4);
    memcpy(&dst->overlayHeight, disk + 1216, 4);
    memcpy(&dst->fAdsBobFactor, disk + 1220, 4);
    memcpy(&dst->fAdsViewBobMult, disk + 1224, 4);
    memcpy(&dst->fHipSpreadStandMin, disk + 1228, 4);
    memcpy(&dst->fHipSpreadDuckedMin, disk + 1232, 4);
    memcpy(&dst->fHipSpreadProneMin, disk + 1236, 4);
    memcpy(&dst->hipSpreadStandMax, disk + 1240, 4);
    memcpy(&dst->hipSpreadDuckedMax, disk + 1244, 4);
    memcpy(&dst->hipSpreadProneMax, disk + 1248, 4);
    memcpy(&dst->fHipSpreadDecayRate, disk + 1252, 4);
    memcpy(&dst->fHipSpreadFireAdd, disk + 1256, 4);
    memcpy(&dst->fHipSpreadTurnAdd, disk + 1260, 4);
    memcpy(&dst->fHipSpreadMoveAdd, disk + 1264, 4);
    memcpy(&dst->fHipSpreadDuckedDecay, disk + 1268, 4);
    memcpy(&dst->fHipSpreadProneDecay, disk + 1272, 4);
    memcpy(&dst->fHipReticleSidePos, disk + 1276, 4);
    memcpy(&dst->fAdsIdleAmount, disk + 1280, 4);
    memcpy(&dst->fHipIdleAmount, disk + 1284, 4);
    memcpy(&dst->adsIdleSpeed, disk + 1288, 4);
    memcpy(&dst->hipIdleSpeed, disk + 1292, 4);
    memcpy(&dst->fIdleCrouchFactor, disk + 1296, 4);
    memcpy(&dst->fIdleProneFactor, disk + 1300, 4);
    memcpy(&dst->fGunMaxPitch, disk + 1304, 4);
    memcpy(&dst->fGunMaxYaw, disk + 1308, 4);
    memcpy(&dst->swayMaxAngle, disk + 1312, 4);
    memcpy(&dst->swayLerpSpeed, disk + 1316, 4);
    memcpy(&dst->swayPitchScale, disk + 1320, 4);
    memcpy(&dst->swayYawScale, disk + 1324, 4);
    memcpy(&dst->swayHorizScale, disk + 1328, 4);
    memcpy(&dst->swayVertScale, disk + 1332, 4);
    memcpy(&dst->swayShellShockScale, disk + 1336, 4);
    memcpy(&dst->adsSwayMaxAngle, disk + 1340, 4);
    memcpy(&dst->adsSwayLerpSpeed, disk + 1344, 4);
    memcpy(&dst->adsSwayPitchScale, disk + 1348, 4);
    memcpy(&dst->adsSwayYawScale, disk + 1352, 4);
    dst->sharedAmmo = disk[1356] != 0;
    dst->bRifleBullet = disk[1357] != 0;
    dst->armorPiercing = disk[1358] != 0;
    dst->bBoltAction = disk[1359] != 0;
    dst->bUseAltTagFlash = disk[1360] != 0;
    dst->bUseAntiLagRewind = disk[1361] != 0;
    dst->bIsCarriedKillstreakWeapon = disk[1362] != 0;
    dst->aimDownSight = disk[1363] != 0;
    dst->bRechamberWhileAds = disk[1364] != 0;
    dst->bReloadWhileAds = disk[1365] != 0;
    memcpy(&dst->adsViewErrorMin, disk + 1368, 4);
    memcpy(&dst->adsViewErrorMax, disk + 1372, 4);
    dst->bCookOffHold = disk[1376] != 0;
    dst->bClipOnly = disk[1377] != 0;
    dst->bCanUseInVehicle = disk[1378] != 0;
    dst->bNoDropsOrRaises = disk[1379] != 0;
    dst->adsFireOnly = disk[1380] != 0;
    dst->cancelAutoHolsterWhenEmpty = disk[1381] != 0;
    dst->suppressAmmoReserveDisplay = disk[1382] != 0;
    dst->laserSightDuringNightvision = disk[1383] != 0;
    dst->bHideThirdPerson = disk[1384] != 0;
    dst->bHasBayonet = disk[1385] != 0;
    dst->bDualWield = disk[1386] != 0;
    dst->bExplodeOnGround = disk[1387] != 0;
    dst->bThrowBack = disk[1388] != 0;
    dst->bRetrievable = disk[1389] != 0;
    dst->bDieOnRespawn = disk[1390] != 0;
    dst->bNoThirdPersonDropsOrRaises = disk[1391] != 0;
    dst->bContinuousFire = disk[1392] != 0;
    dst->bNoPing = disk[1393] != 0;
    dst->bForceBounce = disk[1394] != 0;
    dst->bUseDroppedModelAsStowed = disk[1395] != 0;
    dst->bNoQuickDropWhenEmpty = disk[1396] != 0;
    dst->bKeepCrosshairWhenADS = disk[1397] != 0;
    dst->bUseOnlyAltWeaoponHideTagsInAltMode = disk[1398] != 0;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->killIcon), *reinterpret_cast<uint32_t *>(disk + 1400));
    memcpy(&dst->killIconRatio, disk + 1404, 4);
    dst->flipKillIcon = disk[1408] != 0;
    dst->bNoPartialReload = disk[1409] != 0;
    dst->bSegmentedReload = disk[1410] != 0;
    dst->bNoADSAutoReload = disk[1411] != 0;
    memcpy(&dst->iReloadAmmoAdd, disk + 1412, 4);
    memcpy(&dst->iReloadStartAdd, disk + 1416, 4);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->szSpawnedGrenadeWeaponName), *reinterpret_cast<uint32_t *>(disk + 1420));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->szDualWieldWeaponName), *reinterpret_cast<uint32_t *>(disk + 1424));
    memcpy(&dst->dualWieldWeaponIndex, disk + 1428, 4);
    memcpy(&dst->iDropAmmoMin, disk + 1432, 4);
    memcpy(&dst->iDropAmmoMax, disk + 1436, 4);
    memcpy(&dst->iDropClipAmmoMin, disk + 1440, 4);
    memcpy(&dst->iDropClipAmmoMax, disk + 1444, 4);
    dst->blocksProne = disk[1448] != 0;
    dst->bShowIndicator = disk[1449] != 0;
    memcpy(&dst->isRollingGrenade, disk + 1452, 4);
    memcpy(&dst->iExplosionRadius, disk + 1456, 4);
    memcpy(&dst->iExplosionRadiusMin, disk + 1460, 4);
    memcpy(&dst->iIndicatorRadius, disk + 1464, 4);
    memcpy(&dst->iExplosionInnerDamage, disk + 1468, 4);
    memcpy(&dst->iExplosionOuterDamage, disk + 1472, 4);
    memcpy(&dst->damageConeAngle, disk + 1476, 4);
    memcpy(&dst->iProjectileSpeed, disk + 1480, 4);
    memcpy(&dst->iProjectileSpeedUp, disk + 1484, 4);
    memcpy(&dst->iProjectileSpeedRelativeUp, disk + 1488, 4);
    memcpy(&dst->iProjectileSpeedForward, disk + 1492, 4);
    memcpy(&dst->iProjectileActivateDist, disk + 1496, 4);
    memcpy(&dst->projLifetime, disk + 1500, 4);
    memcpy(&dst->timeToAccelerate, disk + 1504, 4);
    memcpy(&dst->projectileCurvature, disk + 1508, 4);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->projectileModel), *reinterpret_cast<uint32_t *>(disk + 1512));
    memcpy(&dst->projExplosion, disk + 1516, 4);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->projExplosionEffect), *reinterpret_cast<uint32_t *>(disk + 1520));
    dst->projExplosionEffectForceNormalUp = disk[1524] != 0;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->projExplosionEffect2), *reinterpret_cast<uint32_t *>(disk + 1528));
    dst->projExplosionEffect2ForceNormalUp = disk[1532] != 0;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->projExplosionEffect3), *reinterpret_cast<uint32_t *>(disk + 1536));
    dst->projExplosionEffect3ForceNormalUp = disk[1540] != 0;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->projExplosionEffect4), *reinterpret_cast<uint32_t *>(disk + 1544));
    dst->projExplosionEffect4ForceNormalUp = disk[1548] != 0;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->projExplosionEffect5), *reinterpret_cast<uint32_t *>(disk + 1552));
    dst->projExplosionEffect5ForceNormalUp = disk[1556] != 0;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->projDudEffect), *reinterpret_cast<uint32_t *>(disk + 1560));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->projExplosionSound), *reinterpret_cast<uint32_t *>(disk + 1564));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->projDudSound), *reinterpret_cast<uint32_t *>(disk + 1568));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->mortarShellSound), *reinterpret_cast<uint32_t *>(disk + 1572));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->tankShellSound), *reinterpret_cast<uint32_t *>(disk + 1576));
    dst->bProjImpactExplode = disk[1580] != 0;
    dst->bBulletImpactExplode = disk[1581] != 0;
    memcpy(&dst->stickiness, disk + 1584, 4);
    memcpy(&dst->rotateType, disk + 1588, 4);
    dst->plantable = disk[1592] != 0;
    dst->hasDetonator = disk[1593] != 0;
    dst->timedDetonation = disk[1594] != 0;
    dst->bNoCrumpleMissile = disk[1595] != 0;
    dst->rotate = disk[1596] != 0;
    dst->bKeepRolling = disk[1597] != 0;
    dst->holdButtonToThrow = disk[1598] != 0;
    dst->offhandHoldIsCancelable = disk[1599] != 0;
    dst->freezeMovementWhenFiring = disk[1600] != 0;
    memcpy(&dst->lowAmmoWarningThreshold, disk + 1604, 4);
    memcpy(&dst->meleeChargeRange, disk + 1608, 4);
    dst->bUseAsMelee = disk[1612] != 0;
    dst->isCameraSensor = disk[1613] != 0;
    dst->isAcousticSensor = disk[1614] != 0;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->parallelBounce), *reinterpret_cast<uint32_t *>(disk + 1616));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->perpendicularBounce), *reinterpret_cast<uint32_t *>(disk + 1620));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->projTrailEffect), *reinterpret_cast<uint32_t *>(disk + 1624));
    memcpy(&dst->vProjectileColor, disk + 1628, 12);
    memcpy(&dst->guidedMissileType, disk + 1640, 4);
    memcpy(&dst->maxSteeringAccel, disk + 1644, 4);
    memcpy(&dst->projIgnitionDelay, disk + 1648, 4);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->projIgnitionEffect), *reinterpret_cast<uint32_t *>(disk + 1652));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->projIgnitionSound), *reinterpret_cast<uint32_t *>(disk + 1656));
    memcpy(&dst->fAdsAimPitch, disk + 1660, 4);
    memcpy(&dst->fAdsCrosshairInFrac, disk + 1664, 4);
    memcpy(&dst->fAdsCrosshairOutFrac, disk + 1668, 4);
    memcpy(&dst->adsGunKickReducedKickBullets, disk + 1672, 4);
    memcpy(&dst->adsGunKickReducedKickPercent, disk + 1676, 4);
    memcpy(&dst->fAdsGunKickPitchMin, disk + 1680, 4);
    memcpy(&dst->fAdsGunKickPitchMax, disk + 1684, 4);
    memcpy(&dst->fAdsGunKickYawMin, disk + 1688, 4);
    memcpy(&dst->fAdsGunKickYawMax, disk + 1692, 4);
    memcpy(&dst->fAdsGunKickAccel, disk + 1696, 4);
    memcpy(&dst->fAdsGunKickSpeedMax, disk + 1700, 4);
    memcpy(&dst->fAdsGunKickSpeedDecay, disk + 1704, 4);
    memcpy(&dst->fAdsGunKickStaticDecay, disk + 1708, 4);
    memcpy(&dst->fAdsViewKickPitchMin, disk + 1712, 4);
    memcpy(&dst->fAdsViewKickPitchMax, disk + 1716, 4);
    memcpy(&dst->fAdsViewKickYawMin, disk + 1720, 4);
    memcpy(&dst->fAdsViewKickYawMax, disk + 1724, 4);
    memcpy(&dst->fAdsViewScatterMin, disk + 1728, 4);
    memcpy(&dst->fAdsViewScatterMax, disk + 1732, 4);
    memcpy(&dst->fAdsSpread, disk + 1736, 4);
    memcpy(&dst->hipGunKickReducedKickBullets, disk + 1740, 4);
    memcpy(&dst->hipGunKickReducedKickPercent, disk + 1744, 4);
    memcpy(&dst->fHipGunKickPitchMin, disk + 1748, 4);
    memcpy(&dst->fHipGunKickPitchMax, disk + 1752, 4);
    memcpy(&dst->fHipGunKickYawMin, disk + 1756, 4);
    memcpy(&dst->fHipGunKickYawMax, disk + 1760, 4);
    memcpy(&dst->fHipGunKickAccel, disk + 1764, 4);
    memcpy(&dst->fHipGunKickSpeedMax, disk + 1768, 4);
    memcpy(&dst->fHipGunKickSpeedDecay, disk + 1772, 4);
    memcpy(&dst->fHipGunKickStaticDecay, disk + 1776, 4);
    memcpy(&dst->fHipViewKickPitchMin, disk + 1780, 4);
    memcpy(&dst->fHipViewKickPitchMax, disk + 1784, 4);
    memcpy(&dst->fHipViewKickYawMin, disk + 1788, 4);
    memcpy(&dst->fHipViewKickYawMax, disk + 1792, 4);
    memcpy(&dst->fHipViewScatterMin, disk + 1796, 4);
    memcpy(&dst->fHipViewScatterMax, disk + 1800, 4);
    memcpy(&dst->fightDist, disk + 1804, 4);
    memcpy(&dst->maxDist, disk + 1808, 4);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->accuracyGraphName[0]), *reinterpret_cast<uint32_t *>(disk + 1812));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->accuracyGraphName[1]), *reinterpret_cast<uint32_t *>(disk + 1816));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->accuracyGraphKnots[0]), *reinterpret_cast<uint32_t *>(disk + 1820));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->accuracyGraphKnots[1]), *reinterpret_cast<uint32_t *>(disk + 1824));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->originalAccuracyGraphKnots[0]), *reinterpret_cast<uint32_t *>(disk + 1828));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->originalAccuracyGraphKnots[1]), *reinterpret_cast<uint32_t *>(disk + 1832));
    memcpy(&dst->accuracyGraphKnotCount, disk + 1836, 8);
    memcpy(&dst->originalAccuracyGraphKnotCount, disk + 1844, 8);
    memcpy(&dst->iPositionReloadTransTime, disk + 1852, 4);
    memcpy(&dst->leftArc, disk + 1856, 4);
    memcpy(&dst->rightArc, disk + 1860, 4);
    memcpy(&dst->topArc, disk + 1864, 4);
    memcpy(&dst->bottomArc, disk + 1868, 4);
    memcpy(&dst->accuracy, disk + 1872, 4);
    memcpy(&dst->aiSpread, disk + 1876, 4);
    memcpy(&dst->playerSpread, disk + 1880, 4);
    memcpy(&dst->minTurnSpeed, disk + 1884, 8);
    memcpy(&dst->maxTurnSpeed, disk + 1892, 8);
    memcpy(&dst->pitchConvergenceTime, disk + 1900, 4);
    memcpy(&dst->yawConvergenceTime, disk + 1904, 4);
    memcpy(&dst->suppressTime, disk + 1908, 4);
    memcpy(&dst->maxRange, disk + 1912, 4);
    memcpy(&dst->fAnimHorRotateInc, disk + 1916, 4);
    memcpy(&dst->fPlayerPositionDist, disk + 1920, 4);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->szUseHintString), *reinterpret_cast<uint32_t *>(disk + 1924));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->dropHintString), *reinterpret_cast<uint32_t *>(disk + 1928));
    memcpy(&dst->iUseHintStringIndex, disk + 1932, 4);
    memcpy(&dst->dropHintStringIndex, disk + 1936, 4);
    memcpy(&dst->horizViewJitter, disk + 1940, 4);
    memcpy(&dst->vertViewJitter, disk + 1944, 4);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->szScript), *reinterpret_cast<uint32_t *>(disk + 1948));
    memcpy(&dst->minDamage, disk + 1952, 4);
    memcpy(&dst->minPlayerDamage, disk + 1956, 4);
    memcpy(&dst->fMaxDamageRange, disk + 1960, 4);
    memcpy(&dst->fMinDamageRange, disk + 1964, 4);
    memcpy(&dst->destabilizationRateTime, disk + 1968, 4);
    memcpy(&dst->destabilizationCurvatureMax, disk + 1972, 4);
    memcpy(&dst->destabilizeDistance, disk + 1976, 4);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->locationDamageMultipliers), *reinterpret_cast<uint32_t *>(disk + 1980));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->fireRumble), *reinterpret_cast<uint32_t *>(disk + 1984));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->meleeImpactRumble), *reinterpret_cast<uint32_t *>(disk + 1988));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->reloadRumble), *reinterpret_cast<uint32_t *>(disk + 1992));
    memcpy(&dst->adsDofStart, disk + 1996, 4);
    memcpy(&dst->adsDofEnd, disk + 2000, 4);
    memcpy(&dst->hipDofStart, disk + 2004, 4);
    memcpy(&dst->hipDofEnd, disk + 2008, 4);
    memcpy(&dst->scanSpeed, disk + 2012, 4);
    memcpy(&dst->scanAccel, disk + 2016, 4);
    memcpy(&dst->scanPauseTime, disk + 2020, 4);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->flameTableFirstPerson), *reinterpret_cast<uint32_t *>(disk + 2024));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->flameTableThirdPerson), *reinterpret_cast<uint32_t *>(disk + 2028));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->flameTableFirstPersonPtr), *reinterpret_cast<uint32_t *>(disk + 2032));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->flameTableThirdPersonPtr), *reinterpret_cast<uint32_t *>(disk + 2036));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->tagFx_preparationEffect), *reinterpret_cast<uint32_t *>(disk + 2040));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->tagFlash_preparationEffect), *reinterpret_cast<uint32_t *>(disk + 2044));
    dst->doGibbing = disk[2048] != 0;
    memcpy(&dst->maxGibDistance, disk + 2052, 4);
    DB_Expand_Commit(static_cast<int>(sizeof(WeaponDef)));
}


#pragma pack(push, 1)
struct rectData_s_32
{
    ExpressionStatement_32 rectXExp;
    ExpressionStatement_32 rectYExp;
    ExpressionStatement_32 rectWExp;
    ExpressionStatement_32 rectHExp;
};
struct imageDef_s_32
{
    ExpressionStatement_32 materialExp;
};
struct ownerDrawDef_s_32
{
    ExpressionStatement_32 dataExp;
};
struct textDef_s_32
{
    float textRect[6];
    int alignment;
    int fontEnum;
    int itemFlags;
    int textAlignMode;
    float textalignx;
    float textaligny;
    float textscale;
    int textStyle;
    uint32_t text;
    uint32_t textExpData;
    uint32_t textTypeData;
};
struct focusItemDef_s_32
{
    uint32_t mouseEnterText;
    uint32_t mouseExitText;
    uint32_t mouseEnter;
    uint32_t mouseExit;
    uint32_t onKey;
    uint32_t focusTypeData;
};
struct animParamsDef_t_32
{
    uint32_t name;
    float rectClient[6];
    float borderSize;
    float foreColor[4];
    float backColor[4];
    float borderColor[4];
    float outlineColor[4];
    float textScale;
    float rotation;
    uint32_t onEvent;
};
struct UIAnimInfo_32
{
    int animStateCount;
    uint32_t animStates;
    animParamsDef_t_32 currentAnimState;
    animParamsDef_t_32 nextAnimState;
    int animating;
    int animStartTime;
    int animDuration;
};
struct listBoxDef_s_32
{
    int mousePos;
    int cursorPos[1];
    int startPos[1];
    int endPos[1];
    int drawPadding;
    float elementWidth;
    float elementHeight;
    int numColumns;
    float special;
    uint8_t columnInfo[512]; // 16 * 32
    int notselectable;
    int noScrollBars;
    int usePaging;
    float selectBorder[4];
    float disableColor[4];
    float focusColor[4];
    float elementHighlightColor[4];
    float elementBackgroundColor[4];
    uint32_t selectIcon;
    uint32_t backgroundItemListbox;
    uint32_t highlightTexture;
    int noBlinkingHighlight;
    uint32_t rows;
    int maxRows;
    int rowCount;
};
struct multiDef_s_32
{
    uint32_t dvarList[32];
    uint32_t dvarStr[32];
    float dvarValue[32];
    int count;
    int actionOnEnterPressOnly;
    int strDef;
};
#pragma pack(pop)

static_assert(sizeof(rectData_s_32) == 64, "Win32 rectData");
static_assert(sizeof(imageDef_s_32) == 16, "Win32 imageDef");
static_assert(sizeof(ownerDrawDef_s_32) == 16, "Win32 ownerDrawDef");
static_assert(sizeof(textDef_s_32) == 68, "Win32 textDef");
static_assert(sizeof(focusItemDef_s_32) == 24, "Win32 focusItemDef");
static_assert(sizeof(animParamsDef_t_32) == 108, "Win32 animParamsDef");
static_assert(sizeof(UIAnimInfo_32) == 236, "Win32 UIAnimInfo");
static_assert(sizeof(listBoxDef_s_32) == 668, "Win32 listBoxDef");
static_assert(sizeof(multiDef_s_32) == 396, "Win32 multiDef");

void DB_ExpandLoad_rectData_t(rectData_s *dst)
{
    iassert(dst == reinterpret_cast<rectData_s *>(DB_GetStreamPos()));
    rectData_s_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_Expand_ExpressionStatement(&dst->rectXExp, disk.rectXExp);
    DB_Expand_ExpressionStatement(&dst->rectYExp, disk.rectYExp);
    DB_Expand_ExpressionStatement(&dst->rectWExp, disk.rectWExp);
    DB_Expand_ExpressionStatement(&dst->rectHExp, disk.rectHExp);
    DB_Expand_Commit(static_cast<int>(sizeof(rectData_s)));
}

void DB_ExpandLoad_imageDef_t(imageDef_s *dst)
{
    iassert(dst == reinterpret_cast<imageDef_s *>(DB_GetStreamPos()));
    imageDef_s_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_Expand_ExpressionStatement(&dst->materialExp, disk.materialExp);
    DB_Expand_Commit(static_cast<int>(sizeof(imageDef_s)));
}

void DB_ExpandLoad_ownerDrawDef_t(ownerDrawDef_s *dst)
{
    iassert(dst == reinterpret_cast<ownerDrawDef_s *>(DB_GetStreamPos()));
    ownerDrawDef_s_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_Expand_ExpressionStatement(&dst->dataExp, disk.dataExp);
    DB_Expand_Commit(static_cast<int>(sizeof(ownerDrawDef_s)));
}

void DB_ExpandLoad_textDef_t(textDef_s *dst)
{
    iassert(dst == reinterpret_cast<textDef_s *>(DB_GetStreamPos()));
    textDef_s_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    memcpy(dst->textRect, disk.textRect, sizeof(dst->textRect));
    dst->alignment = disk.alignment;
    dst->fontEnum = disk.fontEnum;
    dst->itemFlags = disk.itemFlags;
    dst->textAlignMode = disk.textAlignMode;
    dst->textalignx = disk.textalignx;
    dst->textaligny = disk.textaligny;
    dst->textscale = disk.textscale;
    dst->textStyle = disk.textStyle;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->text), disk.text);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->textExpData), disk.textExpData);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->textTypeData.data), disk.textTypeData);
    DB_Expand_Commit(static_cast<int>(sizeof(textDef_s)));
}

void DB_ExpandLoad_focusItemDef_t(focusItemDef_s *dst)
{
    iassert(dst == reinterpret_cast<focusItemDef_s *>(DB_GetStreamPos()));
    focusItemDef_s_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->mouseEnterText), disk.mouseEnterText);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->mouseExitText), disk.mouseExitText);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->mouseEnter), disk.mouseEnter);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->mouseExit), disk.mouseExit);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->onKey), disk.onKey);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->focusTypeData.data), disk.focusTypeData);
    DB_Expand_Commit(static_cast<int>(sizeof(focusItemDef_s)));
}

static void DB_Expand_animParams(animParamsDef_t *dst, const animParamsDef_t_32 &s)
{
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->name), s.name);
    memcpy(&dst->rectClient, s.rectClient, sizeof(dst->rectClient));
    dst->borderSize = s.borderSize;
    memcpy(dst->foreColor, s.foreColor, sizeof(dst->foreColor));
    memcpy(dst->backColor, s.backColor, sizeof(dst->backColor));
    memcpy(dst->borderColor, s.borderColor, sizeof(dst->borderColor));
    memcpy(dst->outlineColor, s.outlineColor, sizeof(dst->outlineColor));
    dst->textScale = s.textScale;
    dst->rotation = s.rotation;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->onEvent), s.onEvent);
}

void DB_ExpandLoad_animParamsDef_t(animParamsDef_t *dst)
{
    iassert(dst == reinterpret_cast<animParamsDef_t *>(DB_GetStreamPos()));
    animParamsDef_t_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    DB_Expand_animParams(dst, disk);
    DB_Expand_Commit(static_cast<int>(sizeof(animParamsDef_t)));
}

void DB_ExpandLoad_UIAnimInfo(UIAnimInfo *dst)
{
    iassert(dst == reinterpret_cast<UIAnimInfo *>(DB_GetStreamPos()));
    UIAnimInfo_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    dst->animStateCount = disk.animStateCount;
    if (disk.animStateCount < 0 || disk.animStateCount > 4096)
    {
        dst->animStateCount = 0;
        disk.animStates = 0;
    }
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->animStates), disk.animStates);
    DB_Expand_animParams(&dst->currentAnimState, disk.currentAnimState);
    DB_Expand_animParams(&dst->nextAnimState, disk.nextAnimState);
    dst->animating = disk.animating;
    dst->animStartTime = disk.animStartTime;
    dst->animDuration = disk.animDuration;
    DB_Expand_Commit(static_cast<int>(sizeof(UIAnimInfo)));
}

void DB_ExpandLoad_listBoxDef_t(listBoxDef_s *dst)
{
    iassert(dst == reinterpret_cast<listBoxDef_s *>(DB_GetStreamPos()));
    listBoxDef_s_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    dst->mousePos = disk.mousePos;
    dst->cursorPos[0] = disk.cursorPos[0];
    dst->startPos[0] = disk.startPos[0];
    dst->endPos[0] = disk.endPos[0];
    dst->drawPadding = disk.drawPadding;
    dst->elementWidth = disk.elementWidth;
    dst->elementHeight = disk.elementHeight;
    dst->numColumns = disk.numColumns;
    dst->special = disk.special;
    memcpy(dst->columnInfo, disk.columnInfo, sizeof(dst->columnInfo));
    dst->notselectable = disk.notselectable;
    dst->noScrollBars = disk.noScrollBars;
    dst->usePaging = disk.usePaging;
    memcpy(dst->selectBorder, disk.selectBorder, sizeof(dst->selectBorder));
    memcpy(dst->disableColor, disk.disableColor, sizeof(dst->disableColor));
    memcpy(dst->focusColor, disk.focusColor, sizeof(dst->focusColor));
    memcpy(dst->elementHighlightColor, disk.elementHighlightColor, sizeof(dst->elementHighlightColor));
    memcpy(dst->elementBackgroundColor, disk.elementBackgroundColor, sizeof(dst->elementBackgroundColor));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->selectIcon), disk.selectIcon);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->backgroundItemListbox), disk.backgroundItemListbox);
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->highlightTexture), disk.highlightTexture);
    dst->noBlinkingHighlight = disk.noBlinkingHighlight;
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->rows), disk.rows);
    dst->maxRows = disk.maxRows;
    if (disk.maxRows < 0 || disk.maxRows > 4096)
    {
        dst->maxRows = 0;
        disk.rows = 0;
    }
    dst->rowCount = disk.rowCount;
    DB_Expand_Commit(static_cast<int>(sizeof(listBoxDef_s)));
}

void DB_ExpandLoad_multiDef_t(multiDef_s *dst)
{
    iassert(dst == reinterpret_cast<multiDef_s *>(DB_GetStreamPos()));
    multiDef_s_32 disk{};
    DB_Expand_ReadFile(&disk, sizeof(disk));
    memset(dst, 0, sizeof(*dst));
    for (int i = 0; i < 32; ++i)
    {
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->dvarList[i]), disk.dvarList[i]);
        DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->dvarStr[i]), disk.dvarStr[i]);
    }
    memcpy(dst->dvarValue, disk.dvarValue, sizeof(dst->dvarValue));
    dst->count = disk.count;
    dst->actionOnEnterPressOnly = disk.actionOnEnterPressOnly;
    dst->strDef = disk.strDef;
    DB_Expand_Commit(static_cast<int>(sizeof(multiDef_s)));
}

void DB_ExpandLoad_enumDvarDef_t(enumDvarDef_s *dst)
{
    iassert(dst == reinterpret_cast<enumDvarDef_s *>(DB_GetStreamPos()));
    uint32_t name32 = 0;
    DB_Expand_ReadFile(&name32, sizeof(name32));
    memset(dst, 0, sizeof(*dst));
    DB_ExpandPtrStore(reinterpret_cast<uintptr_t *>(&dst->enumDvarName), name32);
    DB_Expand_Commit(static_cast<int>(sizeof(enumDvarDef_s)));
}

#endif // !BO1_ASSETLIST_GATE
#endif // BO1_PORT_X64
