// Android-port diagnostic extension, not the retail SP segment-5 format.
// Only logical animation state is persisted. Native pointers, allocator links,
// streamed caches and old string handles never cross the process boundary.
#include <cmath>

static unsigned short s_saveAnimMap[XANIM_INFO_MAX];
static unsigned short s_saveAnimReverse[XANIM_INFO_MAX];
#if defined(__ANDROID__)
static int s_loadedAnimationMagic = 0x394D4E41;
#endif

static XAnimTree_s *G_SaveAnimationOwner(int kind, int index)
{
    if (kind == 0 && index >= 0 && index < MAX_GENTITIES && g_entities[index].r.inuse)
        return g_entities[index].pAnimTree;
    if (kind == 1 && index >= 0 && index < MAX_ACTORS)
        return G_ActorXAnimTreeSlot(index);
    if (kind == 2 && index >= 0 && index < ARRAY_COUNT(g_scr_data.actorCorpseInfo))
        return g_scr_data.actorCorpseInfo[index].tree;
    Com_Error(ERR_DROP, "Save animation: invalid owner %i:%i", kind, index);
    return nullptr;
}

static void G_WriteAnimationNodes(MemoryFile *file, XAnimTree_s *tree, unsigned int node,
    int depth, bool originalIds, bool *seen, int *count)
{
    if (depth > 256)
        Com_Error(ERR_DROP, "Save animation: tree too deep");
    for (; node; node = (*GetAnimInfo(node)).next)
    {
        if (node >= (unsigned)g_xAnimInfoCount || seen[node])
            Com_Error(ERR_DROP, "Save animation: duplicate/invalid node %u", node);
        seen[node] = true;
        ++*count;
        const XAnimInfo &info = (*GetAnimInfo(node));
        const int id = originalIds ? s_saveAnimReverse[node] : node;
        const int parent = originalIds ? s_saveAnimReverse[info.parent] : info.parent;
        if (!id || (info.parent && !parent) || info.streamedParts || info.streamedAnimToModel)
            Com_Error(ERR_DROP, "Save animation: unmapped node or unsupported streamed animation");
        MemFile_WriteInt(file, id);
        MemFile_WriteInt(file, parent);
        MemFile_WriteInt(file, info.animIndex);
        static_assert(sizeof(XAnimState) == 32, "Animation state wire image");
        MemFile_WriteData(file, sizeof(info.state), (unsigned char *)&info.state);
        MemFile_WriteInt(file, info.notifyChild);
        MemFile_WriteInt(file, info.notifyIndex);
        MemFile_WriteInt(file, info.notifyType);
        MemFile_WriteCString(file, info.notifyName ? SL_ConvertToString(info.notifyName, SCRIPTINSTANCE_SERVER) : (char *)"");
        // Canonical XAnim maps are binary interned strings: 20 part-bit bytes,
        // one bone count byte and one byte per bone. Preserve bytes, not handles.
        const unsigned char *map = info.animToModel
            ? (unsigned char *)SL_ConvertToString(info.animToModel, SCRIPTINSTANCE_SERVER) : nullptr;
        const int length = map ? 21 + map[20] : 0;
        MemFile_WriteInt(file, length);
        if (length)
            MemFile_WriteData(file, length, (unsigned char *)map);
        G_WriteAnimationNodes(file, tree, info.children, depth + 1, originalIds, seen, count);
    }
}

