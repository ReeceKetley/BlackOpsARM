// FDO1 is a logical registry inventory, not a raw centity/DObj memory image.
// Full centity state and animation playback must be restored before VM frames.
#include <cgame_mp/cg_main_mp.h>
#include <qcommon/dobj_management.h>
#include <database/db_registry.h>
#include <vector>
#include <string>
#include <cgame_mp/cg_animtree_mp.h>
#include <cgame/cg_weapons.h>
#include <cgame_mp/cg_snapshot_mp.h>
#include <cgame/cg_spawn.h>
static bool s_clientCheckpointFramesActive;

struct SPClientModelPart
{
    std::string model, tag;
    bool ignoreCollision = false;
    unsigned char parentBone = 255;
};

struct SPClientModelOwner
{
    int handle = 0;
    bool animated = false;
    unsigned char flags = 0;
    unsigned hide[5]{};
    std::vector<SPClientModelPart> parts;
    std::vector<std::string> bones;
};

struct SPClientEntityRegistry
{
    struct Slot
    {
        bool used = false, linked = false;
        unsigned flags = 0;
        int currentUse = 0, nextUse = 0, type = 0, parent = 1023, tagMode = -1;
        float origin[3]{}, angles[3]{}, axis[4][3]{}, relative[3]{};
        bool anglesSet = false;
        std::string tag;
    } slots[512];
    int end = 0, count = 0, fromMap = 0, fromScript = 0;
    std::vector<SPClientModelOwner> models;
};

static SPClientEntityRegistry s_clientEntityRegistry;

static void SP_ClientOwnerName(SPNativeCheckpointCodec &c, std::string &name, bool optional = false)
{
    bool present = !name.empty();
    if (optional) c.Field(present);
    if (!present && optional) { name.clear(); return; }
    // MemFile_ReadCString uses one shared scratch buffer. Own every name now.
    name = c.Name(name.c_str());
}

static void SP_ClientModelRecord(SPNativeCheckpointCodec &c, SPClientModelOwner &owner)
{
    c.Field(owner.handle); c.Field(owner.animated); c.Field(owner.flags);
    if ((unsigned)owner.handle >= MAX_CL_ENTNUM)
        Com_Error(ERR_DROP, "Client model checkpoint rejected handle");
    for (auto &bits : owner.hide) c.Field(bits);
    const int count = c.Int(static_cast<int>(owner.parts.size()));
    if (count < 1 || count > 32) Com_Error(ERR_DROP, "Client model checkpoint rejected model count");
    if (c.reader) owner.parts.resize(count);
    int boneCount = 0;
    for (auto &part : owner.parts)
    {
        SP_ClientOwnerName(c, part.model); SP_ClientOwnerName(c, part.tag, true);
        c.Field(part.ignoreCollision); c.Field(part.parentBone);
        XModel *model = DB_FindXAssetHeader(ASSET_TYPE_XMODEL, const_cast<char *>(part.model.c_str()), false, -1).model;
        if (!model || DB_IsXAssetDefault(ASSET_TYPE_XMODEL, part.model.c_str()) || part.model != model->name)
            Com_Error(ERR_DROP, "Client model checkpoint missing exact model %s", part.model.c_str());
        boneCount += model->numBones;
    }
    const int bones = c.Int(static_cast<int>(owner.bones.size()));
    if (bones < 1 || bones > DOBJ_MAX_PARTS || bones != boneCount)
        Com_Error(ERR_DROP, "Client model checkpoint rejected bone count");
    if (c.reader) owner.bones.resize(bones);
    int index = 0;
    for (auto &part : owner.parts)
    {
        XModel *model = DB_FindXAssetHeader(ASSET_TYPE_XMODEL, const_cast<char *>(part.model.c_str()), false, -1).model;
        // The attachment name must resolve in an earlier model, as DObjCreate
        // does. Never accept the factory's missing-tag fallback as restoration.
        if (!part.tag.empty())
        {
            bool found = false;
            for (int j = 0; j < index; ++j) found |= owner.bones[j] == part.tag;
            if (!found) Com_Error(ERR_DROP, "Client model checkpoint rejected attachment tag %s", part.tag.c_str());
        }
        if ((part.parentBone == 255) != part.tag.empty()
            || (part.parentBone != 255 && (part.parentBone >= index || owner.bones[part.parentBone] != part.tag)))
            Com_Error(ERR_DROP, "Client model checkpoint rejected attachment bone index");
        for (int j = 0; j < model->numBones; ++j, ++index)
        {
            // Some shipped static models (e.g. zombie_zapper_cagelight) have
            // an unnamed root. Preserve it, then compare to the actual asset.
            SP_ClientOwnerName(c, owner.bones[index], true);
            if (owner.bones[index] != SL_ConvertToString(model->localBoneNames[j], SCRIPTINSTANCE_SERVER))
                Com_Error(ERR_DROP, "Client model checkpoint changed bone order/identity");
        }
    }
}

