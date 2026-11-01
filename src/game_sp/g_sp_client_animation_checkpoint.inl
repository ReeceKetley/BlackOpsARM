// CAN1: definitions and playback graphs, with save-local sharing identities.
#include <clientscript/cscr_memorytree.h>
#include <xanim/xanim.h>
#include <clientscript/cscr_animtree.h>

struct SPClientAnimEntry
{
    unsigned short created = 0, count = 0, first = 0, values = 0, parent = 0, flags = 0, children = 0;
    std::string asset;
    bool hasParts = false, defaultAsset = false;
};
struct SPClientAnimDefinition
{
    std::vector<SPClientAnimEntry> entries;
    std::vector<XAnimParam> params;
};
struct SPClientAnimNode
{
    int parent = 0, anim = 0;
    XAnimState state{};
    unsigned short notifyChild = 0, notifyType = 0;
    short notifyIndex = -1;
    std::string notify;
    std::vector<unsigned char> boneMap;
};
struct SPClientAnimTree
{
    int definition = 0, instance = 0;
    std::vector<SPClientAnimNode> nodes;
};
struct SPClientAnimRoot
{
    int kind = 0, index = 0, tree = 0;
};
struct SPClientAnimRegistration
{
    int instance = 0, user = 0, index = 0, definition = 0;
};
struct SPClientAnimOwners
{
    std::vector<SPClientAnimRoot> roots;
    std::vector<SPClientAnimRegistration> registrations;
    int genericDefinition = 0, viewDefinition = 0, viewHandle = -1, viewHasTree = 0;
    int hands[2][2]{};
    unsigned partBits[5]{};
};
struct SPClientAnimations
{
    int schema = 0x314E4143; // CAN1 remains readable; new captures use CAN2.
    std::vector<SPClientAnimDefinition> definitions;
    std::vector<SPClientAnimTree> trees;
    int bindings[MAX_CL_ENTNUM]{}; // 0 = no tree, otherwise save-local tree + 1
    SPClientAnimOwners owners;
};
static SPClientAnimations s_clientAnimations;
static bool s_clientAnimationsPresent, s_clientModelProbeActive;
static std::vector<XAnimTree_s *> s_liveClientTrees;
static std::vector<std::pair<XAnim_s *, int>> s_liveClientDefinitions;
static bool s_liveClientModels[MAX_CL_ENTNUM];
static int SP_ClientFreeAnimationNodes()
{
    return XAnimGetFreeInfoCount();
}

