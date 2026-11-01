// CNS1 typed state codecs. No native structure images or objective pointers.
template<class T> static void SP_CNSValue(SPNativeCheckpointCodec &c, T &value)
{
    static_assert(std::is_arithmetic_v<T> || std::is_enum_v<T>);
    c.Field(value);
    if constexpr (std::is_floating_point_v<T>)
        if (!std::isfinite(value)) Com_Error(ERR_DROP, "Client network checkpoint rejected nonfinite value");
}
static void SP_CNSValue(SPNativeCheckpointCodec &c, AmmoPool &v);
static void SP_CNSValue(SPNativeCheckpointCodec &c, AmmoClip &v);
static void SP_CNSValue(SPNativeCheckpointCodec &c, SprintState &v);
static void SP_CNSValue(SPNativeCheckpointCodec &c, MantleState &v);
static void SP_CNSValue(SPNativeCheckpointCodec &c, ActionSlotParam_SpecifyWeapon &v);
static void SP_CNSValue(SPNativeCheckpointCodec &c, ActionSlotParam &v);
static void SP_CNSValue(SPNativeCheckpointCodec &c, score_s &v);
static void SP_CNSValue(SPNativeCheckpointCodec &c, ArchivedMatchState &v);
static void SP_CNSValue(SPNativeCheckpointCodec &c, UnarchivedMatchState &v);
static void SP_CNSValue(SPNativeCheckpointCodec &c, renderOptions_s &v);
static void SP_CNSValue(SPNativeCheckpointCodec &c, PlayerHeldWeapon &v);
static void SP_CNSValue(SPNativeCheckpointCodec &c, hudelem_color_t &v);
static void SP_CNSValue(SPNativeCheckpointCodec &c, hudelem_s &v);
static void SP_CNSValue(SPNativeCheckpointCodec &c, clientState_s &v);
static void SP_CNSValue(SPNativeCheckpointCodec &c, trajectory_t &v);
static void SP_CNSValue(SPNativeCheckpointCodec &c, LerpEntityState &v);
template<class T, size_t N> static void SP_CNSValue(SPNativeCheckpointCodec &c, T (&value)[N])
{ for (auto &v : value) SP_CNSValue(c, v); }
static void SP_CNSValue(SPNativeCheckpointCodec &c, AmmoPool &v)
{
    SP_CNSValue(c, v.ammoIndex);
    SP_CNSValue(c, v.count);
}
static void SP_CNSValue(SPNativeCheckpointCodec &c, AmmoClip &v)
{
    SP_CNSValue(c, v.clipIndex);
    SP_CNSValue(c, v.count);
}
static void SP_CNSValue(SPNativeCheckpointCodec &c, SprintState &v)
{
    SP_CNSValue(c, v.sprintButtonUpRequired);
    SP_CNSValue(c, v.sprintDelay);
    SP_CNSValue(c, v.lastSprintStart);
    SP_CNSValue(c, v.lastSprintEnd);
    SP_CNSValue(c, v.sprintStartMaxLength);
    SP_CNSValue(c, v.sprintDuration);
    SP_CNSValue(c, v.sprintCooldown);
}
static void SP_CNSValue(SPNativeCheckpointCodec &c, MantleState &v)
{
    SP_CNSValue(c, v.yaw);
    SP_CNSValue(c, v.timer);
    SP_CNSValue(c, v.transIndex);
    SP_CNSValue(c, v.flags);
}
static void SP_CNSValue(SPNativeCheckpointCodec &c, ActionSlotParam_SpecifyWeapon &v)
{
    SP_CNSValue(c, v.index);
}
static void SP_CNSValue(SPNativeCheckpointCodec &c, ActionSlotParam &v)
{
    SP_CNSValue(c, v.specifyWeapon);
}
static void SP_CNSValue(SPNativeCheckpointCodec &c, score_s &v)
{
    SP_CNSValue(c, v.ping);
    SP_CNSValue(c, v.status_icon);
    SP_CNSValue(c, v.place);
    SP_CNSValue(c, v.score);
    SP_CNSValue(c, v.kills);
    SP_CNSValue(c, v.assists);
    SP_CNSValue(c, v.deaths);
    SP_CNSValue(c, v.scoreboardColumns);
}
static void SP_CNSValue(SPNativeCheckpointCodec &c, ArchivedMatchState &v)
{
    SP_CNSValue(c, v.matchUIVisibilityFlags);
    SP_CNSValue(c, v.bombTimer);
}
static void SP_CNSValue(SPNativeCheckpointCodec &c, UnarchivedMatchState &v)
{
    SP_CNSValue(c, v.alliesScore);
    SP_CNSValue(c, v.axisScore);
    SP_CNSValue(c, v.scoreLimit);
    SP_CNSValue(c, v.matchUIVisibilityFlags);
    SP_CNSValue(c, v.scoreboardColumnTypes);
    SP_CNSValue(c, v.mapCenter);
    SP_CNSValue(c, v.talkFlags);
}
static void SP_CNSValue(SPNativeCheckpointCodec &c, renderOptions_s &v) { c.Field(v.i); }
static void SP_CNSValue(SPNativeCheckpointCodec &c, hudelem_color_t &v) { c.Field(v.rgba); }
static void SP_CNSValue(SPNativeCheckpointCodec &c, PlayerHeldWeapon &v)
{
    SP_CNSValue(c, v.weapon); SP_CNSValue(c, v.model); SP_CNSValue(c, v.options);
    // The heatPercent/fuelTankTime union is a fixed four-byte wire payload.
    unsigned bits; memcpy(&bits, &v.fuelTankTime, 4); c.Field(bits); memcpy(&v.fuelTankTime, &bits, 4);
    SP_CNSValue(c, v.overHeating); SP_CNSValue(c, v.needsRechamber); SP_CNSValue(c, v.heldBefore);
    SP_CNSValue(c, v.quickReload); SP_CNSValue(c, v.blockWeaponPickup);
}
static void SP_CNSValue(SPNativeCheckpointCodec &c, hudelem_s &v)
{
    SP_CNSValue(c, v.x);
    SP_CNSValue(c, v.y);
    SP_CNSValue(c, v.z);
    SP_CNSValue(c, v.fontScale);
    SP_CNSValue(c, v.color);
    SP_CNSValue(c, v.fromColor);
    SP_CNSValue(c, v.fadeStartTime);
    SP_CNSValue(c, v.scaleStartTime);
    SP_CNSValue(c, v.fromX);
    SP_CNSValue(c, v.fromY);
    SP_CNSValue(c, v.moveStartTime);
    SP_CNSValue(c, v.time);
    SP_CNSValue(c, v.duration);
    SP_CNSValue(c, v.value);
    SP_CNSValue(c, v.sort);
    SP_CNSValue(c, v.glowColor);
    SP_CNSValue(c, v.fxBirthTime);
    SP_CNSValue(c, v.targetEntNum);
    SP_CNSValue(c, v.fadeTime);
    SP_CNSValue(c, v.label);
    SP_CNSValue(c, v.width);
    SP_CNSValue(c, v.height);
    SP_CNSValue(c, v.fromWidth);
    SP_CNSValue(c, v.fromHeight);
    SP_CNSValue(c, v.scaleTime);
    SP_CNSValue(c, v.moveTime);
    SP_CNSValue(c, v.text);
    SP_CNSValue(c, v.fxLetterTime);
    SP_CNSValue(c, v.fxDecayStartTime);
    SP_CNSValue(c, v.fxDecayDuration);
    SP_CNSValue(c, v.fxRedactDecayStartTime);
    SP_CNSValue(c, v.fxRedactDecayDuration);
    SP_CNSValue(c, v.flags);
    SP_CNSValue(c, v.type);
    SP_CNSValue(c, v.font);
    SP_CNSValue(c, v.alignOrg);
    SP_CNSValue(c, v.alignScreen);
    SP_CNSValue(c, v.materialIndex);
    SP_CNSValue(c, v.offscreenMaterialIdx);
    SP_CNSValue(c, v.fromAlignOrg);
    SP_CNSValue(c, v.fromAlignScreen);
    SP_CNSValue(c, v.soundID);
    SP_CNSValue(c, v.ui3dWindow);
}
static void SP_CNSValue(SPNativeCheckpointCodec &c, trajectory_t &v)
{
    SP_CNSValue(c, v.trType);
    SP_CNSValue(c, v.trTime);
    SP_CNSValue(c, v.trDuration);
    SP_CNSValue(c, v.trBase);
    SP_CNSValue(c, v.trDelta);
}
static void SP_CNSValue(SPNativeCheckpointCodec &c, clientState_s &v)
{
    SP_CNSValue(c, v.clientIndex);
    SP_CNSValue(c, v.team);
    SP_CNSValue(c, v.ffaTeam);
    SP_CNSValue(c, v.modelindex);
    SP_CNSValue(c, v.attachModelIndex);
    SP_CNSValue(c, v.attachTagIndex);
    SP_CNSValue(c, v.name);
    SP_CNSValue(c, v.maxSprintTimeMultiplier);
    SP_CNSValue(c, v.rank);
    SP_CNSValue(c, v.prestige);
    SP_CNSValue(c, v.lastDamageTime);
    SP_CNSValue(c, v.lastStandStartTime);
    SP_CNSValue(c, v.xuid);
    SP_CNSValue(c, v.perks);
    SP_CNSValue(c, v.clanAbbrev);
    SP_CNSValue(c, v.attachedVehEntNum);
    SP_CNSValue(c, v.attachedVehSeat);
    SP_CNSValue(c, v.needsRevive);
    SP_CNSValue(c, v.vehAnimState);
    SP_CNSValue(c, v.score);
    SP_CNSValue(c, v.clientUIVisibilityFlags);
}
static void SP_CNSValue(SPNativeCheckpointCodec &c, LerpEntityState &v)
{
    SP_CNSValue(c, v.eFlags); SP_CNSValue(c, v.eFlags2);
    SP_CNSValue(c, v.pos); SP_CNSValue(c, v.apos);
    // All members of this network union are pointer-free, fixed-width payloads.
    static_assert(sizeof(LerpEntityStateTypeUnion) == 32);
    unsigned bits[8]; memcpy(bits, &v.u, 32); SP_CNSValue(c, bits); memcpy(&v.u, bits, 32);
    SP_CNSValue(c, v.useCount);
}
static void SP_CNSEntityCodec(SPNativeCheckpointCodec &c, entityState_s &v, std::string &target)
{
    SP_CNSValue(c, v.number);
    SP_CNSValue(c, v.lerp);
    SP_CNSValue(c, v.time2);
    SP_CNSValue(c, v.loopSoundId);
    SP_CNSValue(c, v.solid);
    SP_CNSValue(c, v.renderOptions);
    static_assert(sizeof(v.animState) == 16 && sizeof(v.un3) == 4 && sizeof(v.index) == 2);
    c.Field(v.animState); c.Field(v.un3);
    SP_CNSValue(c, v.partBits);
    SP_CNSValue(c, v.events);
    SP_CNSValue(c, v.eventParms);
    SP_CNSValue(c, v.eventParm);
    SP_CNSValue(c, v.eType);
    SP_CNSValue(c, v.groundEntityNum);
    c.Field(v.index);
    SP_CNSValue(c, v.otherEntityNum);
    SP_CNSValue(c, v.attackerEntityNum);
    SP_CNSValue(c, v.enemyModel);
    SP_CNSValue(c, v.weapon);
    SP_CNSValue(c, v.lastStandPrevWeapon);
    SP_ClientOwnerName(c, target, true);
    SP_CNSValue(c, v.loopSoundFade);
    SP_CNSValue(c, v.eventSequence);
    SP_CNSValue(c, v.surfType);
    SP_CNSValue(c, v.clientNum);
    SP_CNSValue(c, v.iHeadIcon);
    c.Field(v.faction); SP_CNSValue(c, v.weaponModel); c.Field(v.un1);
    SP_CNSValue(c, v.clientLinkInfo.parentEnt);
    SP_CNSValue(c, v.clientLinkInfo.tagIndex);
    SP_CNSValue(c, v.clientLinkInfo.flags);
    SP_CNSValue(c, v.animtreeIndex);
    if (!SP_ClientPoseSlot(0, v.number) || v.eType < 0) Com_Error(ERR_DROP, "Client network checkpoint rejected entity identity");
}
struct SPClientPlayerImage
{
    playerState_s value{};
    bool textPresent[32]{};
    std::string text[32];
};
static void SP_CNSReadPlayer(const playerState_s &p, SPClientPlayerImage &out)
{
    out.value = p;
    for (int i = 0; i < 32; ++i)
    {
        out.textPresent[i] = p.objective[i].alt_3D_text != nullptr;
        out.text[i] = out.textPresent[i] ? p.objective[i].alt_3D_text : "";
        if (out.text[i].size() > 255) Com_Error(ERR_DROP, "Client objective text exceeds checkpoint bounds");
    }
}
static void SP_CNSBindPlayer(SPClientPlayerImage &out)
{
    for (int i = 0; i < 32; ++i) out.value.objective[i].alt_3D_text = out.textPresent[i] ? out.text[i].c_str() : nullptr;
}
static void SP_CNSPlayerCodec(SPNativeCheckpointCodec &c, SPClientPlayerImage &image)
{
    auto &v = image.value;
    SP_CNSValue(c, v.commandTime);
    SP_CNSValue(c, v.pm_type);
    SP_CNSValue(c, v.bobCycle);
    SP_CNSValue(c, v.pm_flags);
    SP_CNSValue(c, v.weapFlags);
    SP_CNSValue(c, v.otherFlags);
    SP_CNSValue(c, v.pm_time);
    SP_CNSValue(c, v.loopSoundId);
    SP_CNSValue(c, v.loopSoundFade);
    SP_CNSValue(c, v.origin);
    SP_CNSValue(c, v.velocity);
    SP_CNSValue(c, v.weaponTime);
    SP_CNSValue(c, v.weaponDelay);
    SP_CNSValue(c, v.weaponTimeLeft);
    SP_CNSValue(c, v.weaponDelayLeft);
    SP_CNSValue(c, v.weaponIdleTime);
    SP_CNSValue(c, v.grenadeTimeLeft);
    SP_CNSValue(c, v.throwBackGrenadeOwner);
    SP_CNSValue(c, v.throwBackGrenadeTimeLeft);
    SP_CNSValue(c, v.weaponRestrictKickTime);
    SP_CNSValue(c, v.mountAvailable);
    SP_CNSValue(c, v.bRunLeftGun);
    SP_CNSValue(c, v.bCarryingTurret);
    SP_CNSValue(c, v.mountPos);
    SP_CNSValue(c, v.mountDir);
    SP_CNSValue(c, v.foliageSoundTime);
    SP_CNSValue(c, v.gravity);
    SP_CNSValue(c, v.leanf);
    SP_CNSValue(c, v.speed);
    SP_CNSValue(c, v.delta_angles);
    SP_CNSValue(c, v.groundEntityNum);
    SP_CNSValue(c, v.groundType);
    SP_CNSValue(c, v.vLadderVec);
    SP_CNSValue(c, v.jumpTime);
    SP_CNSValue(c, v.jumpOriginZ);
    SP_CNSValue(c, v.slideTime);
    SP_CNSValue(c, v.moveType);
    SP_CNSValue(c, v.legsTimer);
    SP_CNSValue(c, v.torsoTimer);
    SP_CNSValue(c, v.legsAnim);
    SP_CNSValue(c, v.torsoAnim);
    SP_CNSValue(c, v.legsAnimDuration);
    SP_CNSValue(c, v.torsoAnimDuration);
    SP_CNSValue(c, v.damageTimer);
    SP_CNSValue(c, v.damageDuration);
    SP_CNSValue(c, v.dmgDirection);
    SP_CNSValue(c, v.dmgType);
    SP_CNSValue(c, v.corpseIndex);
    SP_CNSValue(c, v.movementDir);
    SP_CNSValue(c, v.eFlags);
    SP_CNSValue(c, v.eFlags2);
    SP_CNSValue(c, v.predictableEventSequence);
    SP_CNSValue(c, v.predictableEventSequenceOld);
    SP_CNSValue(c, v.predictableEvents);
    SP_CNSValue(c, v.predictableEventParms);
    SP_CNSValue(c, v.unpredictableEventSequence);
    SP_CNSValue(c, v.unpredictableEventSequenceOld);
    SP_CNSValue(c, v.unpredictableEvents);
    SP_CNSValue(c, v.unpredictableEventParms);
    SP_CNSValue(c, v.clientNum);
    SP_CNSValue(c, v.offHandIndex);
    SP_CNSValue(c, v.offhandSecondary);
    SP_CNSValue(c, v.offhandPrimary);
    SP_CNSValue(c, v.renderOptions);
    SP_CNSValue(c, v.weapon);
    SP_CNSValue(c, v.lastStandPrevWeapon);
    SP_CNSValue(c, v.lastWeaponAltModeSwitch);
    SP_CNSValue(c, v.stowedWeapon);
    SP_CNSValue(c, v.stowedWeaponCamo);
    SP_CNSValue(c, v.meleeWeapon);
    SP_CNSValue(c, v.weaponstate);
    SP_CNSValue(c, v.weaponstateLeft);
    SP_CNSValue(c, v.weaponShotCount);
    SP_CNSValue(c, v.weaponShotCountLeft);
    SP_CNSValue(c, v.fWeaponPosFrac);
    SP_CNSValue(c, v.adsDelayTime);
    SP_CNSValue(c, v.spreadOverride);
    SP_CNSValue(c, v.spreadOverrideState);
    SP_CNSValue(c, v.weaponSpinLerp);
    SP_CNSValue(c, v.viewmodelIndex);
    SP_CNSValue(c, v.viewangles);
    SP_CNSValue(c, v.viewHeightTarget);
    SP_CNSValue(c, v.viewHeightCurrent);
    SP_CNSValue(c, v.viewHeightLerpTime);
    SP_CNSValue(c, v.viewHeightLerpTarget);
    SP_CNSValue(c, v.viewHeightLerpDown);
    SP_CNSValue(c, v.viewAngleClampBase);
    SP_CNSValue(c, v.viewAngleClampRange);
    SP_CNSValue(c, v.stackFireCount);
    SP_CNSValue(c, v.damageEvent);
    SP_CNSValue(c, v.damageYaw);
    SP_CNSValue(c, v.damagePitch);
    SP_CNSValue(c, v.damageCount);
    SP_CNSValue(c, v.stats);
    SP_CNSValue(c, v.heldWeapons);
    SP_CNSValue(c, v.ammoNotInClip);
    SP_CNSValue(c, v.ammoInClip);
    SP_CNSValue(c, v.proneDirection);
    SP_CNSValue(c, v.proneDirectionPitch);
    SP_CNSValue(c, v.proneTorsoPitch);
    SP_CNSValue(c, v.viewlocked);
    SP_CNSValue(c, v.viewlocked_entNum);
    SP_CNSValue(c, v.vehiclePos);
    SP_CNSValue(c, v.vehicleType);
    SP_CNSValue(c, v.vehicleAnimBoneIndex);
    SP_CNSValue(c, v.linkFlags);
    SP_CNSValue(c, v.linkAngles);
    SP_CNSValue(c, v.cursorHint);
    SP_CNSValue(c, v.cursorHintString);
    SP_CNSValue(c, v.cursorHintEntIndex);
    SP_CNSValue(c, v.iCompassPlayerInfo);
    SP_CNSValue(c, v.spyplaneTypeEnabled);
    SP_CNSValue(c, v.satelliteTypeEnabled);
    SP_CNSValue(c, v.locationSelectionType);
    SP_CNSValue(c, v.locationSelectionInfo);
    SP_CNSValue(c, v.sprintState);
    SP_CNSValue(c, v.lastDtpEnd);
    SP_CNSValue(c, v.fTorsoPitch);
    SP_CNSValue(c, v.fWaistPitch);
    SP_CNSValue(c, v.holdBreathScale);
    SP_CNSValue(c, v.holdBreathTimer);
    SP_CNSValue(c, v.moveSpeedScaleMultiplier);
    SP_CNSValue(c, v.mantleState);
    SP_CNSValue(c, v.vehicleAnimStage);
    SP_CNSValue(c, v.vehicleEntryPoint);
    SP_CNSValue(c, v.scriptedAnim);
    SP_CNSValue(c, v.scriptedAnimTime);
    SP_CNSValue(c, v.meleeChargeYaw);
    SP_CNSValue(c, v.meleeChargeDist);
    SP_CNSValue(c, v.meleeChargeTime);
    SP_CNSValue(c, v.weapLockFlags);
    SP_CNSValue(c, v.weapLockedEntnum);
    SP_CNSValue(c, v.perks);
    SP_CNSValue(c, v.actionSlotType);
    SP_CNSValue(c, v.actionSlotParam);
    SP_CNSValue(c, v.entityEventSequence);
    SP_CNSValue(c, v.weapAnim);
    SP_CNSValue(c, v.weapAnimLeft);
    SP_CNSValue(c, v.aimSpreadScale);
    SP_CNSValue(c, v.shellshockIndex);
    SP_CNSValue(c, v.shellshockTime);
    SP_CNSValue(c, v.shellshockDuration);
    SP_CNSValue(c, v.dofNearStart);
    SP_CNSValue(c, v.dofNearEnd);
    SP_CNSValue(c, v.dofFarStart);
    SP_CNSValue(c, v.dofFarEnd);
    SP_CNSValue(c, v.dofNearBlur);
    SP_CNSValue(c, v.dofFarBlur);
    SP_CNSValue(c, v.dofViewmodelStart);
    SP_CNSValue(c, v.dofViewmodelEnd);
    SP_CNSValue(c, v.waterlevel);
    SP_CNSValue(c, v.smokeColorIndex);
    SP_CNSValue(c, v.hudElemLastAssignedSoundID);
    SP_CNSValue(c, v.adsZoomSelect);
    SP_CNSValue(c, v.adsZoomLatchTime);
    SP_CNSValue(c, v.adsZoomLatchState);
    SP_CNSValue(c, v.artilleryInboundIconLocation);
    SP_CNSValue(c, v.visionSetLerpRatio);
    SP_CNSValue(c, v.poisoned);
    SP_CNSValue(c, v.binoculars);
    SP_CNSValue(c, v.scriptCursorHintString);
    for (int i = 0; i < 32; ++i)
    {
        auto &o = v.objective[i];
        SP_CNSValue(c, o.state);
        SP_CNSValue(c, o.origin);
        SP_CNSValue(c, o.size);
        SP_CNSValue(c, o.entNum);
        SP_CNSValue(c, o.teamNum);
        SP_CNSValue(c, o.icon);
        SP_CNSValue(c, o.use3D);
        SP_CNSValue(c, o.color);
        c.Field(image.textPresent[i]);
        // Preserve null versus a present empty string without storing an address.
        if (image.textPresent[i])
        {
            int length = c.Int(static_cast<int>(image.text[i].size()));
            if (length < 0 || length > 255) Com_Error(ERR_DROP, "Client objective checkpoint rejected text length");
            if (c.reader) image.text[i].resize(length);
            for (auto &ch : image.text[i]) { c.Field(ch); if (!ch) Com_Error(ERR_DROP, "Client objective checkpoint rejected embedded NUL"); }
        }
        else if (c.reader) image.text[i].clear();
    }
    SP_CNSValue(c, v.deltaTime);
    SP_CNSValue(c, v.killCamEntity);
    SP_CNSValue(c, v.killCamTargetEntity);
    SP_CNSValue(c, v.hud.current); SP_CNSValue(c, v.hud.archival);
    if (v.clientNum != 0) Com_Error(ERR_DROP, "Client prediction checkpoint requires local solo player");
    SP_CNSBindPlayer(image);
}
