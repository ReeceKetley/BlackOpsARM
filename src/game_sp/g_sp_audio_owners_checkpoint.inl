// SDP3: slot identities matter to stationary sound handles. Native entity
// pointers are represented only by indices into the fixed 296-entry pool.
struct SPAudioOwnersCheckpoint
{
    unsigned frame;
    snd_line_emitter lines[128];
    snd_loop_emitter loops[256];
    snd_ent_state entities[296];
    int next[296], heads[MAX_CL_ENTNUM];
    float voicePositions[74][3];
};

static int SP_AudioEntitySlot(const snd_ent_state *p)
{
    if (!p) return -1;
    for (int i = 0; i < 296; ++i) if (p == &g_snd.entState[i]) return i;
    Com_Error(ERR_DROP, "Audio entity checkpoint pointer outside its native pool");
    return -1;
}

static bool SP_ValidAudioEntityGraph(const SPAudioOwnersCheckpoint &s)
{
    bool seen[296]{};
    if (!s.frame) return false;
    for (int bucket = 0; bucket < MAX_CL_ENTNUM; ++bucket)
    {
        int length = 0;
        for (int index = s.heads[bucket]; index != -1; index = s.next[index])
        {
            if (index < 0 || index >= 296 || seen[index] || ++length > 50) return false;
            const auto &e = s.entities[index];
            const unsigned handle = e.handle.handle;
            if (!e.lastUsed || e.lastUsed > s.frame || (handle & 0xFFF) != static_cast<unsigned>(bucket)
                || (handle & (1u << 21)) || (handle & 0xFFF) >= 0xFFE) return false;
            for (int j = 0; j < 296; ++j)
                if (seen[j] && s.entities[j].handle.handle == handle) return false;
            seen[index] = true;
        }
    }
    for (int i = 0; i < 296; ++i)
        if (seen[i] != (s.entities[i].lastUsed != 0)) return false;
    return true;
}

static void SP_ReadLiveAudioOwners(SPAudioOwnersCheckpoint &s)
{
    s.frame = g_snd.frame;
    memcpy(s.lines, g_snd.lineEmitters, sizeof(s.lines));
    memcpy(s.loops, g_snd.loopEmitters, sizeof(s.loops));
    memcpy(s.entities, g_snd.entState, sizeof(s.entities));
    for (int i = 0; i < 296; ++i)
    {
        s.next[i] = s.entities[i].lastUsed ? SP_AudioEntitySlot(s.entities[i].next) : -1;
        s.entities[i].next = nullptr;
    }
    for (int i = 0; i < MAX_CL_ENTNUM; ++i) s.heads[i] = SP_AudioEntitySlot(g_snd.entStateIndex[i]);
    memcpy(s.voicePositions, g_snd.voicePositionCache, sizeof(s.voicePositions));
}

static const snd_alias_list_t *SP_AudioEmitterIdentity(SPNativeCheckpointCodec &c, unsigned &id, const snd_alias_list_t *cached)
{
    c.Field(id);
    if (!id) return nullptr;
    bool resolved = cached != nullptr; c.Field(resolved);
    const auto *asset = SND_FindAliasFromId(id);
    if (!asset || !asset->head || asset->count <= 0 || asset->id != id
        || (!c.reader && cached && cached != asset))
        Com_Error(ERR_DROP, "Audio emitter checkpoint lost exact alias identity");
    if (strcmp(c.Name(asset->name), asset->name) || SND_FindAlias(asset->name) != asset)
        Com_Error(ERR_DROP, "Audio emitter checkpoint alias name changed");
    return resolved ? asset : nullptr;
}

static void SP_ClientAudioOwnersCodec(SPNativeCheckpointCodec &c, SPAudioOwnersCheckpoint &s)
{
    c.Field(s.frame);
    int lines = 0, loops = 0, entities = 0;
    for (auto &e : s.lines)
    {
        e.alias = SP_AudioEmitterIdentity(c, e.id, e.alias);
        if (e.id) { for (auto &v : e.origin) SP_AudioMixerFloat(c, v); ++lines; }
    }
    for (auto &e : s.loops)
    {
        e.alias = SP_AudioEmitterIdentity(c, e.id, e.alias);
        if (e.id) { SP_AudioMixerFloat(c, e.origin); ++loops; }
    }
    for (int i = 0; i < 296; ++i)
    {
        auto &e = s.entities[i]; c.Field(e.lastUsed);
        if (!e.lastUsed) { s.next[i] = -1; continue; }
        c.Field(e.handle.handle); SP_AudioMixerFloat(c, e.origin); SP_AudioMixerFloat(c, e.velocity);
        for (auto &axis : e.orientation) SP_AudioMixerFloat(c, axis);
        c.Field(s.next[i]); ++entities;
    }
    c.Field(s.heads);
    for (auto &p : s.voicePositions) SP_AudioMixerFloat(c, p);
    if (!SP_ValidAudioEntityGraph(s)) Com_Error(ERR_DROP, "Audio entity checkpoint invalid pool/index graph");
    Com_Printf(15, "Client audio owners: %s %i line / %i loop emitters, %i entity cache owners, sound frame %u\n",
        c.reader ? "decoded" : "captured", lines, loops, entities, s.frame);
}

static void SP_PublishAudioOwners(const SPAudioOwnersCheckpoint &s)
{
    if (!SP_ValidAudioEntityGraph(s)) Com_Error(ERR_DROP, "Audio entity publication rejected invalid graph");
    SND_ResetEntState();
    g_snd.frame = s.frame;
    memcpy(g_snd.lineEmitters, s.lines, sizeof(s.lines));
    memcpy(g_snd.loopEmitters, s.loops, sizeof(s.loops));
    for (int i = 0; i < 296; ++i)
        if (s.entities[i].lastUsed)
        {
            g_snd.entState[i] = s.entities[i];
            g_snd.entState[i].next = s.next[i] >= 0 ? &g_snd.entState[s.next[i]] : nullptr;
        }
    for (int i = 0; i < MAX_CL_ENTNUM; ++i)
        g_snd.entStateIndex[i] = s.heads[i] >= 0 ? &g_snd.entState[s.heads[i]] : nullptr;
    memcpy(g_snd.voicePositionCache, s.voicePositions, sizeof(s.voicePositions));
    int queried = 0;
    for (int i = 0; i < 296; ++i)
        if (s.entities[i].lastUsed)
        {
            if (SND_FindEntState(s.entities[i].handle, false) != &g_snd.entState[i])
                Com_Error(ERR_DROP, "Audio entity canonical lookup failed after rebinding");
            ++queried;
        }
    Com_Printf(15, "Client audio owners: REBOUND emitter slots and %i entity owners, actual canonical lookups verified\n", queried);
}
