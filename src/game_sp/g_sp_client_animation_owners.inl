// Native storage roots and compiled-definition identities. Never patch VM ints.
#include <cgame/cg_local.h>
#include <game_sp/actor_sp_ext.h>
#include <clientscript/cscr_animtree.h>

template<class F> static void SP_VisitClientAnimRootSlots(F visit)
{
    cg_s *cg = CG_GetLocalClientGlobals(0);
    cgs_t *cgs = CG_GetLocalClientStaticGlobals(0);
    for (int i = 0; i < MAX_CL_ENTNUM; ++i)
        if (SP_ClientAnimRootIndex(0, i)) visit(0, i, &CG_GetEntity(0, i)->tree);
    visit(1, 0, &cg->predictedPlayerEntity.tree);
    for (int i = 0; i < 32; ++i) visit(2, i, &cg->bgs.clientinfo[i].pXAnimTree);
    for (int i = 0; i < 4; ++i) visit(3, i, &cgs->corpseinfo[i].pXAnimTree);
    for (int i = 0; i < MAX_ACTORS; ++i) visit(4, i, &BG_SP_GetActorInfo(&cg->bgs, i)->pXAnimTree);
    for (int i = 0; i < MAX_ACTOR_CORPSES_SP; ++i) visit(5, i, &cgs->actorCorpseInfo[i].pXAnimTree);
    visit(6, 0, &CG_GetLocalClientViewModelInfo(0)->tree);
}
static int SP_AddClientAnimDefinition(const XAnim_s *anims, SPClientAnimations &state, std::vector<const XAnim_s *> &definitions)
{
    if (!anims) return 0;
    int id = 0; for (; id < definitions.size() && definitions[id] != anims; ++id) {}
    if (id == definitions.size())
    {
        definitions.push_back(anims); state.definitions.emplace_back();
        SP_ReadClientAnimDefinition(anims, state.definitions.back());
    }
    return id + 1;
}
static int SP_AddClientAnimTree(const XAnimTree_s *tree, SPClientAnimations &state, std::vector<const XAnimTree_s *> &trees,
    std::vector<const XAnim_s *> &definitions, std::vector<bool> &seen)
{
    if (!tree) return 0;
    int id = 0; for (; id < trees.size() && trees[id] != tree; ++id) {}
    if (id < trees.size()) return id + 1;
    trees.push_back(tree); state.trees.emplace_back(); auto &saved = state.trees.back();
    saved.definition = SP_AddClientAnimDefinition(tree->anims, state, definitions) - 1; saved.instance = tree->inst;
    SP_ReadClientAnimNodes(tree, tree->children, 0, saved, seen, 0);
    return id + 1;
}
static void SP_ReadClientAnimOwners(SPClientAnimations &state, std::vector<const XAnimTree_s *> &trees,
    std::vector<const XAnim_s *> &definitions, std::vector<bool> &seen)
{
    auto &o = state.owners;
    SP_VisitClientAnimRootSlots([&](int kind, int index, XAnimTree_s **slot) {
        if (*slot) o.roots.push_back({kind, index, SP_AddClientAnimTree(*slot, state, trees, definitions, seen)});
    });
    const ViewModelInfo &view = *CG_GetLocalClientViewModelInfo(0);
    o.viewDefinition = SP_AddClientAnimDefinition(view.anims, state, definitions);
    if (view.viewModelDObj)
    {
        o.viewHandle = CG_WeaponDObjHandle(0);
        if (Com_GetClientDObj(o.viewHandle, 0) != view.viewModelDObj)
            Com_Error(ERR_DROP, "Client animation checkpoint viewmodel registry differs");
    }
    o.viewHasTree = view.hasAnimTree; memcpy(o.partBits, view.partBits, sizeof(o.partBits));
    for (int i = 0; i < 2; ++i) { o.hands[i][0] = view.hand[i].iPrevAnim; o.hands[i][1] = view.hand[i].iHandAnimIndex; }
    const bgsAnim_s *data = CG_GetLocalClientGlobals(0)->bgs.animData;
    o.genericDefinition = SP_AddClientAnimDefinition(data ? data->generic_human.tree.anims : nullptr, state, definitions);
    for (int inst = 0; inst < 2; ++inst)
        for (int user = 0; user < 2; ++user)
        {
            if (gScrAnimPub[inst].xanim_num[user] >= MAX_XANIMTREE_NUM)
                Com_Error(ERR_DROP, "Client animation checkpoint invalid compiled registry count");
            for (int index = 1; index <= gScrAnimPub[inst].xanim_num[user]; ++index)
                for (int def = 0; def < definitions.size(); ++def)
                    if (gScrAnimPub[inst].xanim_lookup[user][index].anims == definitions[def])
                        o.registrations.push_back({inst, user, index, def});
        }
}