static int SP_ClientAnimationCount(SPNativeCheckpointCodec &c, int count, int limit)
{
    count = c.Int(count);
    if (count < 0 || count > limit) Com_Error(ERR_DROP, "Client animation checkpoint rejected count");
    return count;
}
static void SP_ClientAnimationDefinition(SPNativeCheckpointCodec &c, SPClientAnimDefinition &def)
{
    const int count = SP_ClientAnimationCount(c, def.entries.size(), 65535);
    const int params = SP_ClientAnimationCount(c, def.params.size(), 32767);
    if (!count) Com_Error(ERR_DROP, "Client animation checkpoint empty definition");
    if (c.reader) { def.entries.resize(count); def.params.resize(params); }
    for (auto &p : def.params)
    {
        // A canonical parameter name is a fixed 16-byte field, possibly without
        // a terminator. Persist its actual bytes rather than reading past it.
        for (char &v : p.strName) c.Field(v);
        c.Field(p.fValue);
        if (!std::isfinite(p.fValue)) Com_Error(ERR_DROP, "Client animation checkpoint nonfinite parameter");
    }
    std::vector<int> parents(count, -1);
    for (int i = 0; i < count; ++i)
    {
        auto &e = def.entries[i];
        c.Field(e.created); c.Field(e.count); c.Field(e.first); c.Field(e.values); c.Field(e.parent);
        c.Field(e.hasParts); c.Field(e.defaultAsset);
        SP_ClientOwnerName(c, e.asset, true);
        c.Field(e.flags); c.Field(e.children);
        if (e.created > 1 || e.parent >= count || (!i && e.parent)
            || (e.first ? e.first - 1 + e.values > params : e.values != 0))
            Com_Error(ERR_DROP, "Client animation checkpoint definition range mismatch");
        if (e.count)
        {
            if (e.hasParts || e.defaultAsset || !e.asset.empty() || !e.children || e.children <= i || e.children + e.count > count)
                Com_Error(ERR_DROP, "Client animation checkpoint invalid blend children");
            for (int j = e.children; j < e.children + e.count; ++j)
            {
                if (parents[j] != -1) Com_Error(ERR_DROP, "Client animation checkpoint shared definition child");
                parents[j] = i;
            }
        }
        else if (e.hasParts)
        {
            XAnimParts *part = DB_FindXAssetHeader(ASSET_TYPE_XANIMPARTS, const_cast<char *>(e.asset.c_str()), false, -1).parts;
            if (!e.created || !part || e.asset != part->name || e.flags || e.children
                || e.defaultAsset != DB_IsXAssetDefault(ASSET_TYPE_XANIMPARTS, e.asset.c_str()))
                Com_Error(ERR_DROP, "Client animation checkpoint missing exact animation asset");
        }
        else if (e.created || e.flags || e.children || e.defaultAsset || !e.asset.empty())
            Com_Error(ERR_DROP, "Client animation checkpoint invalid uncreated entry %i created %i count %i flags %i children %i", i, e.created, e.count, e.flags, e.children);
    }
    for (int i = 1; i < count; ++i)
        if (parents[i] != -1 && def.entries[i].parent != parents[i])
            Com_Error(ERR_DROP, "Client animation checkpoint definition parent differs");
}
static void SP_ClientAnimationTree(SPNativeCheckpointCodec &c, SPClientAnimTree &tree, const SPClientAnimations &state)
{
    c.Field(tree.definition); c.Field(tree.instance);
    if ((unsigned)tree.definition >= state.definitions.size()
        || (tree.instance != SCRIPTINSTANCE_CLIENT && (state.schema != 0x324E4143 || tree.instance != SCRIPTINSTANCE_SERVER)))
        Com_Error(ERR_DROP, "Client animation checkpoint invalid definition/notify instance");
    const auto &def = state.definitions[tree.definition];
    const int count = SP_ClientAnimationCount(c, tree.nodes.size(), g_xAnimInfoCount - 1);
    if (c.reader) tree.nodes.resize(count);
    int roots = 0;
    for (int i = 0; i < count; ++i)
    {
        auto &n = tree.nodes[i]; c.Field(n.parent); c.Field(n.anim);
        if (n.parent < 0 || n.parent > i || (unsigned)n.anim >= def.entries.size() || (!n.parent && ++roots > 1))
            Com_Error(ERR_DROP, "Client animation checkpoint invalid node topology");
        // Bound depth before any native recursive tree consumer sees the graph.
        int depth = 0, at = n.parent;
        while (at) { if (++depth > 256) Com_Error(ERR_DROP, "Client animation checkpoint too deep"); at = tree.nodes[at - 1].parent; }
        const auto finite = [&c](float &v) { c.Field(v); if (!std::isfinite(v)) Com_Error(ERR_DROP, "Client animation checkpoint nonfinite playback"); };
        finite(n.state.currentAnimTime); finite(n.state.oldTime);
        c.Field(n.state.cycleCount); c.Field(n.state.oldCycleCount);
        finite(n.state.goalTime); finite(n.state.goalWeight); finite(n.state.weight); finite(n.state.rate);
        c.Field(n.state.instantWeightChange);
        c.Field(n.notifyChild); c.Field(n.notifyIndex); c.Field(n.notifyType);
        SP_ClientOwnerName(c, n.notify, true);
        if (n.notifyChild >= def.entries.size() || n.notifyIndex < -1)
            Com_Error(ERR_DROP, "Client animation checkpoint invalid notification");
        const int size = SP_ClientAnimationCount(c, n.boneMap.size(), 276);
        if (c.reader) n.boneMap.resize(size);
        for (auto &v : n.boneMap) c.Field(v);
        const auto &entry = def.entries[n.anim];
        if (!entry.count && !entry.hasParts) Com_Error(ERR_DROP, "Client animation checkpoint references uncreated leaf");
        if (!entry.hasParts ? size != 0 : size && (size < 21 || size != 21 + n.boneMap[20]))
            Com_Error(ERR_DROP, "Client animation checkpoint invalid bone map/leaf binding");
        if (size)
        {
            XAnimParts *part = DB_FindXAssetHeader(ASSET_TYPE_XANIMPARTS, const_cast<char *>(entry.asset.c_str()), false, -1).parts;
            if (n.boneMap[20] != part->boneCount[9]) Com_Error(ERR_DROP, "Client animation checkpoint bone map asset length differs");
            for (int j = 21; j < size; ++j)
                if (n.boneMap[j] >= DOBJ_MAX_PARTS) Com_Error(ERR_DROP, "Client animation checkpoint bone index out of range");
        }
        if (n.parent && def.entries[tree.nodes[n.parent - 1].anim].count == 0)
            Com_Error(ERR_DROP, "Client animation checkpoint leaf has children");
    }
}
static bool SP_ClientAnimRootIndex(int kind, int index)
{
    switch (kind)
    {
    case 0: return (unsigned)index < MAX_CL_ENTNUM && index != ENTNUM_CSC_END
        && (!EntNum_IsCsc(index) || (index >= MAX_GENTITIES && index < g_entNumCscEnd));
    case 1: case 6: return index == 0;
    case 2: return (unsigned)index < 32;
    case 3: return (unsigned)index < 4;
    case 4: return (unsigned)index < MAX_ACTORS;
    case 5: return (unsigned)index < MAX_ACTOR_CORPSES_SP;
    default: return false;
    }
}
static void SP_ClientAnimOwnersCodec(SPNativeCheckpointCodec &c, SPClientAnimations &state, std::vector<bool> &used, std::vector<bool> &definitionsUsed)
{
    auto &o = state.owners;
    const int roots = SP_ClientAnimationCount(c, o.roots.size(), MAX_CL_ENTNUM + 128);
    if (c.reader) o.roots.resize(roots);
    bool seen[7][MAX_CL_ENTNUM]{};
    for (auto &root : o.roots)
    {
        c.Field(root.kind); c.Field(root.index); c.Field(root.tree);
        if (!SP_ClientAnimRootIndex(root.kind, root.index) || root.tree < 1 || root.tree > state.trees.size()
            || seen[root.kind][root.index]) Com_Error(ERR_DROP, "Client animation checkpoint invalid owner root");
        seen[root.kind][root.index] = true; used[root.tree - 1] = true;
    }
    const int registrations = SP_ClientAnimationCount(c, o.registrations.size(), 4 * MAX_XANIMTREE_NUM);
    if (c.reader) o.registrations.resize(registrations);
    bool registered[2][2][MAX_XANIMTREE_NUM]{};
    for (auto &r : o.registrations)
    {
        c.Field(r.instance); c.Field(r.user); c.Field(r.index); c.Field(r.definition);
        if ((unsigned)r.instance > 1 || (unsigned)r.user > 1 || r.index < 1 || r.index >= MAX_XANIMTREE_NUM
            || (unsigned)r.definition >= state.definitions.size() || registered[r.instance][r.user][r.index])
            Com_Error(ERR_DROP, "Client animation checkpoint invalid script definition registration");
        registered[r.instance][r.user][r.index] = true;
    }
    c.Field(o.genericDefinition); c.Field(o.viewDefinition); c.Field(o.viewHandle); c.Field(o.viewHasTree);
    for (auto &hand : o.hands) for (int &v : hand) c.Field(v);
    for (auto &v : o.partBits) c.Field(v);
    if (o.genericDefinition < 0 || o.genericDefinition > state.definitions.size()
        || o.viewDefinition < 0 || o.viewDefinition > state.definitions.size()
        || o.viewHandle < -1 || o.viewHandle >= MAX_CL_ENTNUM)
        Com_Error(ERR_DROP, "Client animation checkpoint invalid native definition owner");
    if (o.genericDefinition) definitionsUsed[o.genericDefinition - 1] = true;
    if (o.viewDefinition) definitionsUsed[o.viewDefinition - 1] = true;
    int viewTree = 0;
    for (const auto &root : o.roots)
        if (root.kind == 6 && (o.viewDefinition != state.trees[root.tree - 1].definition + 1
            || (o.viewHandle >= 0 && state.bindings[o.viewHandle] != root.tree)))
            Com_Error(ERR_DROP, "Client animation checkpoint viewmodel owner alias differs");
        else if (root.kind == 6) viewTree = root.tree;
    if (o.viewHandle >= 0 && (o.viewHandle != ENTNUM_CSC_END || !o.viewDefinition || !viewTree || !state.bindings[o.viewHandle]))
        Com_Error(ERR_DROP, "Client animation checkpoint invalid viewmodel DObj");
}
static void SP_ClientAnimationsCodec(SPNativeCheckpointCodec &c, SPClientAnimations &state)
{
    state.schema = c.Int(state.schema);
    if (state.schema != 0x314E4143 && state.schema != 0x324E4143) Com_Error(ERR_DROP, "Client animation checkpoint schema mismatch");
    const int defs = SP_ClientAnimationCount(c, state.definitions.size(), MAX_CL_ENTNUM);
    if (c.reader) state.definitions.resize(defs);
    for (auto &def : state.definitions) SP_ClientAnimationDefinition(c, def);
    const int trees = SP_ClientAnimationCount(c, state.trees.size(), MAX_CL_ENTNUM);
    if (c.reader) state.trees.resize(trees);
    int nodes = 0, bindings = 0;
    std::vector<bool> used(trees), definitionsUsed(defs);
    for (auto &tree : state.trees)
    {
        SP_ClientAnimationTree(c, tree, state); nodes += tree.nodes.size(); definitionsUsed[tree.definition] = true;
        if (nodes >= g_xAnimInfoCount) Com_Error(ERR_DROP, "Client animation checkpoint exceeds node pool");
    }
    for (int i = 0; i < MAX_CL_ENTNUM; ++i)
    {
        c.Field(state.bindings[i]);
        const int id = state.bindings[i];
        if (id < 0 || id > trees) Com_Error(ERR_DROP, "Client animation checkpoint invalid owner binding");
        if (id)
        {
            if (state.trees[id - 1].instance != SCRIPTINSTANCE_CLIENT) Com_Error(ERR_DROP, "Client animation checkpoint model has nonclient notify owner");
            used[id - 1] = true; ++bindings;
        }
    }
    if (state.schema == 0x324E4143) SP_ClientAnimOwnersCodec(c, state, used, definitionsUsed);
    for (bool v : used) if (!v) Com_Error(ERR_DROP, "Client animation checkpoint unowned tree");
    for (bool v : definitionsUsed) if (!v) Com_Error(ERR_DROP, "Client animation checkpoint unused definition");
    Com_Printf(15, "Client animation checkpoint: %s %i model bindings / %i shared trees / %i definitions / %i playback nodes\n", c.reader ? "decoded" : "captured", bindings, trees, defs, nodes);
    if (state.schema == 0x324E4143) Com_Printf(15, "Client animation owners: %s %i native root slots / %i script definition registrations\n", c.reader ? "decoded" : "captured", static_cast<int>(state.owners.roots.size()), static_cast<int>(state.owners.registrations.size()));
}
static void SP_ReadClientAnimDefinition(const XAnim_s *anims, SPClientAnimDefinition &def)
{
    def = {}; def.entries.resize(anims->size); def.params.resize(anims->paramSize);
    for (int i = 0; i < anims->paramSize; ++i) def.params[i] = anims->params[i];
    for (int i = 0; i < anims->size; ++i)
    {
        const auto &e = anims->entries[i]; auto &out = def.entries[i];
        out.created = e.bCreated; out.count = e.numAnims; out.first = e.firstParamIxPlusOne; out.values = e.paramCount; out.parent = e.parent;
        if (IsLeafNode(&e))
        {
            if (e.parts)
            {
                out.hasParts = true; out.asset = e.parts->name;
                out.defaultAsset = DB_IsXAssetDefault(ASSET_TYPE_XANIMPARTS, e.parts->name);
            }
            if (e.bCreated && out.asset.empty()) Com_Printf(15, "Client animation identity: unnamed created leaf %i parts %i frames %i debug %s\n", i, e.parts != nullptr, e.parts ? e.parts->numframes : -1, anims->debugName ? anims->debugName : "<none>");
        }
        else { out.children = e.animParent.children; out.flags = e.animParent.flags; }
    }
}
static void SP_ReadClientAnimNodes(const XAnimTree_s *tree, int node, int parent, SPClientAnimTree &out, std::vector<bool> &seen, int depth)
{
    if (depth > 256) Com_Error(ERR_DROP, "Client animation capture too deep");
    for (; node; node = GetAnimInfo(node)->next)
    {
        if (node < 0 || node >= g_xAnimInfoCount || seen[node]) Com_Error(ERR_DROP, "Client animation capture duplicate/invalid node");
        seen[node] = true; const XAnimInfo &info = *GetAnimInfo(node);
        if (info.streamedParts || info.streamedAnimToModel) Com_Error(ERR_DROP, "Client animation capture streamed ownership unsupported");
        SPClientAnimNode saved; saved.parent = parent; saved.anim = info.animIndex; saved.state = info.state;
        saved.notifyChild = info.notifyChild; saved.notifyIndex = info.notifyIndex; saved.notifyType = info.notifyType;
        if (info.notifyName) saved.notify = SL_ConvertToString(info.notifyName, static_cast<scriptInstance_t>(tree->inst));
        if (info.animToModel)
        {
            const auto *map = reinterpret_cast<const unsigned char *>(SL_ConvertToString(info.animToModel, SCRIPTINSTANCE_SERVER));
            saved.boneMap.assign(map, map + 21 + map[20]);
        }
        out.nodes.push_back(saved); const int id = out.nodes.size();
        SP_ReadClientAnimNodes(tree, info.children, id, out, seen, depth + 1);
    }
}
#include "g_sp_client_animation_owners.inl"

