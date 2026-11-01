// CCR1 logical character records; native animation/material/tree addresses stay local.
#include <cgame_mp/cg_main_mp.h>
#include <qcommon/actor_model_state.h>
#include <bgame/bg_animation.h>

struct SPClientCharacter
{
    clientInfo_t info{};
    int legs = -1, torso = -1;
    std::string status, rank;
};
struct SPClientActorCharacter
{
    actorInfo_t info{};
    int animation = -1;
    SPActorModelInfo models{};
};
struct SPClientCharacters
{
    SPClientCharacter clients[36];
    std::vector<SPClientActorCharacter> actors;
    std::vector<animation_s> definitions;
    int time = 0, latest = 0, frame = 0, prone[32]{};
};
static SPClientCharacters s_clientCharacters;
static bool s_clientCharactersPresent, s_clientCharactersAttached;

static void SP_CRCOptionalName(SPNativeCheckpointCodec &c, std::string &name)
{
    bool present = !name.empty(); c.Field(present);
    if (present) { const char *s = c.Name(name.c_str()); if (c.reader) name = s; }
    else if (c.reader) name.clear();
}
template<size_t N> static void SP_CRCText(char (&s)[N])
{
    if (!memchr(s, 0, N)) Com_Error(ERR_DROP, "Client character text is unterminated");
}
static void SP_CRCLerp(SPNativeCheckpointCodec &c, lerpFrame_t &v, int &animation)
{
    SP_CNSValue(c, v.yawAngle); SP_CNSValue(c, v.yawing); SP_CNSValue(c, v.pitchAngle); SP_CNSValue(c, v.pitching);
    SP_CNSValue(c, v.animationNumber); SP_CNSValue(c, animation); SP_CNSValue(c, v.animationTime);
    SP_CNSValue(c, v.oldFramePos); SP_CNSValue(c, v.oldFrameYaw); SP_CNSValue(c, v.animSpeedScale); SP_CNSValue(c, v.oldFrameSnapshotTime);
}
static void SP_CRCDefinition(SPNativeCheckpointCodec &c, animation_s &v)
{
    SP_CNSValue(c, v.name); SP_CRCText(v.name);
    SP_CNSValue(c, v.initialLerp); SP_CNSValue(c, v.finalLerp); SP_CNSValue(c, v.forceAnimRate);
    SP_CNSValue(c, v.moveSpeed); SP_CNSValue(c, v.rotSpeed); SP_CNSValue(c, v.duration); SP_CNSValue(c, v.nameHash);
    SP_CNSValue(c, v.flags); SP_CNSValue(c, v.movetype); SP_CNSValue(c, v.stance); SP_CNSValue(c, v.movestatus); SP_CNSValue(c, v.noteType);
}
static void SP_CRCIdentityDefinition(SPNativeCheckpointCodec &c, const animation_s &definition)
{
    // BG_AnimScriptEvent changes the rate to match weapon-event timing.
    // The remaining descriptor fields retain their canonical asset identity.
    auto identity = definition; identity.forceAnimRate = 0; SP_CRCDefinition(c, identity);
}
static void SP_CRCClient(SPNativeCheckpointCodec &c, SPClientCharacter &o)
{
    auto &v = o.info;
#define SP_CRC_FIELD(field) SP_CNSValue(c, v.field)
    SP_CRC_FIELD(infoValid); SP_CRC_FIELD(nextValid); SP_CRC_FIELD(clientNum); SP_CRC_FIELD(name);
    SP_CRC_FIELD(team); SP_CRC_FIELD(oldteam); SP_CRC_FIELD(ffaTeam); SP_CRC_FIELD(rank); SP_CRC_FIELD(prestige);
    SP_CRC_FIELD(needsRevive); SP_CRC_FIELD(perks); SP_CRC_FIELD(xuid); SP_CRC_FIELD(clanAbbrev); SP_CNSValue(c, v.score);
    SP_CRCOptionalName(c, o.status); SP_CRCOptionalName(c, o.rank);
    SP_CRC_FIELD(location); SP_CRC_FIELD(health); SP_CRC_FIELD(model); SP_CRC_FIELD(attachModelNames); SP_CRC_FIELD(attachTagNames);
    SP_CRCLerp(c, v.legs, o.legs); SP_CRCLerp(c, v.torso, o.torso);
    SP_CRC_FIELD(lerpMoveDir); SP_CRC_FIELD(lerpLean); SP_CRC_FIELD(playerAngles); SP_CRC_FIELD(leftHandGun); SP_CRC_FIELD(dobjDirty);
    SP_CPCControllerCodec(c, v.control);
    SP_CRC_FIELD(lastDamageTime); SP_CRC_FIELD(lastStandStartTime); SP_CRC_FIELD(moveType); SP_CRC_FIELD(eventType); SP_CRC_FIELD(clientConditions);
    SP_CRC_FIELD(iDObjWeapon); SP_CRC_FIELD(weaponModel); SP_CRC_FIELD(iDObjOffhandWeapon); SP_CRC_FIELD(offhandWeaponModel);
    SP_CRC_FIELD(iDObjMeleeWeapon); SP_CRC_FIELD(meleeWeaponModel); SP_CRC_FIELD(stanceTransitionTime); SP_CRC_FIELD(turnAnimEndTime);
    SP_CRC_FIELD(turnAnimType); SP_CRC_FIELD(attachedVehEntNum); SP_CRC_FIELD(attachedVehSeat); SP_CRC_FIELD(hideWeapon);
    SP_CRC_FIELD(weaponHideBits); SP_CRC_FIELD(usingKnife); SP_CRC_FIELD(usingGrenade); SP_CRC_FIELD(nextHideWeaponTime);
    SP_CRC_FIELD(vehAnimLastOrg); SP_CRC_FIELD(vehAnimLastAngle); SP_CRC_FIELD(vehAnimLastBone); SP_CRC_FIELD(vehAnimLastTime);
    SP_CRC_FIELD(vehAnimLerpStartTime); SP_CRC_FIELD(vehAnimLerpEndTime); SP_CRC_FIELD(weaponSpinLerp); SP_CRC_FIELD(lastWeaponSpinLerp);
    SP_CRC_FIELD(weaponSpin); SP_CRC_FIELD(minigunWasFiring); SP_CRC_FIELD(clientUIVisibilityFlags); SP_CRC_FIELD(teleportTime); SP_CRC_FIELD(teleportBit);
#undef SP_CRC_FIELD
    SP_CRCText(v.name); SP_CRCText(v.clanAbbrev); SP_CRCText(v.model);
    for (int i = 0; i < 6; ++i) { SP_CRCText(v.attachModelNames[i]); SP_CRCText(v.attachTagNames[i]); }
    if ((v.infoValid && (v.clientNum < 0 || v.clientNum >= 32)) || (v.infoValid != 0 && v.infoValid != 1)
        || (v.nextValid != 0 && v.nextValid != 1)) Com_Error(ERR_DROP, "Client character identity invalid");
}
static void SP_CRCActor(SPNativeCheckpointCodec &c, SPClientActorCharacter &o)
{
    auto &v = o.info;
    SP_CNSValue(c, v.infoValid); SP_CNSValue(c, v.nextValid); SP_CNSValue(c, v.actorNum); SP_CNSValue(c, v.entityNum);
    SP_CNSValue(c, v.animInfo.lerp.bAnimating); SP_CNSValue(c, v.animInfo.lerp.animationState);
    SP_CRCLerp(c, v.animInfo.legs, o.animation); SP_CNSValue(c, v.dobjDirty);
    SP_CNSValue(c, o.models.model); SP_CNSValue(c, o.models.attachModelNames); SP_CNSValue(c, o.models.attachTagNames);
    SP_CNSValue(c, o.models.attachIgnoreCollision); SP_CRCText(o.models.model);
    for (int i = 0; i < 6; ++i) { SP_CRCText(o.models.attachModelNames[i]); SP_CRCText(o.models.attachTagNames[i]); }
    if ((v.infoValid != 0 && v.infoValid != 1) || (v.nextValid != 0 && v.nextValid != 1)
        || (v.infoValid && (v.actorNum < 0 || v.actorNum >= MAX_ACTORS || v.entityNum < 0 || v.entityNum >= MAX_CL_ENTNUM))
        || (o.models.attachIgnoreCollision & ~63u)) Com_Error(ERR_DROP, "Client actor character identity invalid");
}
static void SP_ClientCharactersCodec(SPNativeCheckpointCodec &c, SPClientCharacters &state)
{
    if (c.Int(0x31524343) != 0x31524343) Com_Error(ERR_DROP, "Client character schema invalid");
    int count = c.Int(state.definitions.size()); if (count < 0 || count > 1024) Com_Error(ERR_DROP, "Client character animation count invalid");
    if (c.reader) state.definitions.resize(count);
    for (auto &d : state.definitions) SP_CRCDefinition(c, d);
    SP_CNSValue(c, state.time); SP_CNSValue(c, state.latest); SP_CNSValue(c, state.frame); SP_CNSValue(c, state.prone);
    const auto reference = [&](int id) { if (id < -1 || id >= count) Com_Error(ERR_DROP, "Client character animation borrower invalid"); };
    for (auto &o : state.clients) { SP_CRCClient(c, o); reference(o.legs); reference(o.torso); }
    if (c.Int(MAX_ACTORS + MAX_ACTOR_CORPSES_SP) != MAX_ACTORS + MAX_ACTOR_CORPSES_SP)
        Com_Error(ERR_DROP, "Client actor capacity differs");
    if (c.reader) state.actors.resize(MAX_ACTORS + MAX_ACTOR_CORPSES_SP);
    if (state.actors.size() != MAX_ACTORS + MAX_ACTOR_CORPSES_SP) Com_Error(ERR_DROP, "Client actor capacity missing");
    for (auto &o : state.actors) { SP_CRCActor(c, o); reference(o.animation); }
}
static clientInfo_t *SP_CRCClientSlot(int i)
{
    return i < 32 ? &CG_GetLocalClientGlobals(0)->bgs.clientinfo[i] : &CG_GetLocalClientStaticGlobals(0)->corpseinfo[i - 32];
}
static actorInfo_t *SP_CRCActorSlot(int i)
{
    return i < MAX_ACTORS ? BG_SP_GetActorInfo(&CG_GetLocalClientGlobals(0)->bgs, i) : &CG_GetLocalClientStaticGlobals(0)->actorCorpseInfo[i - MAX_ACTORS];
}
static int SP_CRCAnimationOrdinal(const animation_s *animation, const bgsAnim_s *data)
{
    if (!animation) return -1;
    if (data) for (unsigned i = 0; i < data->animScriptData.numAnimations; ++i)
        if (animation == &data->animScriptData.animations[i]) return i;
    Com_Error(ERR_DROP, "Client character animation has no canonical owner"); return -1;
}
static void SP_CRCReadCurrent(SPClientCharacters &state)
{
    auto &bg = CG_GetLocalClientGlobals(0)->bgs; auto *data = bg.animData;
    if (!data || data->animScriptData.numAnimations > 1024) Com_Error(ERR_DROP, "Client character animation service unavailable");
    state = {}; state.time = bg.time; state.latest = bg.latestSnapshotTime; state.frame = bg.frametime; memcpy(state.prone, bg.proneTime, sizeof(state.prone));
    state.definitions.assign(data->animScriptData.animations, data->animScriptData.animations + data->animScriptData.numAnimations);
    for (int i = 0; i < 36; ++i)
    {
        auto &o = state.clients[i]; o.info = *SP_CRCClientSlot(i);
        o.legs = SP_CRCAnimationOrdinal(o.info.legs.animation, data); o.torso = SP_CRCAnimationOrdinal(o.info.torso.animation, data);
        if (o.info.hStatusIcon) o.status = o.info.hStatusIcon->info.name;
        if (o.info.hRankIcon) o.rank = o.info.hRankIcon->info.name;
        o.info.legs.animation = o.info.torso.animation = nullptr; o.info.hStatusIcon = o.info.hRankIcon = nullptr; o.info.pXAnimTree = nullptr;
    }
    state.actors.resize(MAX_ACTORS + MAX_ACTOR_CORPSES_SP);
    for (int i = 0; i < state.actors.size(); ++i)
    {
        auto &o = state.actors[i]; auto *slot = SP_CRCActorSlot(i); o.info = *slot;
        o.animation = SP_CRCAnimationOrdinal(o.info.animInfo.legs.animation, data);
        o.info.animInfo.legs.animation = nullptr; o.info.pXAnimTree = nullptr;
        o.models = *CG_SP_GetActorModels(0, slot);
    }
}
static Material *SP_CRCMaterial(const std::string &name)
{
    if (name.empty()) return nullptr;
    auto *material = DB_FindXAssetHeader(ASSET_TYPE_MATERIAL, const_cast<char *>(name.c_str()), false, -1).material;
    if (!material || strcmp(material->info.name, name.c_str())) Com_Error(ERR_DROP, "Client character material identity missing");
    return material;
}
static void SP_CRCCheckDefinitions()
{
    auto *data = CG_GetLocalClientGlobals(0)->bgs.animData;
    if (!data || data->animScriptData.numAnimations != s_clientCharacters.definitions.size())
        Com_Error(ERR_DROP, "Client character canonical animation table differs");
    static unsigned char aBytes[0x40000], bBytes[0x40000]; static MemoryFile a, b;
    MemFile_InitForWriting(&a, sizeof(aBytes), aBytes, true, true); MemFile_InitForWriting(&b, sizeof(bBytes), bBytes, true, true);
    SPNativeCheckpointCodec ac{nullptr, &a}, bc{nullptr, &b};
    for (unsigned i = 0; i < s_clientCharacters.definitions.size(); ++i)
    {
        SP_CRCIdentityDefinition(ac, s_clientCharacters.definitions[i]);
        SP_CRCIdentityDefinition(bc, data->animScriptData.animations[i]);
    }
    MemFile_StartSegment(&a, -1); MemFile_StartSegment(&b, -1);
    if (a.bytesUsed != b.bytesUsed || memcmp(aBytes, bBytes, a.bytesUsed))
    {
        for (unsigned i = 0; i < s_clientCharacters.definitions.size(); ++i)
        {
            auto x = s_clientCharacters.definitions[i], y = data->animScriptData.animations[i];
            x.forceAnimRate = y.forceAnimRate = 0;
            // Diagnostic only: locate state that changes across a server restart.
            // The full typed comparison above remains the acceptance gate.
            if (memcmp(&x, &y, sizeof(x)))
            {
                Com_Printf(15, "Client character animation mismatch %u names '%.64s'/'%.64s' nameBytes %i lerp %i/%i %i/%i rate %.9g/%.9g move %.9g/%.9g rot %.9g/%.9g duration %i/%i hash %i/%i flags %i/%i moveType %lld/%lld stance %i/%i status %i/%i note %i/%i\n",
                    i, x.name, y.name, memcmp(x.name, y.name, sizeof(x.name)), x.initialLerp, y.initialLerp, x.finalLerp, y.finalLerp,
                    x.forceAnimRate, y.forceAnimRate, x.moveSpeed, y.moveSpeed, x.rotSpeed, y.rotSpeed, x.duration, y.duration,
                    x.nameHash, y.nameHash, x.flags, y.flags, static_cast<long long>(x.movetype), static_cast<long long>(y.movetype), x.stance, y.stance, x.movestatus, y.movestatus, x.noteType, y.noteType);
                break;
            }
        }
        Com_Error(ERR_DROP, "Client character canonical animation identities differ");
    }
}
static void SP_WriteClientCharacters(MemoryFile *file)
{
    SP_CRCReadCurrent(s_clientCharacters); SPNativeCheckpointCodec c{nullptr, file}; SP_ClientCharactersCodec(c, s_clientCharacters);
    int players = 0, actors = 0; for (auto &o : s_clientCharacters.clients) players += o.info.infoValid != 0;
    for (auto &o : s_clientCharacters.actors) actors += o.info.infoValid != 0;
    Com_Printf(15, "Client characters: captured %i valid player/corpse and %i actor/corpse records, %i canonical animation definitions\n", players, actors, static_cast<int>(s_clientCharacters.definitions.size()));
}
static void SP_DecodeClientCharacters(SPNativeCheckpointCodec &c)
{
    s_clientCharacters = {}; SP_ClientCharactersCodec(c, s_clientCharacters); SP_CRCCheckDefinitions();
    for (auto &o : s_clientCharacters.clients) { SP_CRCMaterial(o.status); SP_CRCMaterial(o.rank); }
    for (int i = 0; i < 36; ++i)
    {
        const auto &a = s_clientCharacters.clients[i].info.control; const auto &b = s_clientPredictionOwners.controls[i];
        if (memcmp(a.angles, b.angles, sizeof(a.angles)) || memcmp(a.tag_origin_angles, b.tag_origin_angles, sizeof(a.tag_origin_angles))
            || memcmp(a.tag_origin_offset, b.tag_origin_offset, sizeof(a.tag_origin_offset)))
            Com_Error(ERR_DROP, "Client character/controller records disagree");
    }
    s_clientCharactersPresent = true;
    Com_Printf(15, "Client characters: canonical animation/material identities validated BEFORE teardown\n");
}
// Leases are RAM-only. Tree roots remain owned by CAN2; controllers by CPC1.
static clientInfo_t s_previousClientCharacters[36];
static std::vector<actorInfo_t> s_previousActorCharacters;
static std::vector<SPActorModelInfo> s_previousActorModels;
static bgs_t s_previousCharacterServices;
static bgsAnim_s *s_characterRateOwner;
static std::vector<float> s_previousCharacterAnimRates;
static void SP_DetachClientCharacters()
{
    if (!s_clientCharactersAttached) return;
    s_clientCharactersAttached = false;
    if (s_characterRateOwner)
        for (int i = 0; i < s_previousCharacterAnimRates.size(); ++i)
            s_characterRateOwner->animScriptData.animations[i].forceAnimRate = s_previousCharacterAnimRates[i];
    s_characterRateOwner = nullptr; s_previousCharacterAnimRates.clear();
    for (int i = 0; i < 36; ++i) *SP_CRCClientSlot(i) = s_previousClientCharacters[i];
    for (int i = 0; i < s_previousActorCharacters.size(); ++i)
    {
        auto *slot = SP_CRCActorSlot(i); CG_SP_SetCorpseModels(0, slot, &s_previousActorModels[i]); *slot = s_previousActorCharacters[i];
    }
    auto &bg = CG_GetLocalClientGlobals(0)->bgs;
    // Preserve the CAN2 roots while restoring service pointers and clock state.
    bg.animData = s_previousCharacterServices.animData; bg.time = s_previousCharacterServices.time;
    bg.latestSnapshotTime = s_previousCharacterServices.latestSnapshotTime; bg.frametime = s_previousCharacterServices.frametime;
    memcpy(bg.proneTime, s_previousCharacterServices.proneTime, sizeof(bg.proneTime));
    bg.GetXModel = s_previousCharacterServices.GetXModel; bg.CreateDObj = s_previousCharacterServices.CreateDObj;
    bg.AttachWeapon = s_previousCharacterServices.AttachWeapon; bg.GetDObj = s_previousCharacterServices.GetDObj;
    bg.SafeDObjFree = s_previousCharacterServices.SafeDObjFree; bg.AllocXAnim = s_previousCharacterServices.AllocXAnim;
    bg.anim_user = s_previousCharacterServices.anim_user; bg.Rand = s_previousCharacterServices.Rand; bg.Random = s_previousCharacterServices.Random;
}
static void SP_AttachClientCharacters()
{
    if (!s_clientCharactersPresent) return;
    if (s_clientCharactersAttached) Com_Error(ERR_DROP, "Client character owners already attached");
    auto &bg = CG_GetLocalClientGlobals(0)->bgs;
    s_previousCharacterServices = bg;
    s_previousActorCharacters.resize(s_clientCharacters.actors.size()); s_previousActorModels.resize(s_clientCharacters.actors.size());
    for (int i = 0; i < 36; ++i) s_previousClientCharacters[i] = *SP_CRCClientSlot(i);
    for (int i = 0; i < s_previousActorCharacters.size(); ++i)
    { auto *slot = SP_CRCActorSlot(i); s_previousActorCharacters[i] = *slot; s_previousActorModels[i] = *CG_SP_GetActorModels(0, slot); }
    s_characterRateOwner = nullptr; s_previousCharacterAnimRates.clear();
    s_clientCharactersAttached = true; CG_InitAnimationServices(0); SP_CRCCheckDefinitions();
    s_previousCharacterAnimRates.resize(s_clientCharacters.definitions.size()); s_characterRateOwner = bg.animData;
    for (int i = 0; i < s_clientCharacters.definitions.size(); ++i)
    {
        s_previousCharacterAnimRates[i] = bg.animData->animScriptData.animations[i].forceAnimRate;
        bg.animData->animScriptData.animations[i].forceAnimRate = s_clientCharacters.definitions[i].forceAnimRate;
    }
    bg.time = s_clientCharacters.time; bg.latestSnapshotTime = s_clientCharacters.latest; bg.frametime = s_clientCharacters.frame;
    memcpy(bg.proneTime, s_clientCharacters.prone, sizeof(bg.proneTime));
    const auto animation = [&](int id) -> animation_s * { return id < 0 ? nullptr : &bg.animData->animScriptData.animations[id]; };
    for (int i = 0; i < 36; ++i)
    {
        auto *slot = SP_CRCClientSlot(i); auto *tree = slot->pXAnimTree; const auto &o = s_clientCharacters.clients[i];
        *slot = o.info; slot->pXAnimTree = tree; slot->legs.animation = animation(o.legs); slot->torso.animation = animation(o.torso);
        slot->hStatusIcon = SP_CRCMaterial(o.status); slot->hRankIcon = SP_CRCMaterial(o.rank);
    }
    for (int i = 0; i < s_clientCharacters.actors.size(); ++i)
    {
        auto *slot = SP_CRCActorSlot(i); auto *tree = slot->pXAnimTree; const auto &o = s_clientCharacters.actors[i];
        CG_SP_SetCorpseModels(0, slot, &o.models); *slot = o.info; slot->pXAnimTree = tree; slot->animInfo.legs.animation = animation(o.animation);
    }
    static SPClientCharacters actual; SP_CRCReadCurrent(actual);
    static unsigned char aBytes[0x40000], bBytes[0x40000]; static MemoryFile a, b;
    MemFile_InitForWriting(&a, sizeof(aBytes), aBytes, true, true); MemFile_InitForWriting(&b, sizeof(bBytes), bBytes, true, true);
    SPNativeCheckpointCodec ac{nullptr, &a}, bc{nullptr, &b}; SP_ClientCharactersCodec(ac, s_clientCharacters); SP_ClientCharactersCodec(bc, actual);
    MemFile_StartSegment(&a, -1); MemFile_StartSegment(&b, -1);
    if (a.bytesUsed != b.bytesUsed || memcmp(aBytes, bBytes, a.bytesUsed)) Com_Error(ERR_DROP, "Client character live readback differs");
    if (!bg.GetXModel || !bg.CreateDObj || !bg.AttachWeapon || !bg.GetDObj || !bg.SafeDObjFree || !bg.AllocXAnim || !bg.Rand || !bg.Random
        || bg.anim_user != 0 || !bg.animData->animScriptData.soundAlias || !bg.animData->animScriptData.playSoundAlias)
        Com_Error(ERR_DROP, "Client character canonical services unavailable");
    int models = 0; for (const auto &owner : s_clientEntityRegistry.models) if (bg.GetDObj(owner.handle, 0)) ++models;
    if (models != s_clientEntityRegistry.models.size()) Com_Error(ERR_DROP, "Client character native model callback differs");
    Com_Printf(15, "Client characters: FRESH PROCESS 36 player/corpse and %i actor/corpse records, %i animation identities, %i actual DObj callback queries LIVE RESERIALIZATION IDENTICAL; NO CLIENT FRAMES\n",
        static_cast<int>(actual.actors.size()), static_cast<int>(actual.definitions.size()), models);
}