static void SP_ReadClientModel(int handle, const DObj *obj, SPClientModelOwner &owner)
{
    if (!obj || obj->locked || obj->numModels < 1 || obj->numModels > 32 || obj->numBones > DOBJ_MAX_PARTS)
        Com_Error(ERR_DROP, "Client model checkpoint requires an unlocked valid owner");
    DObjModel_s parts[32]{}; unsigned short count, entnum; XAnimTree_s *tree;
    DObjGetCreateParms(obj, parts, &count, &tree, &entnum);
    if (entnum != handle + 1 || obj->localClientIndex != 0)
        Com_Error(ERR_DROP, "Client model checkpoint owner registry mismatch");
    owner = {}; owner.handle = handle; owner.animated = tree != nullptr; owner.flags = obj->flags;
    memcpy(owner.hide, obj->hidePartBits, sizeof(owner.hide));
    owner.parts.resize(count); owner.bones.resize(obj->numBones);
    for (int j = 0; j < count; ++j)
    {
        owner.parts[j].model = parts[j].model->name;
        if (owner.parts[j].model.empty() || owner.parts[j].model.size() > 255)
            Com_Error(ERR_DROP, "Client model checkpoint model identity handle %i part %i length %i", handle, j, static_cast<int>(owner.parts[j].model.size()));
        if (parts[j].boneName) owner.parts[j].tag = SL_ConvertToString(parts[j].boneName, SCRIPTINSTANCE_SERVER);
        owner.parts[j].ignoreCollision = parts[j].ignoreCollision;
        owner.parts[j].parentBone = reinterpret_cast<const unsigned char *>(obj->localModels + obj->numModels)[j];
    }
    for (int j = 0; j < obj->numBones; ++j)
    {
        const char *name = DObjGetBoneName(obj, j);
        if (!name || strlen(name) > 255)
            Com_Error(ERR_DROP, "Client model checkpoint bone identity handle %i bone %i model %s name '%s'", handle, j, owner.parts[0].model.c_str(), name ? name : "<null>");
        owner.bones[j] = name;
    }
}

static void SP_ReadClientEntityRegistry(SPClientEntityRegistry &state)
{
    state = {};
    state.end = g_entNumCscEnd; state.count = cg_fakeEntitiesInuseCount[0];
    state.fromMap = cg_fakeEntitiesInuseCountFromMap; state.fromScript = cg_fakeEntitiesInuseCountFromLoadScript;
    for (int i = 0; i < 512; ++i)
    {
        auto &slot = state.slots[i]; slot.used = cg_fakeEntitiesInuseArray[i];
        if (!slot.used) continue;
        const auto &fake = cg_fakeEntitiesArray[i]; const auto &cent = fake.cent;
        if (cent.nextState.number != 1024 + i || cent.pose.localClientNum != 0)
            Com_Error(ERR_DROP, "Client entity checkpoint slot identity mismatch");
        slot.flags = fake.flags; slot.currentUse = cent.currentState.useCount; slot.nextUse = cent.nextState.lerp.useCount;
        slot.type = cent.nextState.eType;
        memcpy(slot.origin, cent.pose.origin, sizeof(slot.origin)); memcpy(slot.angles, cent.pose.angles, sizeof(slot.angles));
        slot.linked = cent.linkInfo != nullptr;
        if (slot.linked)
        {
            const auto &link = *cent.linkInfo; slot.parent = link.linkEnt;
            slot.tagMode = link.linkTag > 0 ? 1 : link.linkTag;
            if (link.linkTag > 0) slot.tag = SL_ConvertToString(link.linkTag, SCRIPTINSTANCE_SERVER);
            memcpy(slot.axis, link.axis, sizeof(slot.axis)); memcpy(slot.relative, link.relative_angles, sizeof(slot.relative));
            slot.anglesSet = link.angles_set;
        }
    }
    for (int i = 0; i < MAX_CL_ENTNUM; ++i)
        if (const DObj *obj = Com_GetClientDObj(i, 0))
        {
            state.models.emplace_back(); SP_ReadClientModel(i, obj, state.models.back());
        }
}