static void SP_ReadClientAnimations(SPClientAnimations &state, int schema = 0x324E4143)
{
    state = {}; state.schema = schema; std::vector<const XAnimTree_s *> trees; std::vector<const XAnim_s *> definitions;
    std::vector<bool> seen(g_xAnimInfoCount);
    for (int i = 0; i < MAX_CL_ENTNUM; ++i)
    {
        const DObj *obj = Com_GetClientDObj(i, 0); if (!obj || !obj->tree) continue;
        state.bindings[i] = SP_AddClientAnimTree(obj->tree, state, trees, definitions, seen);
    }
    if (schema == 0x324E4143) SP_ReadClientAnimOwners(state, trees, definitions, seen);
}
static void SP_WriteClientAnimations(MemoryFile *file)
{
    SP_ReadClientAnimations(s_clientAnimations);
    SPNativeCheckpointCodec c{nullptr, file}; SP_ClientAnimationsCodec(c, s_clientAnimations);
}
static void SP_StageClientAnimations(MemoryFile *reader)
{
    const auto *original = MemFile_GetSegmentAddess(reader, 5); MemFile_MoveToSegment(reader, 5);
    static unsigned char bytes[0x140000]; static MemoryFile writer;
    MemFile_InitForWriting(&writer, sizeof(bytes), bytes, true, true);
    s_clientAnimations = {}; SPNativeCheckpointCodec c{reader, &writer}; SP_ClientAnimationsCodec(c, s_clientAnimations);
    MemFile_StartSegment(&writer, -1); int length; memcpy(&length, original, 4);
    if (length != writer.bytesUsed || memcmp(original, bytes, length)) Com_Error(ERR_DROP, "Client animation checkpoint reserialization differs");
    bool models[MAX_CL_ENTNUM]{};
    for (const auto &owner : s_clientEntityRegistry.models)
    {
        models[owner.handle] = true;
        if (owner.animated != (s_clientAnimations.bindings[owner.handle] != 0)) Com_Error(ERR_DROP, "Client animation checkpoint DObj binding mismatch");
        if (owner.animated)
        {
            XModel *parts[32]; XModelNameMap modelMap[512];
            for (int i = 0; i < owner.parts.size(); ++i)
                parts[i] = DB_FindXAssetHeader(ASSET_TYPE_XMODEL, const_cast<char *>(owner.parts[i].model.c_str()), false, -1).model;
            for (const auto &bone : owner.bones) if (bone.empty()) Com_Error(ERR_DROP, "Client animation checkpoint cannot map unnamed animated bone");
            XAnimInitModelMap(parts, owner.parts.size(), modelMap);
            const auto &tree = s_clientAnimations.trees[s_clientAnimations.bindings[owner.handle] - 1];
            const auto &def = s_clientAnimations.definitions[tree.definition];
            for (const auto &node : tree.nodes)
            {
                if (node.boneMap.empty()) continue; // canonical lazy, not yet mapped leaf
                const auto &entry = def.entries[node.anim];
                XAnimParts *asset = DB_FindXAssetHeader(ASSET_TYPE_XANIMPARTS, const_cast<char *>(entry.asset.c_str()), false, -1).parts;
                const unsigned id = XAnimGetAnimMap(asset, modelMap);
                const auto *map = reinterpret_cast<const unsigned char *>(SL_ConvertToString(id, SCRIPTINSTANCE_SERVER));
                const bool same = node.boneMap.size() == 21 + map[20] && !memcmp(map, node.boneMap.data(), node.boneMap.size());
                SL_RemoveRefToStringOfSize(SCRIPTINSTANCE_SERVER, id, 21 + map[20]);
                if (!same) Com_Error(ERR_DROP, "Client animation checkpoint map differs from canonical model binding");
            }
        }
    }
    for (int i = 0; i < MAX_CL_ENTNUM; ++i)
        if (s_clientAnimations.bindings[i] && !models[i]) Com_Error(ERR_DROP, "Client animation checkpoint missing model owner");
    if (s_clientAnimations.schema == 0x324E4143) SP_PreflightClientAnimOwners();
    s_clientAnimationsPresent = true;
    Com_Printf(15, "Client animation checkpoint: durable RESERIALIZATION IDENTICAL (%i bytes); native publication pending\n", length);
}