static std::vector<XAnim_s *> s_resolvedClientAnimDefinitions;
static bgsAnim_s *s_resolvedClientAnimData;
static bool s_clientAnimRootsAttached;
static bgsAnim_s *s_previousClientAnimData;
static ViewModelInfo s_previousClientView;
static std::vector<std::pair<XAnimTree_s **, XAnimTree_s *>> s_attachedClientAnimRoots;

static XAnim_s *SP_ClientRegisteredAnimDefinition(const SPClientAnimRegistration &r)
{
    if (r.index > gScrAnimPub[r.instance].xanim_num[r.user])
        Com_Error(ERR_DROP, "Client animation checkpoint compiled registration missing");
    return r.user ? Scr_GetAnims(r.index, static_cast<scriptInstance_t>(r.instance))
        : Scr_GetClientAnims(r.index, static_cast<scriptInstance_t>(r.instance));
}
static void SP_CheckClientDefinitionIdentity(int index, const XAnim_s *anims)
{
    if (!anims) Com_Error(ERR_DROP, "Client animation checkpoint missing canonical definition");
    static SPClientAnimDefinition current; SP_ReadClientAnimDefinition(anims, current);
    static unsigned char expectedBytes[0x140000], actualBytes[0x140000]; static MemoryFile expected, actual;
    MemFile_InitForWriting(&expected, sizeof(expectedBytes), expectedBytes, true, true);
    MemFile_InitForWriting(&actual, sizeof(actualBytes), actualBytes, true, true);
    SPNativeCheckpointCodec a{nullptr, &expected}, b{nullptr, &actual};
    SP_ClientAnimationDefinition(a, s_clientAnimations.definitions[index]); SP_ClientAnimationDefinition(b, current);
    MemFile_StartSegment(&expected, -1); MemFile_StartSegment(&actual, -1);
    if (expected.bytesUsed != actual.bytesUsed || memcmp(expectedBytes, actualBytes, expected.bytesUsed))
        Com_Error(ERR_DROP, "Client animation checkpoint canonical definition %i differs", index);
}
static void SP_BindClientDefinitionIdentity(int index, XAnim_s *anims)
{
    if (!anims || (s_resolvedClientAnimDefinitions[index] && s_resolvedClientAnimDefinitions[index] != anims))
        Com_Error(ERR_DROP, "Client animation checkpoint compiled definition sharing differs");
    SP_CheckClientDefinitionIdentity(index, anims); s_resolvedClientAnimDefinitions[index] = anims;
}
static void SP_PreflightResolvedClientAnimDefinitions()
{
    for (const auto &r : s_clientAnimations.owners.registrations)
        if (SP_ClientRegisteredAnimDefinition(r) != s_resolvedClientAnimDefinitions[r.definition])
            Com_Error(ERR_DROP, "Client animation checkpoint compiled lookup changed during teardown");
    const int generic = s_clientAnimations.owners.genericDefinition;
    if (generic && (!s_resolvedClientAnimData || s_resolvedClientAnimData->generic_human.tree.anims != s_resolvedClientAnimDefinitions[generic - 1]))
        Com_Error(ERR_DROP, "Client animation checkpoint generic definition changed during teardown");
    for (int i = 0; i < s_resolvedClientAnimDefinitions.size(); ++i)
        if (s_resolvedClientAnimDefinitions[i]) SP_CheckClientDefinitionIdentity(i, s_resolvedClientAnimDefinitions[i]);
}
static void SP_PreflightClientAnimOwners()
{
    const auto &state = s_clientAnimations;
    s_resolvedClientAnimDefinitions.assign(state.definitions.size(), nullptr);
    s_resolvedClientAnimData = CG_GetLocalClientGlobals(0)->bgs.animData;
    for (const auto &r : state.owners.registrations) SP_BindClientDefinitionIdentity(r.definition, SP_ClientRegisteredAnimDefinition(r));
    if (state.owners.genericDefinition)
    {
        if (!s_resolvedClientAnimData) Com_Error(ERR_DROP, "Client animation checkpoint canonical client context unavailable");
        SP_BindClientDefinitionIdentity(state.owners.genericDefinition - 1, s_resolvedClientAnimData->generic_human.tree.anims);
    }
    for (int i = 0; i < s_resolvedClientAnimDefinitions.size(); ++i)
        for (int j = 0; j < i; ++j)
            if (s_resolvedClientAnimDefinitions[i] && s_resolvedClientAnimDefinitions[i] == s_resolvedClientAnimDefinitions[j])
                Com_Error(ERR_DROP, "Client animation checkpoint canonical definitions unexpectedly merged");
    // A model handle has already been validated by FDO1; view pointers must
    // resolve to that real registry owner, never to a fabricated stand-in.
    if (state.owners.viewHandle >= 0)
    {
        bool found = false;
        for (const auto &model : s_clientEntityRegistry.models) found |= model.handle == state.owners.viewHandle;
        if (!found) Com_Error(ERR_DROP, "Client animation checkpoint viewmodel owner absent");
    }
    SP_PreflightResolvedClientAnimDefinitions();
    Com_Printf(15, "Client animation owners: PREFLIGHT canonical compiled identities and native owner graph BEFORE teardown\n");
}
static void SP_DetachClientAnimOwners()
{
    if (!s_clientAnimRootsAttached) return;
    s_clientAnimRootsAttached = false;
    for (auto &root : s_attachedClientAnimRoots) if (*root.first == root.second) *root.first = nullptr;
    s_attachedClientAnimRoots.clear();
    *CG_GetLocalClientViewModelInfo(0) = s_previousClientView;
    CG_GetLocalClientGlobals(0)->bgs.animData = s_previousClientAnimData;
}
static void SP_AttachClientAnimOwners()
{
    if (s_clientAnimRootsAttached) Com_Error(ERR_DROP, "Client animation checkpoint roots already attached");
    auto &o = s_clientAnimations.owners;
    SP_VisitClientAnimRootSlots([](int, int, XAnimTree_s **slot) {
        if (*slot) Com_Error(ERR_DROP, "Client animation checkpoint old native root not retired");
    });
    s_previousClientView = *CG_GetLocalClientViewModelInfo(0);
    s_previousClientAnimData = CG_GetLocalClientGlobals(0)->bgs.animData;
    s_clientAnimRootsAttached = true;
    SP_VisitClientAnimRootSlots([&](int kind, int index, XAnimTree_s **slot) {
        for (const auto &root : o.roots)
            if (root.kind == kind && root.index == index)
            {
                *slot = s_liveClientTrees[root.tree - 1];
                s_attachedClientAnimRoots.emplace_back(slot, *slot);
            }
    });
    auto &view = *CG_GetLocalClientViewModelInfo(0);
    view.anims = o.viewDefinition ? s_liveClientDefinitions[o.viewDefinition - 1].first : nullptr;
    view.viewModelDObj = o.viewHandle < 0 ? nullptr : Com_GetClientDObj(o.viewHandle, 0);
    view.hasAnimTree = o.viewHasTree; memcpy(view.partBits, o.partBits, sizeof(o.partBits));
    for (int i = 0; i < 2; ++i) { view.hand[i].iPrevAnim = o.hands[i][0]; view.hand[i].iHandAnimIndex = o.hands[i][1]; }
    CG_GetLocalClientGlobals(0)->bgs.animData = o.genericDefinition ? s_resolvedClientAnimData : nullptr;
}
static void SP_QueryClientAnimOwners()
{
    int roots = 0, registrations = 0, times = 0;
    SP_VisitClientAnimRootSlots([&](int kind, int index, XAnimTree_s **slot) {
        for (const auto &root : s_clientAnimations.owners.roots)
            if (root.kind == kind && root.index == index)
            {
                if (*slot != s_liveClientTrees[root.tree - 1]) Com_Error(ERR_DROP, "Client animation checkpoint native owner lookup differs");
                if ((kind == 0 || kind == 1) && CScr_GetEntAnimTree(kind == 0 ? CG_GetEntity(0, index) : &CG_GetLocalClientGlobals(0)->predictedPlayerEntity) != *slot)
                    Com_Error(ERR_DROP, "Client animation checkpoint actual script owner query differs");
                ++roots;
            }
    });
    for (const auto &r : s_clientAnimations.owners.registrations)
    {
        XAnim_s *anims = s_liveClientDefinitions[r.definition].first;
        if (SP_ClientRegisteredAnimDefinition(r) != anims
            || (r.user && !Scr_GetAnimsIndex(anims, static_cast<scriptInstance_t>(r.instance))))
            Com_Error(ERR_DROP, "Client animation checkpoint actual compiled script query differs");
        ++registrations;
    }
    for (int i = 0; i < s_clientAnimations.trees.size(); ++i)
    {
        const auto &saved = s_clientAnimations.trees[i];
        std::vector<bool> queried(s_clientAnimations.definitions[saved.definition].entries.size());
        for (const auto &node : saved.nodes)
            if (s_clientAnimations.definitions[saved.definition].entries[node.anim].hasParts && !queried[node.anim])
            {
                queried[node.anim] = true;
                if (static_cast<float>(XAnimGetTime(s_liveClientTrees[i], node.anim)) != node.state.currentAnimTime)
                    Com_Error(ERR_DROP, "Client animation checkpoint actual playback query differs");
                ++times;
            }
    }
    const auto &o = s_clientAnimations.owners;
    if (o.viewHandle >= 0 && CG_GetLocalClientViewModelInfo(0)->viewModelDObj != Com_GetClientDObj(CG_WeaponDObjHandle(0), 0))
        Com_Error(ERR_DROP, "Client animation checkpoint actual weapon model query differs");
    Com_Printf(15, "Client animation owners: RECONNECTED %i actual native root slots, %i canonical script lookups, %i actual playback time queries; owner/definition sharing exact; NO CLIENT FRAMES\n", roots, registrations, times);
}