static void SP_ClientEntityRegistryCodec(SPNativeCheckpointCodec &c, SPClientEntityRegistry &state)
{
    if (c.Int(0x314F4446) != 0x314F4446) Com_Error(ERR_DROP, "Client entity checkpoint schema mismatch"); // FDO1
    c.Field(state.end); c.Field(state.count); c.Field(state.fromMap); c.Field(state.fromScript);
    if (state.end != g_entNumCscEnd || state.end < 1024 || state.end > 1536
        || state.count < 0 || state.count > 512 || state.fromMap < 0 || state.fromMap > 512
        || state.fromScript < 0 || state.fromScript > 512)
        Com_Error(ERR_DROP, "Client entity checkpoint rejected pool configuration");
    int used = 0, links = 0;
    const auto finite = [&c](float &v) { c.Field(v); if (!std::isfinite(v)) Com_Error(ERR_DROP, "Client entity checkpoint rejected transform"); };
    for (int i = 0; i < 512; ++i)
    {
        auto &slot = state.slots[i]; c.Field(slot.used);
        if (!slot.used) continue;
        ++used;
        if (1024 + i >= state.end) Com_Error(ERR_DROP, "Client entity checkpoint uses network-reserved slot");
        c.Field(slot.flags); c.Field(slot.currentUse); c.Field(slot.nextUse); c.Field(slot.type);
        if (slot.type < 0 || slot.type > ET_EVENTS) Com_Error(ERR_DROP, "Client entity checkpoint rejected type");
        for (auto &v : slot.origin) finite(v);
        for (auto &v : slot.angles) finite(v);
        c.Field(slot.linked);
        if (!slot.linked) continue;
        ++links; c.Field(slot.parent); c.Field(slot.tagMode); SP_ClientOwnerName(c, slot.tag, true);
        if (slot.tagMode < -1 || slot.tagMode > 1 || (slot.tagMode == 1) != !slot.tag.empty())
            Com_Error(ERR_DROP, "Client entity checkpoint rejected link tag mode");
        if ((unsigned)slot.parent >= MAX_CL_ENTNUM || slot.parent == 1024 + i)
            Com_Error(ERR_DROP, "Client entity checkpoint rejected link parent");
        for (auto &row : slot.axis) for (auto &v : row) finite(v);
        for (auto &v : slot.relative) finite(v);
        c.Field(slot.anglesSet);
    }
    if (used != state.count) Com_Error(ERR_DROP, "Client entity checkpoint pool count differs");
    // Check fake-parent reachability/cycles without assuming a network parent
    // must appear in the current snapshot (the engine permits delayed links).
    for (int i = 0; i < 512; ++i)
    {
        bool seen[512]{}; int at = i;
        while (state.slots[at].used && state.slots[at].linked)
        {
            if (seen[at]) Com_Error(ERR_DROP, "Client entity checkpoint cyclic links");
            seen[at] = true; const int parent = state.slots[at].parent;
            if (parent < 1024 || parent >= state.end) break;
            at = parent - 1024;
            if (!state.slots[at].used) Com_Error(ERR_DROP, "Client entity checkpoint missing fake parent");
        }
    }
    const int count = c.Int(static_cast<int>(state.models.size()));
    if (count < 0 || count > MAX_CL_ENTNUM) Com_Error(ERR_DROP, "Client model checkpoint rejected registry size");
    if (c.reader) state.models.resize(count);
    bool seen[MAX_CL_ENTNUM]{}; int animated = 0, models = 0, bones = 0;
    for (auto &owner : state.models)
    {
        SP_ClientModelRecord(c, owner);
        if (seen[owner.handle]) Com_Error(ERR_DROP, "Client model checkpoint duplicate handle");
        seen[owner.handle] = true; animated += owner.animated;
        models += owner.parts.size(); bones += owner.bones.size();
        if (owner.handle >= 1024 && owner.handle < state.end && !state.slots[owner.handle - 1024].used)
            Com_Error(ERR_DROP, "Client model checkpoint has no fake entity owner");
    }
    Com_Printf(15, "Client entity registry: %s %i exact fake slots, %i links, %i DObjs / %i models / %i bones (%i animated); full centity/animation restore pending\n",
        c.reader ? "decoded" : "captured", used, links, count, models, bones, animated);
}

