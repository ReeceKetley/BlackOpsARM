// SDP2 stores logical mixer state only. DSP delay lines and device preferences
// belong to the new backend; no device, asset or listener pointers go on disk.
#include <sound/snd_radverb.h>

struct SPAudioMixerCheckpoint
{
    snd_game_state gameState;
    float timescale, scriptTimescale;
    int paused, forcePause;
    snd_fader_t volume, minimumPriority;
    float hdrLoad, maximumPriority;
    snd_enveffect envEffects[3];
    int effect;
    snd_listener listener;
    unsigned currentContexts[16];
    snd_snapshot_category snapshots[11];
    float attenuation[64], occlusion[64];
    float proximityOrigin[3], proximityFront[3], proximityDistance[4];
    float proximityReflectance[4], proximityLateModifier[4];
};

static void SP_ReadLiveAudioMixer(SPAudioMixerCheckpoint &m)
{
    m.gameState = g_snd.gameState; m.timescale = g_snd.timescale; m.scriptTimescale = g_snd.scriptTimescale;
    m.paused = g_snd.paused; m.forcePause = g_snd.forcePause;
    m.volume = g_snd.volume; m.minimumPriority = g_snd.minimumPriority;
    m.hdrLoad = g_snd.hdrLoad; m.maximumPriority = g_snd.maximumPriority;
    memcpy(m.envEffects, g_snd.envEffects, sizeof(m.envEffects));
    m.effect = -1;
    for (int i = 0; i < 3; ++i) if (g_snd.effect == &g_snd.envEffects[i]) m.effect = i;
    if (g_snd.effect && m.effect == -1) Com_Error(ERR_DROP, "Audio mixer has an unknown environment owner");
    m.listener = g_snd.listeners[0];
    memcpy(m.currentContexts, g_snd.currentContexts, sizeof(m.currentContexts));
    memcpy(m.snapshots, g_snd.snapshotCategories, sizeof(m.snapshots));
    memcpy(m.attenuation, g_snd.snapshotAttenuation, sizeof(m.attenuation));
    memcpy(m.occlusion, g_snd.snapshotOcclusion, sizeof(m.occlusion));
#define MIX_COPY(member) memcpy(m.member, g_snd.member, sizeof(m.member))
    MIX_COPY(proximityOrigin); MIX_COPY(proximityFront); MIX_COPY(proximityDistance);
    MIX_COPY(proximityReflectance); MIX_COPY(proximityLateModifier);
#undef MIX_COPY
}

static void SP_AudioMixerFloat(SPNativeCheckpointCodec &c, float &value)
{
    c.Field(value);
    if (!std::isfinite(value)) Com_Error(ERR_DROP, "Audio mixer checkpoint rejected nonfinite value");
}
template<size_t N> static void SP_AudioMixerFloat(SPNativeCheckpointCodec &c, float (&values)[N])
{
    for (auto &v : values) SP_AudioMixerFloat(c, v);
}
static void SP_AudioMixerFader(SPNativeCheckpointCodec &c, snd_fader_t &f)
{
    SP_AudioMixerFloat(c, f.value); SP_AudioMixerFloat(c, f.goal); SP_AudioMixerFloat(c, f.rate);
}

