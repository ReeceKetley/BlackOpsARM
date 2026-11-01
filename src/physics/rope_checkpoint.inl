// Included by rope.cpp; canonical storage and factories remain authoritative.
#include "rope_checkpoint.h"
#include <universal/memfile.h>
#include <clientscript/cscr_stringlist.h>
#include <database/db_registry.h>
#include <cmath>
extern unsigned __int8 usedExtraDynEnts[256];

static bool Rope_CheckpointNameValid(const std::string &s)
{ return s.size() <= 255 && s.find('\0') == std::string::npos; }
static bool Rope_CheckpointVectorValid(const float *v)
{ return std::isfinite(v[0]) && std::isfinite(v[1]) && std::isfinite(v[2]); }
static bool Rope_CheckpointOwnerValid(const RopeSavedConstraint &c)
{
    if (c.type == ROPE_CENTITY_CONSTRAINT) return c.entity >= 0 && c.entity < 1024;
    if (c.type != ROPE_DENTITY_CONSTRAINT) return true;
    const int base = cm.originalDynEntCount + 256;
    const int draw = c.entity >= base, index = draw ? c.entity - base : c.entity;
    if (index < 0 || index >= cm.dynEntCount[draw] || !cm.dynEntPoseList[draw] || !cm.dynEntClientList[draw]) return false;
    return draw || index < cm.originalDynEntCount
        || (index < base && usedExtraDynEnts[index - cm.originalDynEntCount]);
}
bool Rope_IsSavedCheckpointValid(const RopeSavedSystem &s)
{
    if (s.capacity < 0 || s.capacity > 4096 || s.clock < 0 || s.highWater < 0 || s.highWater > s.capacity
        || s.mapCount < 0 || s.mapCount > s.totalCount || s.totalCount > s.capacity
        || s.ropes.size() != static_cast<unsigned>(s.highWater) || s.entityRopes.size() > 10) return false;
    unsigned withEntities = 0;
    for (int i = 0; i < s.highWater; ++i)
    {
        const auto &r = s.ropes[i];
        if (!r.active) { if (!(r == RopeSavedState{})) return false; continue; }
        if (r.active != 1 || r.particles < 2 || r.particles > 25 || r.constraints < 0 || r.constraints > 30
            || r.entityAnchors < 0 || r.entityAnchors > 3 || r.visible < 0 || r.visible > 1
            || (r.flags & ~0x1ff) || r.health < 0 || r.frame < 0 || r.stable < 0 || r.staticRope < 0 || r.staticRope > 1
            || r.drawSegments < 0 || r.drawSegments > 49 || r.curveSegments < 0 || r.curveSegments > 49 || r.curveFrame > 1
            || r.material.empty() || !Rope_CheckpointNameValid(r.material)) return false;
        for (const auto *v : {r.mins, r.maxs, r.start, r.end}) if (!Rope_CheckpointVectorValid(v)) return false;
        for (int k = 0; k < 3; ++k) if (r.mins[k] > r.maxs[k]) return false;
        for (float v : {r.segmentLength, r.length, r.width, r.scale, r.forceScale}) if (!std::isfinite(v) || v < 0) return false;
        if (r.scale <= 0 || !std::isfinite(r.length * r.scale)) return false;
        for (int n = 0; n < 25; ++n)
        {
            const auto &p = r.particle[n];
            if (n >= r.particles) { if (!(p == RopeSavedParticle{})) return false; continue; }
            if (!Rope_CheckpointVectorValid(p.p) || !Rope_CheckpointVectorValid(p.previous)
                || !Rope_CheckpointVectorValid(p.drawPrevious) || (p.flags & ~3u)) return false;
        }
        int entityCount = 0;
        for (int n = 0; n < 30; ++n)
        {
            const auto &c = r.constraint[n];
            if (n >= r.constraints) { if (!(c == RopeSavedConstraint{})) return false; continue; }
            if (c.type < ROPE_PAIR_CONSTRAINT || c.type > ROPE_CENTITY_CONSTRAINT || !Rope_CheckpointVectorValid(c.point)
                || c.first < 0 || c.first >= r.particles || c.second < 0 || c.second >= r.particles || !Rope_CheckpointNameValid(c.bone)) return false;
            if (c.type == ROPE_PAIR_CONSTRAINT)
            {
                if (c.first == c.second || c.entity != -1 || !c.bone.empty()) return false;
                for (float v : c.point) if (v != 0) return false;
            }
            else if (c.first != c.second) return false;
            if (c.type == ROPE_WORLD_CONSTRAINT && (c.entity != -1 || !c.bone.empty())) return false;
            if (c.type == ROPE_DENTITY_CONSTRAINT && (c.entity < 0 || c.entity > 65535 || !c.bone.empty())) return false;
            if (c.type == ROPE_CENTITY_CONSTRAINT) { if (c.entity < 0 || c.entity >= 1024) return false; ++entityCount; }
        }
        if (entityCount != r.entityAnchors) return false;
        bool seen[30]{};
        for (int n = 0; n < 3; ++n)
        {
            int index = r.entityAnchor[n];
            if (n >= r.entityAnchors) { if (index) return false; continue; }
            if (index < 0 || index >= r.constraints || seen[index] || r.constraint[index].type != ROPE_CENTITY_CONSTRAINT) return false;
            seen[index] = true;
        }
        for (int n = 0; n < 50; ++n)
        {
            if (!Rope_CheckpointVectorValid(r.curve[n])) return false;
            if (!r.curveSegments || n > r.curveSegments) for (float v : r.curve[n]) if (v != 0) return false;
        }
        withEntities += r.entityAnchors != 0;
    }
    if (withEntities != s.entityRopes.size()) return false;
    bool seen[4096]{};
    for (int i : s.entityRopes)
    {
        if (i < 0 || i >= s.highWater || seen[i] || !s.ropes[i].active || !s.ropes[i].entityAnchors) return false;
        seen[i] = true;
    }
    return true;
}
bool Rope_CaptureCheckpoint(RopeSavedSystem &state)
{
    if (g_max_ropes != cm.max_ropes || g_ropes != cm.ropes || g_max_ropes < 0 || g_max_ropes > 4096
        || (g_max_ropes && !g_ropes) || g_ropeCount < 0 || g_ropeCount > g_max_ropes
        || g_ropesWithEntsAnchorsCount < 0 || g_ropesWithEntsAnchorsCount > 10) return false;
    RopeSavedSystem s; s.capacity = g_max_ropes; s.clock = g_rope_sys_time; s.highWater = g_ropeCount;
    s.mapCount = g_mapRopeCount; s.totalCount = g_totalRopeCount;
    s.entityRopes.assign(g_ropesWithEntsAnchors, g_ropesWithEntsAnchors + g_ropesWithEntsAnchorsCount);
    s.ropes.resize(s.highWater);
    for (int i = 0; i < s.highWater; ++i)
    {
        const auto &r = g_ropes[i]; auto &v = s.ropes[i];
        if (!r.m_in_use) continue;
        if (r.m_in_use != 1 || r.m_num_particles < 2 || r.m_num_particles > 25 || r.m_num_constraints < 0 || r.m_num_constraints > 30
            || r.m_num_entity_anchors < 0 || r.m_num_entity_anchors > 3 || r.m_client_verts.frame_index > 1
            || r.m_dist_constraint || !r.m_material) return false;
        v.active = 1; v.particles = r.m_num_particles; v.constraints = r.m_num_constraints; v.entityAnchors = r.m_num_entity_anchors;
        for (int n = 0; n < v.particles; ++n)
        { memcpy(v.particle[n].p, r.m_particles[n].p, 12); memcpy(v.particle[n].previous, r.m_particles[n].p0, 12); memcpy(v.particle[n].drawPrevious, r.m_particles[n].p_prev, 12); v.particle[n].flags = r.m_particles[n].flags; }
        for (int n = 0; n < v.constraints; ++n)
        {
            const auto &c = r.m_constraints[n]; auto &saved = v.constraint[n];
            saved.type = c.type; saved.first = c.pi1; saved.second = c.pi2;
            if (c.type != ROPE_PAIR_CONSTRAINT) memcpy(saved.point, c.p, 12);
            if (c.type == ROPE_DENTITY_CONSTRAINT || c.type == ROPE_CENTITY_CONSTRAINT) saved.entity = c.entity_index;
            if (c.type == ROPE_CENTITY_CONSTRAINT && c.bone_name_hash)
            {
                if (c.bone_name_hash < 0 || c.bone_name_hash > 65535) return false;
                saved.bone = SL_ConvertToString(c.bone_name_hash, SCRIPTINSTANCE_SERVER);
            }
            if (!Rope_CheckpointOwnerValid(saved)) return false;
        }
        for (int n = 0; n < v.entityAnchors; ++n) v.entityAnchor[n] = r.m_entity_anchors[n];
        memcpy(v.mins, r.m_min, 12); memcpy(v.maxs, r.m_max, 12); memcpy(v.start, r.m_start, 12); memcpy(v.end, r.m_end, 12);
        v.segmentLength = r.m_seglen; v.length = r.m_length; v.width = r.m_width; v.scale = r.m_scale; v.forceScale = r.m_force_scale;
        v.visible = r.m_visible; v.flags = r.m_flags; v.health = r.m_health; v.frame = r.m_frame; v.stable = r.m_stable_count; v.staticRope = r.m_static_rope;
        v.drawSegments = r.m_num_draw_verts; v.curveFrame = r.m_client_verts.frame_index;
        const auto &curve = r.m_client_verts.frame_verts[v.curveFrame]; v.curveSegments = curve.num_verts;
        if (v.curveSegments < 0 || v.curveSegments > 49) return false;
        if (v.curveSegments) memcpy(v.curve, curve.v, (v.curveSegments + 1) * 12);
        v.material = Material_GetName(const_cast<Material *>(r.m_material));
    }
    if (!Rope_IsSavedCheckpointValid(s)) return false;
    state = std::move(s); return true;
}
static Material *Rope_CheckpointMaterial(const std::string &name)
{
    Sys_EnterCriticalSection(CRITSECT_DBHASH);
    const auto *entry = DB_FindXAssetEntry(ASSET_TYPE_MATERIAL, name.c_str());
    Material *material = entry ? entry->entry.asset.header.material : nullptr;
    Sys_LeaveCriticalSection(CRITSECT_DBHASH);
    return material && name == Material_GetName(material) ? material : nullptr;
}
struct RopeCheckpointResources
{
    std::vector<Material *> materials;
    std::vector<unsigned> bones;
};
static bool Rope_ResolveCheckpoint(const RopeSavedSystem &s, RopeCheckpointResources &resources)
{
    if (!Rope_IsSavedCheckpointValid(s) || s.capacity != g_max_ropes || g_max_ropes != cm.max_ropes
        || g_ropes != cm.ropes || (s.capacity && !g_ropes) || s.mapCount != g_mapRopeCount
        || g_ropesWithEntsAnchorsCount < 0 || g_ropesWithEntsAnchorsCount > 10) return false;
    resources.materials.resize(s.highWater); resources.bones.resize(s.highWater * 30);
    auto &materials = resources.materials; auto &bones = resources.bones;
    for (int i = 0; i < s.highWater; ++i) if (s.ropes[i].active)
    {
        materials[i] = Rope_CheckpointMaterial(s.ropes[i].material); if (!materials[i]) return false;
        for (int n = 0; n < s.ropes[i].constraints; ++n)
        {
            const auto &c = s.ropes[i].constraint[n]; if (!Rope_CheckpointOwnerValid(c)) return false;
            if (!c.bone.empty()) { bones[i * 30 + n] = SL_FindString(c.bone.c_str(), SCRIPTINSTANCE_SERVER); if (!bones[i * 30 + n] || bones[i * 30 + n] > 65535) return false; }
        }
    }
    return true;
}
bool Rope_CanApplyCheckpoint(const RopeSavedSystem &s)
{
    RopeCheckpointResources resources;
    return Rope_ResolveCheckpoint(s, resources);
}
bool Rope_ApplyCheckpoint(const RopeSavedSystem &s)
{
    RopeCheckpointResources resources;
    if (!Rope_ResolveCheckpoint(s, resources)) return false;
    const auto &materials = resources.materials; const auto &bones = resources.bones;
    // Everything that can normally reject is resolved before retiring storage.
    // Slot addresses remain canonical; no simulation or settle step is run.
    for (int i = 0; i < g_max_ropes; ++i) { Rope_Reset(i); g_ropes[i] = {}; }
    g_ropesWithEntsAnchorsCount = 0; g_ropeCount = s.highWater;
    for (int i = 0; i < s.highWater; ++i) if (s.ropes[i].active)
    {
        const auto &v = s.ropes[i]; Rope_Create(i);
        Rope_Init(i, v.particles, v.start, v.end, v.length, v.width, v.scale, v.forceScale, v.staticRope);
        Rope_SetMaterial(i, materials[i]); auto &r = g_ropes[i];
        for (int n = 0; n < v.particles; ++n) { memcpy(r.m_particles[n].p, v.particle[n].p, 12); memcpy(r.m_particles[n].p0, v.particle[n].previous, 12); memcpy(r.m_particles[n].p_prev, v.particle[n].drawPrevious, 12); r.m_particles[n].flags = v.particle[n].flags; }
        r.m_num_constraints = v.constraints;
        for (auto &c : r.m_constraints) c = {};
        for (int n = 0; n < v.constraints; ++n)
        {
            const auto &c = v.constraint[n]; auto &native = r.m_constraints[n];
            memcpy(native.p, c.point, 12); native.type = static_cast<rope_constraint_e>(c.type); native.entity_index = c.entity;
            native.bone_name_hash = bones[i * 30 + n]; native.pi1 = static_cast<unsigned char>(c.first); native.pi2 = static_cast<unsigned char>(c.second);
        }
        r.m_num_entity_anchors = v.entityAnchors; memcpy(r.m_entity_anchors, v.entityAnchor, sizeof(v.entityAnchor));
        memcpy(r.m_min, v.mins, 12); memcpy(r.m_max, v.maxs, 12); memcpy(r.m_start, v.start, 12); memcpy(r.m_end, v.end, 12);
        r.m_seglen = v.segmentLength; r.m_length = v.length; r.m_width = v.width; r.m_scale = v.scale; r.m_force_scale = v.forceScale;
        r.m_visible = v.visible; r.m_flags = v.flags; r.m_health = v.health; r.m_frame = v.frame; r.m_stable_count = v.stable; r.m_static_rope = v.staticRope;
        r.m_num_draw_verts = v.drawSegments; r.m_client_verts.frame_index = v.curveFrame;
        auto &curve = r.m_client_verts.frame_verts[v.curveFrame]; curve.num_verts = v.curveSegments;
        if (v.curveSegments) memcpy(curve.v, v.curve, (v.curveSegments + 1) * 12);
        r.m_lightingHandle = 0;
    }
    g_ropesWithEntsAnchorsCount = static_cast<int>(s.entityRopes.size());
    for (int i = 0; i < 10; ++i) g_ropesWithEntsAnchors[i] = i < g_ropesWithEntsAnchorsCount ? s.entityRopes[i] : 0;
    g_rope_sys_time = s.clock; g_mapRopeCount = s.mapCount; g_totalRopeCount = s.totalCount; g_num_contacts = g_update_count = 0;
    RopeSavedSystem check;
    if (!Rope_CaptureCheckpoint(check) || !(check == s)) Com_Error(ERR_DROP, "Rope checkpoint native publication differs");
    return true;
}
static void Rope_CheckpointCodec(MemoryFile *file, RopeSavedSystem &s, bool reading)
{
    const auto value = [&](auto &v) { if (reading) MemFile_ReadData(file, sizeof(v), reinterpret_cast<unsigned char *>(&v)); else MemFile_WriteData(file, sizeof(v), reinterpret_cast<unsigned char *>(&v)); };
    const auto name = [&](std::string &v) { if (reading) v = MemFile_ReadCString(file); else MemFile_WriteCString(file, const_cast<char *>(v.c_str())); if (!Rope_CheckpointNameValid(v)) Com_Error(ERR_DROP, "Rope checkpoint asset name invalid"); };
    int magic = 0x31455052; value(magic); if (magic != 0x31455052) Com_Error(ERR_DROP, "Rope checkpoint schema invalid");
    value(s.capacity); value(s.clock); value(s.highWater); value(s.mapCount); value(s.totalCount);
    int anchored = static_cast<int>(s.entityRopes.size()); value(anchored);
    if (s.capacity < 0 || s.capacity > 4096 || s.highWater < 0 || s.highWater > s.capacity || anchored < 0 || anchored > 10) Com_Error(ERR_DROP, "Rope checkpoint inventory invalid");
    if (reading) { s.entityRopes.resize(anchored); s.ropes.resize(s.highWater); }
    for (auto &index : s.entityRopes) value(index);
    for (auto &r : s.ropes)
    {
        value(r.active); if (r.active != 0 && r.active != 1) Com_Error(ERR_DROP, "Rope checkpoint presence invalid"); if (!r.active) continue;
        value(r.particles); value(r.constraints); value(r.entityAnchors);
        if (r.particles < 2 || r.particles > 25 || r.constraints < 0 || r.constraints > 30 || r.entityAnchors < 0 || r.entityAnchors > 3) Com_Error(ERR_DROP, "Rope checkpoint topology invalid");
        for (int n = 0; n < r.particles; ++n) { value(r.particle[n].p); value(r.particle[n].previous); value(r.particle[n].drawPrevious); value(r.particle[n].flags); }
        for (int n = 0; n < r.constraints; ++n) { auto &c = r.constraint[n]; value(c.type); value(c.point); value(c.entity); value(c.first); value(c.second); name(c.bone); }
        for (int n = 0; n < r.entityAnchors; ++n) value(r.entityAnchor[n]);
        value(r.mins); value(r.maxs); value(r.start); value(r.end); value(r.segmentLength); value(r.length); value(r.width); value(r.scale); value(r.forceScale);
        value(r.visible); value(r.flags); value(r.health); value(r.frame); value(r.stable); value(r.staticRope); value(r.drawSegments); value(r.curveFrame); value(r.curveSegments);
        if (r.curveSegments < 0 || r.curveSegments > 49) Com_Error(ERR_DROP, "Rope checkpoint curve invalid");
        if (r.curveSegments) for (int n = 0; n <= r.curveSegments; ++n) value(r.curve[n]);
        name(r.material);
    }
    if (!Rope_IsSavedCheckpointValid(s)) Com_Error(ERR_DROP, "Rope checkpoint logical graph invalid");
}
void Rope_WriteCheckpoint(MemoryFile *file, const RopeSavedSystem &state)
{
    if (!Rope_IsSavedCheckpointValid(state)) Com_Error(ERR_DROP, "Rope checkpoint invalid capture");
    // Com_Error uses the engine's nonlocal recovery path. Keep codec ownership
    // outside that stack so a truncated/failed operation cannot leak its graph.
    static RopeSavedSystem value;
    value = state; Rope_CheckpointCodec(file, value, false);
}
void Rope_ReadCheckpoint(MemoryFile *file, RopeSavedSystem &state)
{
    static RopeSavedSystem value;
    value = {}; Rope_CheckpointCodec(file, value, true); state = std::move(value);
}
