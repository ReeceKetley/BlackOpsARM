// FXP2 already stores typed kinematics instead of physics IDs. Reconnect those
// records to newly allocated world-1 bodies through the normal FX factory.
#include <EffectsCore/fx_draw.h>
#include <xanim/xmodel.h>

static int SP_ClientEffectPhysicalSeed(const FxEffect &effect, const FxElem &elem)
{
    // The same seed calculation used by FX_SpawnEffect_AllocElems. This selects
    // the original model variant/preset without borrowing a saved native ID.
    return (static_cast<unsigned>(elem.msecBegin) + effect.randomSeed + 296u * elem.sequence) % 0x1DFu;
}

static void SP_PreflightClientEffectsPhysics(const SPFxCheckpointStage &stage)
{
    int count = 0;
    for (int i = 0; i < FX_ELEM_POOL_MAX; ++i)
    {
        const auto &owner = stage.physical[i];
        if (!owner.present) continue;
        if (i >= fx_elemLimit || !Phys_IsSavedBodyStateValid(&owner.body))
            Com_Error(ERR_DROP, "Client effects physics rejected its pool/motion owner");
        const int index = SP_NativeHandleIndex(owner.effectHandle, 1024, sizeof(FxEffectContainer) / 4);
        if (index < 0) Com_Error(ERR_DROP, "Client effects physics has no effect owner");
        const auto &effect = stage.buffers.effects[index].effect;
        const auto &elem = stage.buffers.elems[i].item.elem;
        if (!effect.def || elem.defIndex >= effect.def->elemDefCountLooping + effect.def->elemDefCountOneShot + effect.def->elemDefCountEmission)
            Com_Error(ERR_DROP, "Client effects physics lost its definition owner");
        const auto &def = effect.def->elemDefsEA[elem.defIndex];
        if (def.elemType != 7 || !(def.flags & 0x8000000) || elem.physObjId
            || !def.visualCount || (def.visualCount > 1 && !def.visuals.array))
            Com_Error(ERR_DROP, "Client effects physics contains an invalid model/body binding");
        const auto visual = FX_GetElemVisuals(&def, SP_ClientEffectPhysicalSeed(effect, elem));
        if (!visual.model || !visual.model->physPreset)
            Com_Error(ERR_DROP, "Client effects physics has no canonical model preset");
        ++count;
    }
    if (count != stage.physicalParticles)
        Com_Error(ERR_DROP, "Client effects physics owner count differs");
}

static void SP_PublishClientEffectsPhysics(const SPFxCheckpointStage &stage, FxSystem *system)
{
    int restored = 0;
    for (int i = 0; i < fx_elemLimit; ++i)
    {
        const auto &owner = stage.physical[i];
        if (!owner.present) continue;
        auto *effect = &FX_EffectFromHandle(system, owner.effectHandle)->effect;
        auto *elem = &system->elems[i].item.elem;
        const auto *def = &effect->def->elemDefsEA[elem->defIndex];
        // The factory gives ownership directly to the real live element. If a
        // later check drops, normal FX retirement destroys each created body.
        if (elem->physObjId || !FX_SpawnModelPhysics(system, effect, def,
            SP_ClientEffectPhysicalSeed(*effect, *elem), elem, elem->sequence) || !elem->physObjId)
            Com_Error(ERR_DROP, "Client effects physics could not create its canonical body");
        Phys_ApplyBodyState(elem->physObjId, &owner.body);
        if (!Phys_VerifyAppliedBodyState(elem->physObjId, &owner.body))
            Com_Error(ERR_DROP, "Client effects physics live kinematics differ at element %i", i);
        ++restored;
    }
    Com_Printf(15, "Client effects physics: recreated %i real particle bodies, live motion EXACT CANONICAL TRANSFORM\n", restored);
}