static void __cdecl SP_FreeClientProbeTree(void *memory, int bytes, scriptInstance_t inst) { MT_Free(static_cast<unsigned char *>(memory), bytes, inst); }
void G_SP_NotifyClientCheckpointTreeFree(void *memory)
{
    for (auto &tree : s_liveClientTrees) if (tree == memory) tree = nullptr;
    for (auto &root : s_attachedClientAnimRoots) if (root.second == memory) root.second = nullptr;
}
void G_SP_AbortClientModelsCheckpoint()
{
    if (!Sys_IsMainThread()) return;
    SP_RestoreClientControllerScratch();
    SP_AbortClientMovement();
    if (!s_clientModelProbeActive) return;
    s_clientModelProbeActive = false;
    if (s_clientCheckpointFramesActive)
    {
        s_clientCheckpointFramesActive = false;
        SP_PrepareClientNetworkRetirement();
        // Frames may replace models/trees. Retire actual current consumers
        // before releasing any saved definition or tracked remaining header.
        CG_FreeWeapons(0); CG_FreeClientDObjInfo(0); CG_FreeEntityDObjInfo(0);
        CG_ShutdownEntities(0); CG_ShutdownEntity(0, &CG_GetLocalClientGlobals(0)->predictedPlayerEntity, true);
        CG_ShutdownFakeEntities(0); CG_FreeAnimTreeInstances(0);
    }
    SP_DetachClientCharacters();
    SP_DetachClientPredictionOwners();
    SP_DetachClientNetwork();
    SP_DetachClientEntityPoses();
    SP_DetachClientAnimOwners();
    for (int i = 0; i < MAX_CL_ENTNUM; ++i) if (s_liveClientModels[i]) { s_liveClientModels[i] = false; Com_SafeClientDObjFree(i, 0); }
    // DObjFree releases its borrower only; the shared playback tree owns nodes.
    for (XAnimTree_s *tree : s_liveClientTrees) if (tree) XAnimFreeTree(tree, SP_FreeClientProbeTree, SCRIPTINSTANCE_SERVER);
    s_liveClientTrees.clear();
    for (auto &def : s_liveClientDefinitions) if (def.second) { XAnimFreeList(def.first); MT_Free(reinterpret_cast<unsigned char *>(def.first), def.second, SCRIPTINSTANCE_SERVER); }
    s_liveClientDefinitions.clear();
}
static void *__cdecl SP_AllocateClientProbeDefinition(unsigned bytes)
{
    auto *memory = reinterpret_cast<XAnim_s *>(MT_Alloc(bytes, 5, SCRIPTINSTANCE_SERVER));
    s_liveClientDefinitions.emplace_back(memory, bytes); return memory;
}
static void *__cdecl SP_AllocateClientProbeTree(unsigned bytes)
{
    auto *memory = reinterpret_cast<XAnimTree_s *>(MT_Alloc(bytes, 5, SCRIPTINSTANCE_SERVER));
    s_liveClientTrees.push_back(memory); return memory;
}
static void SP_PublishClientAnimations()
{
    if (bgs || s_clientModelProbeActive) Com_Error(ERR_DROP, "Client animation probe requires isolated main-thread animation boundary");
    s_clientModelProbeActive = true;
    if (s_clientAnimations.schema == 0x324E4143) SP_PreflightResolvedClientAnimDefinitions();
    for (int definition = 0; definition < s_clientAnimations.definitions.size(); ++definition)
    {
        const auto &saved = s_clientAnimations.definitions[definition];
        if (s_clientAnimations.schema == 0x324E4143 && s_resolvedClientAnimDefinitions[definition])
        { s_liveClientDefinitions.emplace_back(s_resolvedClientAnimDefinitions[definition], 0); continue; }
        XAnim_s *anims = XAnimCreateAnimsWithValues("saved_client", saved.entries.size(), saved.params.size(), SP_AllocateClientProbeDefinition);
        for (int i = 0; i < saved.entries.size(); ++i)
        {
            const auto &e = saved.entries[i];
            if (e.count) XAnimBlend(anims, i, "saved_blend", e.children, e.count, e.flags);
            else if (e.hasParts) XAnimCreate(anims, i, const_cast<char *>(e.asset.c_str()));
            if (e.first) XAnimSetValueSection(anims, i, e.first - 1, e.values);
        }
        for (int i = 0; i < saved.params.size(); ++i)
            anims->params[i] = saved.params[i]; // fixed-width canonical parameter values, no native ownership
    }
    for (const auto &saved : s_clientAnimations.trees)
    {
        XAnimTree_s *tree = XAnimCreateTree(s_liveClientDefinitions[saved.definition].first, SP_AllocateClientProbeTree);
        tree->inst = saved.instance;
        std::vector<unsigned> map(saved.nodes.size() + 1);
        for (int i = 0; i < saved.nodes.size(); ++i)
        {
            const auto &n = saved.nodes[i];
            const auto boneMap = n.boneMap.empty() ? 0 : SL_GetStringOfSize(SCRIPTINSTANCE_SERVER, reinterpret_cast<char *>(const_cast<unsigned char *>(n.boneMap.data())), 0, n.boneMap.size(), 11);
            const unsigned id = XAnimAllocInfoWithParent(tree, boneMap, n.anim, map[n.parent], 1); map[i + 1] = id;
            XAnimInfo &info = *GetAnimInfo(id); info.state = n.state;
            info.notifyChild = n.notifyChild; info.notifyIndex = n.notifyIndex; info.notifyType = n.notifyType;
            if (!n.notify.empty()) info.notifyName = SL_GetString_(static_cast<scriptInstance_t>(tree->inst), n.notify.c_str(), 0, 6);
        }
    }
}
static XAnimTree_s *SP_ClientModelProbeTree(int handle)
{
    if (!s_clientAnimationsPresent || !s_clientAnimations.bindings[handle]) return nullptr;
    return s_liveClientTrees[s_clientAnimations.bindings[handle] - 1];
}
static void SP_CheckPublishedClientAnimations()
{
    if (s_clientAnimations.schema == 0x324E4143) SP_AttachClientAnimOwners();
    static SPClientAnimations actual; SP_ReadClientAnimations(actual, s_clientAnimations.schema);
    static unsigned char expectedBytes[0x140000], actualBytes[0x140000]; static MemoryFile expected, check;
    MemFile_InitForWriting(&expected, sizeof(expectedBytes), expectedBytes, true, true);
    MemFile_InitForWriting(&check, sizeof(actualBytes), actualBytes, true, true);
    SPNativeCheckpointCodec a{nullptr, &expected}, b{nullptr, &check};
    SP_ClientAnimationsCodec(a, s_clientAnimations); SP_ClientAnimationsCodec(b, actual);
    MemFile_StartSegment(&expected, -1); MemFile_StartSegment(&check, -1);
    if (expected.bytesUsed != check.bytesUsed || memcmp(expectedBytes, actualBytes, expected.bytesUsed))
        Com_Error(ERR_DROP, "Fresh client animation definition/playback/sharing differs");
    if (s_clientAnimations.schema == 0x324E4143) SP_QueryClientAnimOwners();
    Com_Printf(15, "Client animation publication: FRESH PROCESS LIVE RESERIALIZATION IDENTICAL, %i shared trees / %i definitions; NO CLIENT FRAMES\n", static_cast<int>(actual.trees.size()), static_cast<int>(actual.definitions.size()));
}
