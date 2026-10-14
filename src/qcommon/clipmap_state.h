#pragma once
struct cplane_s;
struct cNode_t;
struct cbrush_t;
struct __declspec(align(4)) cLeaf_s // sizeof=0x2C
{                                       // XREF: cmodel_t/r cLeaf_t/r
    unsigned __int16 firstCollAabbIndex;
    unsigned __int16 collAabbCount;
    int brushContents;                  // XREF: CM_InitBoxHull+3F/w
    int terrainContents;                // XREF: CM_InitBoxHull+49/w
    float mins[3];                      // XREF: CM_InitBoxHull+5B/w
    // CM_InitBoxHull+6B/w ...
    float maxs[3];                      // XREF: CM_InitBoxHull+8B/w
    // CM_InitBoxHull+9B/w ...
    int leafBrushNode;                  // XREF: CM_InitBoxHull+164/w
    __int16 cluster;
    // padding byte
    // padding byte
};

struct cmodel_t // sizeof=0x48
{                                       // XREF: .data:g_box_model/r
                                        // clipMap_t/r
    float mins[3];
    float maxs[3];
    float radius;
    cLeaf_s leaf;                       // XREF: CM_InitBoxHull+3F/w
    // CM_InitBoxHull+49/w ...
};

struct clipMap_t // sizeof=0x14C
{                                       // XREF: .data:clipMap_t cm/r
    const char *name;                   // XREF: CM_LoadMapData_LoadObj+10/r
                                        // CM_Shutdown(void)+4/r ...
    int isInUse;                        // XREF: DynEnt_ClearCollWorld(DynEntityCollType)+7/r
                                        // CM_LoadMap(char const *,int *)+33/w ...
    int planeCount;                     // XREF: CMod_LoadPlanes(void)+6B/w
                                        // CM_GetPlaneCount(void)+3/r
    cplane_s *planes;                   // XREF: CMod_LoadPlanes(void)+63/w
                                        // CMod_LoadPlanes(void)+70/r ...
    unsigned int numStaticModels;       // XREF: CM_LoadStaticModels(void):loc_6B39D9/w
                                        // CM_LoadStaticModels(void)+499/r ...
    struct cStaticModel_s *staticModelList;    // XREF: CM_LoadStaticModels(void)+53/w
                                        // CM_LoadStaticModels(void)+159/w ...
    unsigned int numMaterials;          // XREF: CMod_LoadMaterials+63/w
                                        // CMod_LoadMaterials+98/r ...
    struct dmaterial_t *materials;             // XREF: CMod_LoadMaterials+5B/w
                                        // CMod_LoadMaterials+74/r ...
    unsigned int numBrushSides;         // XREF: CMod_LoadBrushes+BA/w
    struct cbrushside_t *brushsides;           // XREF: CMod_LoadBrushes+B1/w
                                        // CMod_LoadBrushes+BF/r
    unsigned int numNodes;              // XREF: CG_SightTracePointInternal(int *,float const * const,float const * const,int,trace_t *):loc_53EAB4/r
                                        // CMod_LoadNodes+52/w ...
    cNode_t *nodes;                     // XREF: CMod_LoadNodes+4A/w
                                        // CMod_LoadNodes+57/r ...
    unsigned int numLeafs;              // XREF: CM_LeafCluster(int)+6/r
                                        // CM_LeafCluster(int)+E/r ...
    struct cLeaf_s *leafs;                     // XREF: CM_LeafCluster(int)+40/r
                                        // CMod_LoadLeafs+4A/w ...
    unsigned int leafbrushNodesCount;   // XREF: CMod_LoadBrushRelated+CA/w
                                        // CMod_LoadBrushRelated+D6/r
    struct cLeafBrushNode_s *leafbrushNodes;   // XREF: G_DebugDrawBrushModel(gentity_s *,float const * const,int,int)+EF/r
                                        // ScriptEntCmd_PhysicsLaunch(scr_entref_t)+197/r ...
    unsigned int numLeafBrushes;        // XREF: CMod_LoadLeafBrushes+3E/w
                                        // CM_InitBoxHull+175/r ...
    unsigned __int16 *leafbrushes;      // XREF: CMod_LoadLeafBrushNodes+C4/r
                                        // CMod_LoadLeafBrushNodes+10D/r ...
    unsigned int numLeafSurfaces;       // XREF: CMod_LoadLeafSurfaces+3D/w
    unsigned int *leafsurfaces;         // XREF: CMod_LoadLeafSurfaces+35/w
                                        // CMod_LoadLeafSurfaces+42/r
    unsigned int vertCount;             // XREF: CMod_LoadCollisionVerts+3D/w
    float (*verts)[3];                  // XREF: CMod_LoadCollisionVerts+35/w
                                        // CMod_LoadCollisionVerts+42/r ...
    unsigned int numBrushVerts;         // XREF: CMod_LoadBrushes+153/w
    float (*brushVerts)[3];             // XREF: CMod_LoadBrushes+134/w
                                        // CMod_LoadBrushes+141/r ...
    unsigned int nuinds;                // XREF: CMod_LoadPartitionIndices+3C/w
    unsigned __int16 *uinds;            // XREF: CMod_LoadPartitionIndices+34/w
                                        // CMod_LoadPartitionIndices+41/r ...
    int triCount;                       // XREF: CMod_LoadCollisionTriangles+45/w
                                        // CMod_LoadCollisionPartitions+D7/r
    unsigned __int16 *triIndices;       // XREF: CMod_LoadCollisionTriangles+34/w
                                        // CMod_LoadCollisionTriangles+4A/r ...
    unsigned __int8 *triEdgeIsWalkable; // XREF: CMod_LoadCollisionEdgeWalkable+2C/w
                                        // CMod_LoadCollisionEdgeWalkable+39/r ...
    int borderCount;                    // XREF: CMod_LoadCollisionBorders+3D/w
    struct CollisionBorder *borders;           // XREF: CMod_LoadCollisionBorders+35/w
                                        // CMod_LoadCollisionBorders+42/r ...
    int partitionCount;                 // XREF: CM_InitThreadData+4E/r
                                        // CMod_LoadCollisionPartitions+3D/w ...
    struct CollisionPartition *partitions;     // XREF: CMod_LoadCollisionPartitions+35/w
                                        // CMod_LoadCollisionPartitions+42/r ...
    int aabbTreeCount;                  // XREF: CMod_LoadCollisionAabbTrees+3D/w
    struct CollisionAabbTree *aabbTrees;       // XREF: CMod_GetLeafTerrainContents+37/r
                                        // CMod_LoadCollisionAabbTrees+35/w ...
    unsigned int numSubModels;          // XREF: DynEnt_GetSurfaceType(DynEntityDef const *)+40/r
                                        // DynEnt_Create+254/r ...
    cmodel_t *cmodels;                  // XREF: DynEnt_Create+28D/r
                                        // CMod_LoadSubmodels+4A/w ...
    unsigned __int16 numBrushes;        // XREF: CG_SightTracePoint(int *,float const * const,float const * const,int,trace_t *)+93/r
                                        // CMod_LoadBrushes+F6/w ...
    // padding byte
    // padding byte
    cbrush_t *brushes;                  // XREF: CG_SightTracePoint(int *,float const * const,float const * const,int,trace_t *)+A5/r
                                        // G_DebugDrawBrush_r+77/r ...
    int numClusters;                    // XREF: CMod_LoadVisibility+1F/r
                                        // CMod_LoadVisibility+34/w ...
    int clusterBytes;                   // XREF: CMod_LoadVisibility+2E/w
                                        // CMod_LoadVisibility+45/r ...
    unsigned __int8 *visibility;        // XREF: CMod_LoadVisibility+54/w
                                        // CMod_LoadVisibility+64/r ...
    int vised;                          // XREF: CMod_LoadVisibility:loc_6AA4A8/w
                                        // CM_ClusterPVS(int)+3/r
    struct MapEnts *mapEnts;                   // XREF: CMod_LoadEntityString+11/w
                                        // CM_EntityString(void)+3/r ...
    cbrush_t *box_brush;                // XREF: CM_InitThreadData+78/r
                                        // CM_InitBoxHull+14/w ...
    cmodel_t box_model;                 // XREF: CM_InitThreadData+A8/o
                                        // CM_InitBoxHull+3F/w ...
    unsigned __int16 originalDynEntCount;
                                        // XREF: DynEntCl_Shutdown(int)+2D/r
                                        // DynEntCl_Shutdown(int):loc_5AFE5F/r ...
    unsigned __int16 dynEntCount[4];    // XREF: DynEnt_GetEntityDef(ushort,DynEntityDrawType)+A/r
                                        // DynEnt_GetEntityDef(ushort,DynEntityDrawType)+19/r ...
    // padding byte
    // padding byte
    struct DynEntityDef *dynEntDefList[2];     // XREF: DynEnt_GetEntityDef(ushort,DynEntityDrawType)+4F/r
    struct DynEntityPose *dynEntPoseList[2];   // XREF: DynEnt_LoadEntities(void)+6C/w
    struct DynEntityClient *dynEntClientList[2];
    struct DynEntityServer *dynEntServerList[2];
    struct DynEntityColl *dynEntCollList[4];   // XREF: DynEnt_GetEntityColl(DynEntityCollType,ushort)+79/r
                                        // DynEnt_LoadEntities(void)+94/w ...
    int num_constraints;                // XREF: DynEnt_FixupLightConstraints(int,int,int)+1B/r
                                        // CreateRopes(int)+52/r ...
    struct PhysConstraint *constraints;        // XREF: DynEnt_DestroyPhysics(PhysWorld,DynEntityDef const *,DynEntityClient *)+69/r
                                        // DynEnt_FixupLightConstraints(int,int,int)+30/r ...
    int max_ropes;                      // XREF: DynEnt_LoadEntities(void):loc_5B99EE/w
                                        // DynEnt_LoadEntities(void):loc_5B9B80/r ...
    struct rope_t *ropes;                      // XREF: DynEnt_LoadEntities(void)+1147/w
                                        // Rope_Reset(void)+10/r
    unsigned int checksum;              // XREF: CM_LoadMap(char const *,int *)+40/r
};
extern clipMap_t cm;
clipMap_t *CM_GetClipMap();