static void SP_ClientAudioMixerCodec(SPNativeCheckpointCodec &c, SPAudioMixerCheckpoint &m)
{
    const auto *globals = g_snd.global_constants;
    if (!globals || globals->contextCount > 16 || globals->snapshotGroupCount > 64)
        Com_Error(ERR_DROP, "Audio mixer checkpoint lacks bounded global definitions");
    // Voices, snapshot channels and context slots use asset-defined indices.
    // Validate the actual ordered identities instead of silently remapping them.
    if (strcmp(c.Name(globals->name), globals->name)
        || c.Int(globals->groupCount) != globals->groupCount
        || c.Int(globals->snapshotGroupCount) != globals->snapshotGroupCount
        || c.Int(globals->contextCount) != globals->contextCount)
        Com_Error(ERR_DROP, "Audio mixer checkpoint global layout changed");
    for (unsigned i = 0; i < globals->groupCount; ++i)
        if (strcmp(c.Name(globals->groups[i].name), globals->groups[i].name)
            || static_cast<unsigned>(c.Int(globals->groups[i].id)) != globals->groups[i].id)
            Com_Error(ERR_DROP, "Audio mixer checkpoint group identity changed");
    for (unsigned i = 0; i < globals->snapshotGroupCount; ++i)
        if (strcmp(c.Name(globals->snapshotGroups[i].name), globals->snapshotGroups[i].name))
            Com_Error(ERR_DROP, "Audio mixer checkpoint snapshot layout changed");
    for (unsigned i = 0; i < globals->contextCount; ++i)
    {
        const auto &def = globals->contexts[i];
        if (def.valueCount > 8 || static_cast<unsigned>(c.Int(def.type)) != def.type
            || c.Int(def.valueCount) != def.valueCount)
            Com_Error(ERR_DROP, "Audio mixer checkpoint context definition changed");
        // Canonical SND_FindContextValueIndex searches all eight slots, even
        // when valueCount describes fewer entries. Preserve that contract.
        for (unsigned j = 0; j < 8; ++j)
            if (static_cast<unsigned>(c.Int(def.values[j])) != def.values[j])
                Com_Error(ERR_DROP, "Audio mixer checkpoint context value changed");
    }
    SP_AudioMixerFloat(c, m.gameState.timescale);
    c.Field(m.gameState.cgTime); c.Field(m.gameState.seed);
    c.Field(m.gameState.gamePaused); c.Field(m.gameState.mature);
    SP_AudioMixerFloat(c, m.timescale); SP_AudioMixerFloat(c, m.scriptTimescale);
    c.Field(m.paused); c.Field(m.forcePause);
    if (static_cast<unsigned>(m.paused) > 1 || static_cast<unsigned>(m.forcePause) > 1)
        Com_Error(ERR_DROP, "Audio mixer checkpoint invalid pause state");
    SP_AudioMixerFader(c, m.volume); SP_AudioMixerFader(c, m.minimumPriority);
    SP_AudioMixerFloat(c, m.hdrLoad); SP_AudioMixerFloat(c, m.maximumPriority);
    c.Field(m.effect);
    if (m.effect < -1 || m.effect >= 3) Com_Error(ERR_DROP, "Audio mixer checkpoint invalid environment slot");
    for (auto &e : m.envEffects)
    {
        c.Field(e.reverbId); c.Field(e.active);
        SP_AudioMixerFloat(c, e.drylevel); SP_AudioMixerFloat(c, e.drygoal); SP_AudioMixerFloat(c, e.dryrate);
        SP_AudioMixerFloat(c, e.wetlevel); SP_AudioMixerFloat(c, e.wetgoal); SP_AudioMixerFloat(c, e.wetrate);
        if (e.reverbId)
        {
            const auto *asset = SND_GetRadverb(e.reverbId);
            if (!asset || asset->id != e.reverbId || strcmp(c.Name(asset->name), asset->name))
                Com_Error(ERR_DROP, "Audio mixer checkpoint missing exact reverb asset");
        }
        else if (e.active) Com_Error(ERR_DROP, "Audio mixer checkpoint active reverb has no identity");
    }
    c.Field(m.listener.active); c.Field(m.listener.clientNum); c.Field(m.listener.team);
    SP_AudioMixerFloat(c, m.listener.orient.origin);
    for (auto &axis : m.listener.orient.axis) SP_AudioMixerFloat(c, axis);
    c.Field(m.currentContexts);
    for (unsigned i = 0; i < 16; ++i)
    {
        bool valid = m.currentContexts[i] == 0;
        if (i < globals->contextCount)
            for (unsigned j = 0; j < 8; ++j)
                valid |= m.currentContexts[i] == globals->contexts[i].values[j];
        if (!valid) Com_Error(ERR_DROP, "Audio mixer checkpoint unknown selected context");
    }
    for (auto &s : m.snapshots)
    {
        c.Field(s.snapshot);
        const auto *asset = SND_GetSnapshotById(s.snapshot);
        if (!asset || asset->id != s.snapshot || strcmp(c.Name(asset->name), asset->name))
            Com_Error(ERR_DROP, "Audio mixer checkpoint missing exact snapshot asset");
        SP_AudioMixerFloat(c, s.length); SP_AudioMixerFloat(c, s.amount);
        for (auto &f : s.attenuation) SP_AudioMixerFader(c, f);
        for (auto &f : s.occlusion) SP_AudioMixerFader(c, f);
    }
    SP_AudioMixerFloat(c, m.attenuation); SP_AudioMixerFloat(c, m.occlusion);
    SP_AudioMixerFloat(c, m.proximityOrigin); SP_AudioMixerFloat(c, m.proximityFront);
    SP_AudioMixerFloat(c, m.proximityDistance); SP_AudioMixerFloat(c, m.proximityReflectance);
    SP_AudioMixerFloat(c, m.proximityLateModifier);
}

static void SP_PublishAudioMixer(const SPAudioMixerCheckpoint &m)
{
    // Caller owns the joined sound worker and stopped mixer. Asset validation
    // completed before old client teardown; publish no raw saved pointers.
    g_snd.gameState = m.gameState; g_snd.timescale = m.timescale; g_snd.scriptTimescale = m.scriptTimescale;
    g_snd.paused = m.paused; g_snd.forcePause = m.forcePause;
    g_snd.volume = m.volume; g_snd.minimumPriority = m.minimumPriority;
    g_snd.hdrLoad = m.hdrLoad; g_snd.maximumPriority = m.maximumPriority;
    memcpy(g_snd.envEffects, m.envEffects, sizeof(m.envEffects));
    g_snd.effect = m.effect >= 0 ? &g_snd.envEffects[m.effect] : nullptr;
    g_snd.listeners[0] = m.listener;
    memcpy(g_snd.currentContexts, m.currentContexts, sizeof(m.currentContexts));
    memcpy(g_snd.snapshotCategories, m.snapshots, sizeof(m.snapshots));
    memcpy(g_snd.snapshotAttenuation, m.attenuation, sizeof(m.attenuation));
    memcpy(g_snd.snapshotOcclusion, m.occlusion, sizeof(m.occlusion));
#define MIX_COPY(member) memcpy(g_snd.member, m.member, sizeof(m.member))
    MIX_COPY(proximityOrigin); MIX_COPY(proximityFront); MIX_COPY(proximityDistance);
    MIX_COPY(proximityReflectance); MIX_COPY(proximityLateModifier);
#undef MIX_COPY
}