static void G_WriteSaveAnimations(MemoryFile *file, bool originalIds)
{
#if defined(__ANDROID__)
    const int magic = originalIds ? s_loadedAnimationMagic : SP_NewSaveAnimationMagic();
    MemFile_WriteInt(file, magic); // ANM9 adds placed dynamic map objects.
    Com_Printf(15, "Save animation: format %08x client DynEnt inventory %i\n", magic, magic == 0x394D4E41);
    if (magic >= 0x374D4E41) SP_WriteServerBodies(file);
    if (magic >= 0x384D4E41) SP_WriteGlass(file);
    if (magic == 0x394D4E41) SP_DynEntCodec(file, false);
#else
    MemFile_WriteInt(file, 0x364D4E41); // ANM6, extension version 6
#endif
    G_WriteSaveConfigstrings(file);
    static bool seen[XANIM_INFO_MAX];
    memset(seen, 0, sizeof(seen));
    int trees = 0, nodes = 0;
    for (int kind = 0; kind < 3; ++kind)
    {
        const int limit = kind == 0 ? MAX_GENTITIES : kind == 1 ? MAX_ACTORS : ARRAY_COUNT(g_scr_data.actorCorpseInfo);
        for (int index = 0; index < limit; ++index)
        {
            if (kind == 0 && !g_entities[index].r.inuse)
                continue;
            XAnimTree_s *tree = G_SaveAnimationOwner(kind, index);
            if (!tree)
                continue;
            if (tree->inst != SCRIPTINSTANCE_SERVER)
                Com_Error(ERR_DROP, "Save animation: non-server tree");
            MemFile_WriteInt(file, kind);
            MemFile_WriteInt(file, index);
            MemFile_WriteInt(file, Scr_GetAnimsIndex(tree->anims, SCRIPTINSTANCE_SERVER));
            G_WriteAnimationNodes(file, tree, tree->children, 0, originalIds, seen, &nodes);
            MemFile_WriteInt(file, -1);
            ++trees;
        }
    }
    MemFile_WriteInt(file, -1);
    G_SP_WriteSavedAnimCommandOwners(file);
    Destructible_WriteSaveState(file);
    G_WriteSaveRandomState(file, originalIds);
    Com_Printf(15, "Save animation: wrote %i trees, %i nodes%s\n", trees, nodes, originalIds ? " with original IDs" : "");
}

static int G_ReadAnimationInt(MemoryFile *file)
{
    int value;
    MemFile_ReadData(file, 4, (unsigned char *)&value);
    return value;
}