static void SP_WriteClientEntityPoses(MemoryFile *file);
static void SP_DecodeClientEntityPoses(SPNativeCheckpointCodec &c);
static void SP_PreflightClientEntityPoses();
static void SP_WriteClientNetwork(MemoryFile *file);
static void SP_DecodeClientNetwork(SPNativeCheckpointCodec &c);
static void SP_PreflightClientDynLights(int version);
static void SP_WriteClientWeaponSelection(MemoryFile *file);
static void SP_DecodeClientWeaponSelection(SPNativeCheckpointCodec &c);
static void SP_WriteClientHud(MemoryFile *file);
static void SP_DecodeClientHud(SPNativeCheckpointCodec &c);
static void SP_WriteClientBolts(MemoryFile *file);
static void SP_DecodeClientBolts(SPNativeCheckpointCodec &c);
static void SP_StageClientBolts(int version);
static void SP_WriteClientMovement(MemoryFile *file);
static void SP_DecodeClientMovement(SPNativeCheckpointCodec &c);
static void SP_WriteClientCharacters(MemoryFile *file);
static void SP_DecodeClientCharacters(SPNativeCheckpointCodec &c);
static void SP_WriteClientPredictionOwners(MemoryFile *file);
static void SP_DecodeClientPredictionOwners(SPNativeCheckpointCodec &c);
static void SP_WriteClientEntityRegistry(MemoryFile *file)
{
    SP_ReadClientEntityRegistry(s_clientEntityRegistry);
    SPNativeCheckpointCodec c{nullptr, file}; SP_ClientEntityRegistryCodec(c, s_clientEntityRegistry);
    SP_WriteClientEntityPoses(file);
    SP_WriteClientNetwork(file);
    SP_WriteClientPredictionOwners(file);
    SP_WriteClientCharacters(file);
    SP_WriteClientMovement(file);
    SP_WriteClientWeaponSelection(file);
    SP_WriteClientBolts(file);
    SP_WriteClientHud(file);
}

static void SP_DecodeClientGenericMaterials(SPNativeCheckpointCodec &c);
static void SP_StageClientEntityRegistry(MemoryFile *reader, int version)
{
    const auto *original = MemFile_GetSegmentAddess(reader, 4); MemFile_MoveToSegment(reader, 4);
    static unsigned char bytes[0x140000]; static MemoryFile writer;
    MemFile_InitForWriting(&writer, sizeof(bytes), bytes, true, true);
    s_clientEntityRegistry = {};
    SPNativeCheckpointCodec c{reader, &writer}; SP_ClientEntityRegistryCodec(c, s_clientEntityRegistry);
    if (version >= 7) SP_DecodeClientEntityPoses(c);
    if (version >= 8) SP_DecodeClientNetwork(c);
    if (version >= 9) SP_DecodeClientPredictionOwners(c);
    if (version >= 10) SP_DecodeClientCharacters(c);
    if (version >= 11) SP_DecodeClientMovement(c);
    if (version >= 12) SP_DecodeClientWeaponSelection(c);
    if (version >= 13) SP_DecodeClientBolts(c);
    if (version >= 14) SP_DecodeClientHud(c);
    if (version >= 15) SP_DecodeClientGenericMaterials(c);
    MemFile_StartSegment(&writer, -1); int length; memcpy(&length, original, 4);
    if (length != writer.bytesUsed || memcmp(original, bytes, length))
        Com_Error(ERR_DROP, "Client entity registry reserialization differs");
    SP_PreflightClientDynLights(version);
    if (version >= 7) SP_PreflightClientEntityPoses();
    if (version >= 11) SP_StageClientBolts(version);
    Com_Printf(15, "Client entity registry: durable RESERIALIZATION IDENTICAL (%i bytes), native slots not yet published\n", length);
}

static void SP_DetachClientEntityPoses();
static void SP_DetachClientNetwork();
static void SP_PrepareClientNetworkRetirement();
static void SP_DetachClientCharacters();
static void SP_DetachClientPredictionOwners();
static void SP_RestoreClientControllerScratch();
static void SP_AbortClientMovement();
#include "g_sp_client_animation_checkpoint.inl"
#include "g_sp_client_entity_checkpoint.inl"
#include "g_sp_client_network_checkpoint.inl"
#include "g_sp_client_prediction_owners.inl"
#include "g_sp_client_character_checkpoint.inl"
#include "g_sp_client_movement_checkpoint.inl"
#include "g_sp_client_weapon_checkpoint.inl"
#include "g_sp_client_bolt_checkpoint.inl"
#include "g_sp_client_hud_checkpoint.inl"
#include "g_sp_client_generic_materials_checkpoint.inl"

