#pragma once
#include <string>
#include <vector>
struct MemoryFile;

struct RopeSavedParticle
{
    float p[3]{}, previous[3]{}, drawPrevious[3]{};
    unsigned flags = 0;
    bool operator==(const RopeSavedParticle &) const = default;
};
struct RopeSavedConstraint
{
    float point[3]{};
    int type = 0, entity = -1, first = 0, second = 0;
    std::string bone;
    bool operator==(const RopeSavedConstraint &) const = default;
};
struct RopeSavedState
{
    int active = 0, particles = 0, constraints = 0, entityAnchors = 0;
    RopeSavedParticle particle[25]{};
    RopeSavedConstraint constraint[30]{};
    int entityAnchor[3]{};
    float mins[3]{}, maxs[3]{}, start[3]{}, end[3]{};
    float segmentLength = 0, length = 0, width = 0, scale = 0, forceScale = 0;
    int visible = 0, flags = 0, health = 0, frame = 0, stable = 0, staticRope = 0;
    int drawSegments = 0, curveSegments = 0;
    unsigned curveFrame = 0;
    float curve[50][3]{};
    std::string material;
    bool operator==(const RopeSavedState &) const = default;
};
struct RopeSavedSystem
{
    int capacity = 0, clock = 0, highWater = 0, mapCount = 0, totalCount = 0;
    std::vector<int> entityRopes;
    std::vector<RopeSavedState> ropes;
    bool operator==(const RopeSavedSystem &) const = default;
};
bool Rope_IsSavedCheckpointValid(const RopeSavedSystem &state);
// Main-thread, quiescent checkpoint operations (codec recovery storage is shared).
bool Rope_CaptureCheckpoint(RopeSavedSystem &state);
bool Rope_CanApplyCheckpoint(const RopeSavedSystem &state);
// Caller must quiesce rope updates/draw consumers and publish saved dynamic and
// client entity owners first. No renderer lease or native SL/material pointer
// is copied. Missing resources/owners reject before native storage is changed.
bool Rope_ApplyCheckpoint(const RopeSavedSystem &state);
void Rope_WriteCheckpoint(MemoryFile *file, const RopeSavedSystem &state);
void Rope_ReadCheckpoint(MemoryFile *file, RopeSavedSystem &state);
