// Explicit developer acceptance probe. Select only already loaded retail FX;
// play them through the normal factory, then pause actual nonzero body owners.
#include <algorithm>
struct SPPhysicalEffectsProbe
{
    std::vector<const FxEffectDef *> definitions;
    int effects = 0, modelElements = 0, physicalElements = 0;
};
static void __cdecl SP_FindPhysicalEffectsProbe(XAssetHeader header, void *data)
{
    const auto *def = header.fx;
    if (!def || !def->name || !def->elemDefsEA) return;
    auto *probe = static_cast<SPPhysicalEffectsProbe *>(data);
    ++probe->effects;
    bool useful = false;
    for (int i = 0; i < def->elemDefCountLooping + def->elemDefCountOneShot; ++i)
    {
        const auto &elem = def->elemDefsEA[i];
        if (elem.elemType == 7) ++probe->modelElements;
        if (elem.elemType == 7 && (elem.flags & 0x8000000)) ++probe->physicalElements;
        if (elem.elemType == 7 && (elem.flags & 0x8000000) && elem.lifeSpanMsec.base > 0
            && elem.visualCount && (elem.visualCount == 1 || elem.visuals.array))
        {
            // Select a normal physical model with a real preset for every
            // possible variant; never invent a model or a collision shape.
            bool valid = true;
            for (int seed = 0; seed < 479; ++seed)
            {
                const auto visual = FX_GetElemVisuals(&elem, seed);
                if (!visual.model || !visual.model->physPreset) { valid = false; break; }
            }
            useful |= valid;
        }
    }
    if (useful) static_cast<SPPhysicalEffectsProbe *>(data)->definitions.push_back(def);
}

void G_SP_ProbeClientEffectsPhysics()
{
    if (!Sys_IsMainThread() || CL_GetLocalClientConnectionState(0) != CA_ACTIVE
        || !G_SP_IsSPLevel() || SP_IsFrontendMapForSave()) return;
    Com_SyncThreads();
    SPPhysicalEffectsProbe probe;
    DB_EnumXAssets_FastFile(ASSET_TYPE_FX, SP_FindPhysicalEffectsProbe, &probe, false);
    std::sort(probe.definitions.begin(), probe.definitions.end(), [](const FxEffectDef *a, const FxEffectDef *b) {
        return strcmp(a->name, b->name) < 0;
    });
    if (Cmd_Argc() == 1)
    {
        Com_Printf(15, "Physical FX probe: %i preloaded model effects available (%i effects, %i model elements, %i physical elements)\n",
            static_cast<int>(probe.definitions.size()), probe.effects, probe.modelElements, probe.physicalElements);
        for (int i = 0; i < probe.definitions.size() && i < 64; ++i)
            Com_Printf(15, "Physical FX probe asset: %s\n", probe.definitions[i]->name);
        return;
    }
    const FxEffectDef *selected = nullptr;
    for (const auto *def : probe.definitions) if (!strcmp(def->name, Cmd_Argv(1))) selected = def;
    if (!selected) { Com_Printf(15, "Physical FX probe: requested effect is not an eligible preloaded model effect\n"); return; }
    const auto *cg = CG_GetLocalClientGlobals(0);
    float origin[3];
    for (int axis = 0; axis < 3; ++axis) origin[axis] = cg->refdef.vieworg[axis] + 24.0f * cg->refdef.viewaxis[0][axis];
    origin[2] += 8.0f;
    const float axis[3][3] = {{1,0,0},{0,1,0},{0,0,1}};
    auto *system = &FX_GetSystem(0)->system;
    const unsigned handle = FX_PlayOrientedEffect(system, selected, system->msecNow, origin, axis);
    const auto *effect = UniqueHandleToEffect(system, handle);
    int bodies = 0, visited = 0;
    if (effect)
        for (int group = 0; group < 3; ++group)
            for (unsigned short h = effect->effect.firstElemHandle[group]; h != 0xFFFF && visited++ < fx_elemLimit;)
            {
                const auto *elem = FX_ElemFromHandle(system, h);
                const auto &def = selected->elemDefsEA[elem->item.elem.defIndex];
                bodies += def.elemType == 7 && (def.flags & 0x8000000) && elem->item.elem.physObjId != 0;
                h = elem->item.shared.nextElemHandleInEffect;
            }
    Com_Printf(15, "Physical FX probe: normal factory played %s handle %u with %i actual body owners\n", selected->name, handle, bodies);
    if (bodies && !Dvar_GetInt("cl_paused")) Cbuf_AddText(0, "pause\n");
}