static void SP_VerifyFreshClientModels(bool retain = false)
{
    // This probe runs after canonical old cgame teardown, with no client frames.
    // It deliberately excludes trees until their playback graph is restorable.
    for (const auto &owner : s_clientEntityRegistry.models)
        if (Com_GetClientDObj(owner.handle, 0)) Com_Error(ERR_DROP, "Fresh client model handle still occupied");
    const int freeNodes = SP_ClientFreeAnimationNodes();
    int requiredNodes = 0;
    if (s_clientAnimationsPresent)
        for (const auto &tree : s_clientAnimations.trees) requiredNodes += tree.nodes.size();
    if (freeNodes < requiredNodes) Com_Error(ERR_DROP, "Fresh client animation node pool too small");
    if (s_clientAnimationsPresent) SP_PublishClientAnimations();
    else s_clientModelProbeActive = true;
    int restored = 0, animated = 0, bones = 0;
    for (auto &owner : s_clientEntityRegistry.models)
    {
        if (owner.animated && !s_clientAnimationsPresent) { ++animated; continue; }
        DObjModel_s parts[32]{};
        for (int j = 0; j < owner.parts.size(); ++j)
        {
            auto &part = owner.parts[j];
            parts[j].model = DB_FindXAssetHeader(ASSET_TYPE_XMODEL, const_cast<char *>(part.model.c_str()), false, -1).model;
            // These tags are already owned by the exact resolved XModels.
            // Borrow the existing server string identity; acquire no extra
            // references that could leak across the engine's ERR_DROP longjmp.
            if (!part.tag.empty())
            {
                parts[j].boneName = SL_FindString(part.tag.c_str(), SCRIPTINSTANCE_SERVER);
                if (!parts[j].boneName) Com_Error(ERR_DROP, "Fresh model attachment identity disappeared");
            }
            parts[j].ignoreCollision = part.ignoreCollision;
        }
        DObj *obj = Com_ClientDObjCreate(parts, owner.parts.size(), SP_ClientModelProbeTree(owner.handle), owner.handle, 0);
        s_liveClientModels[owner.handle] = true;
        obj->flags = owner.flags; memcpy(obj->hidePartBits, owner.hide, sizeof(owner.hide));
        static SPClientModelOwner check; SP_ReadClientModel(owner.handle, obj, check);
        static unsigned char expectedBytes[0x20000], actualBytes[0x20000]; static MemoryFile expected, actual;
        MemFile_InitForWriting(&expected, sizeof(expectedBytes), expectedBytes, true, true);
        MemFile_InitForWriting(&actual, sizeof(actualBytes), actualBytes, true, true);
        SPNativeCheckpointCodec a{nullptr, &expected}, b{nullptr, &actual};
        SP_ClientModelRecord(a, owner); SP_ClientModelRecord(b, check);
        MemFile_StartSegment(&expected, -1); MemFile_StartSegment(&actual, -1);
        const bool identical = expected.bytesUsed == actual.bytesUsed && !memcmp(expectedBytes, actualBytes, expected.bytesUsed);
        if (!identical) Com_Error(ERR_DROP, "Fresh client model topology differs");
        ++restored; bones += owner.bones.size();
    }
    SP_AttachClientEntityPoses();
    SP_AttachClientNetwork();
    SP_AttachClientPredictionOwners();
    SP_AttachClientCharacters();
    SP_VerifyClientMovement(retain);
    if (s_clientAnimationsPresent) SP_CheckPublishedClientAnimations();
    if (retain)
    {
        for (const auto &owner : s_clientEntityRegistry.models)
            for (const auto &pose : s_clientEntityPoses.owners)
                if (!pose.kind && pose.index == owner.handle)
                    CG_SetDObjInfo(0, owner.handle, pose.type,
                        DB_FindXAssetHeader(ASSET_TYPE_XMODEL, const_cast<char *>(owner.parts[0].model.c_str()), false, -1).model);
        s_clientCheckpointFramesActive = true;
        Com_Printf(15, "Client models: retained %i actual DObjs and native animation owners for bounded saved CLIENT frames\n", restored);
        return;
    }
    G_SP_AbortClientModelsCheckpoint();
    if (SP_ClientFreeAnimationNodes() != freeNodes) Com_Error(ERR_DROP, "Fresh client animation node retirement differs");
    if (s_clientAnimationsPresent && s_clientAnimations.schema == 0x324E4143) SP_PreflightResolvedClientAnimDefinitions();
    for (const auto &owner : s_clientEntityRegistry.models)
        if (Com_GetClientDObj(owner.handle, 0)) Com_Error(ERR_DROP, "Fresh client model retirement differs");
    Com_Printf(15, "Client models: FRESH PROCESS %i DObjs / %i bones at saved handles LIVE RESERIALIZATION IDENTICAL, canonically retired; %i animated owners NOT restored; NO CLIENT FRAMES\n", restored, bones, animated);
}