static void G_LoadSaveAnimations(SPSaveGame *save)
{
    SP_SaveMemory_MoveToSegment(save, 5, true);
    MemoryFile *file = &save->memFile[0];
    const int magic = G_ReadAnimationInt(file);
#if defined(__ANDROID__)
    s_loadedAnimationMagic = magic;
    if (magic < 0x384D4E41) { s_savedGlass.clear(); s_savedGlassPresent = false; }
    if (magic < 0x394D4E41) { s_savedDynEnts.clear(); s_savedDynBindings.clear(); s_savedDynEntsPresent = false; }
    if (magic == 0x374D4E41 || magic == 0x384D4E41 || magic == 0x394D4E41) SP_ReadServerBodies(file);
    if (magic == 0x384D4E41 || magic == 0x394D4E41) SP_ReadGlass(file);
    if (magic == 0x394D4E41) SP_DynEntCodec(file, true);
    if (magic != 0x394D4E41 && s_serverBodiesStaged && s_serverBodySchema == 0x33505053)
        Com_Error(ERR_DROP, "Unified physics checkpoint missing paired dynamic registry");
    if (magic != 0x394D4E41 && magic != 0x364D4E41 && magic != 0x374D4E41 && magic != 0x384D4E41)
#else
    if (magic != 0x364D4E41)
#endif
        Com_Error(ERR_DROP, "Save animation: missing/unsupported animation section; capture a new diagnostic save");
    G_LoadSaveConfigstrings(file);
    memset(s_saveAnimMap, 0, sizeof(s_saveAnimMap));
    memset(s_saveAnimReverse, 0, sizeof(s_saveAnimReverse));
    static bool seenOwners[3][MAX_GENTITIES];
    memset(seenOwners, 0, sizeof(seenOwners));
    int trees = 0, nodes = 0;
    for (;;)
    {
        const int kind = G_ReadAnimationInt(file);
        if (kind == -1)
            break;
        const int index = G_ReadAnimationInt(file);
        XAnimTree_s *tree = G_SaveAnimationOwner(kind, index);
        if (!tree || seenOwners[kind][index] || tree->inst != SCRIPTINSTANCE_SERVER)
            Com_Error(ERR_DROP, "Save animation: missing/duplicate tree %i:%i", kind, index);
        seenOwners[kind][index] = true;
        if (G_ReadAnimationInt(file) != Scr_GetAnimsIndex(tree->anims, SCRIPTINSTANCE_SERVER))
            Com_Error(ERR_DROP, "Save animation: animation definition mismatch");
        XAnimClearTree(tree);
        bool root = false;
        for (;;)
        {
            const int id = G_ReadAnimationInt(file);
            if (id == -1)
                break;
            const int parent = G_ReadAnimationInt(file);
            const int anim = G_ReadAnimationInt(file);
            if (id <= 0 || id >= g_xAnimInfoCount || s_saveAnimMap[id]
                || parent < 0 || parent >= g_xAnimInfoCount || (parent && !s_saveAnimMap[parent])
                || anim < 0 || (unsigned)anim >= tree->anims->size || (!parent && root))
                Com_Error(ERR_DROP, "Save animation: invalid node topology");
            if (parent && !s_saveAnimReverse[s_saveAnimMap[parent]])
                Com_Error(ERR_DROP, "Save animation: missing parent");
            // Parent must belong to this tree, not merely exist in another owner.
            unsigned int ancestor = parent ? s_saveAnimMap[parent] : 0;
            while (ancestor && (*GetAnimInfo(ancestor)).parent)
                ancestor = (*GetAnimInfo(ancestor)).parent;
            if (parent && ancestor != tree->children)
                Com_Error(ERR_DROP, "Save animation: parent belongs to another tree");
            XAnimState state;
            MemFile_ReadData(file, sizeof(state), (unsigned char *)&state);
            if (!std::isfinite(state.currentAnimTime) || !std::isfinite(state.oldTime)
                || !std::isfinite(state.goalTime) || !std::isfinite(state.goalWeight)
                || !std::isfinite(state.weight) || !std::isfinite(state.rate)
                || ((unsigned char *)&state)[offsetof(XAnimState, instantWeightChange)] > 1)
                Com_Error(ERR_DROP, "Save animation: non-finite playback state");
            const int child = G_ReadAnimationInt(file);
            const int notifyIndex = G_ReadAnimationInt(file);
            const int notifyType = G_ReadAnimationInt(file);
            if (child < 0 || (unsigned)child >= tree->anims->size || notifyIndex < -1 || notifyIndex > 32767
                || notifyType < 0 || notifyType > 65535)
                Com_Error(ERR_DROP, "Save animation: invalid notification");
            const char *name = MemFile_ReadCString(file);
            const int length = G_ReadAnimationInt(file);
            unsigned char map[276];
            if (length && (length < 21 || length > sizeof(map)))
                Com_Error(ERR_DROP, "Save animation: invalid bone map size");
            unsigned short mapId = 0;
            if (length)
            {
                MemFile_ReadData(file, length, map);
                if (length != 21 + map[20])
                    Com_Error(ERR_DROP, "Save animation: invalid bone map");
                mapId = SL_GetStringOfSize(SCRIPTINSTANCE_SERVER, (char *)map, 0, length, 11);
            }
            if ((length != 0) != IsLeafNode(&tree->anims->entries[anim]))
                Com_Error(ERR_DROP, "Save animation: leaf/bone map mismatch");
            const unsigned int notify = *name ? SL_GetString_(SCRIPTINSTANCE_SERVER, name, 0, 6) : 0;
            const unsigned int live = XAnimAllocInfoWithParent(tree, mapId, anim, s_saveAnimMap[parent], 1);
            s_saveAnimMap[id] = live;
            s_saveAnimReverse[live] = id;
            XAnimInfo &info = (*GetAnimInfo(live));
            memcpy(&info.state, &state, sizeof(state));
            info.notifyChild = child;
            info.notifyIndex = notifyIndex;
            info.notifyType = notifyType;
            info.notifyName = notify;
            root |= parent == 0;
            ++nodes;
        }
        ++trees;
    }
    G_SP_ReadSavedAnimCommandOwners(file);
    Destructible_ReadSaveState(file);
    G_ReadSaveRandomState(file);
    G_SP_InstallSavedAnimCommandNodes(s_saveAnimMap, g_xAnimInfoCount);
    Com_Printf(15, "loadgame animation: installed %i trees, %i nodes; NO gameplay resumed\n", trees, nodes);
}

static void G_CheckRestoredAnimations(SPSaveGame *save, const char *stage)
{
    MemoryFile *file = &save->memFile[0];
    // Reserialize with original logical IDs; native pool positions may differ.
    static unsigned char checkBuffer[0x200000];
    static MemoryFile writer;
    MemFile_InitForWriting(&writer, sizeof(checkBuffer), checkBuffer, true, true);
    G_WriteSaveAnimations(&writer, true);
    MemFile_StartSegment(&writer, -1);
    const unsigned char *original = MemFile_GetSegmentAddess(file, 5);
    int length;
    memcpy(&length, original, sizeof(length));
    const bool same = !writer.memoryOverflow && length == writer.bytesUsed && !memcmp(original, checkBuffer, length);
    Com_Printf(15, "loadgame animation: %s reserialization %s (%i bytes); NO gameplay resumed\n",
        stage, same ? "identical" : "DIFFERENT", length);
    iassert(same);
}
