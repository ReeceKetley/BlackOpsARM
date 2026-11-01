// SDP4 preserves completed logical trace results, never old collision brush
// hints or native worker/event ownership. Caller holds the occlusion gate.
struct SPAudioOcclusionCheckpoint
{
    snd_occlusion_trace_t traces[74];
    unsigned sequence, completed, framesSince;
};

static void SP_ReadLiveAudioOcclusion(SPAudioOcclusionCheckpoint &s)
{
    if (!SND_OcclusionCheckpointActive()) Com_Error(ERR_DROP, "Occlusion capture requires completed worker gate");
    memcpy(s.traces, g_snd.occlusionTraces, sizeof(s.traces));
    for (auto &trace : s.traces) trace.cache = 0;
    s.sequence = g_snd.traceSequence; s.completed = g_snd.lastTraceSequence;
    s.framesSince = g_snd.framesSinceLastOcclusionUpdate;
}

static void SP_ClientAudioOcclusionCodec(SPNativeCheckpointCodec &c, SPAudioOcclusionCheckpoint &s)
{
    c.Field(s.sequence); c.Field(s.completed); c.Field(s.framesSince);
    if (s.sequence - s.completed > 1u) Com_Error(ERR_DROP, "Occlusion checkpoint invalid batch sequence");
    int active = 0;
    for (auto &t : s.traces)
    {
        c.Field(t.id); t.cache = 0;
        if (t.id < -1) Com_Error(ERR_DROP, "Occlusion checkpoint invalid playback identity");
        SP_AudioMixerFloat(c, t.listener); SP_AudioMixerFloat(c, t.playback);
        SP_AudioMixerFloat(c, t.occlusion);
        if (t.occlusion < 0 || t.occlusion > 1) Com_Error(ERR_DROP, "Occlusion checkpoint invalid muffling amount");
        active += t.id != -1;
    }
    Com_Printf(15, "Client audio occlusion: %s %i trace results, sequence %u completed %u, collision hints invalidated\n",
        c.reader ? "decoded" : "captured", active, s.sequence, s.completed);
}

static void SP_PublishAudioOcclusion(const SPAudioOcclusionCheckpoint &s)
{
    if (!SND_OcclusionCheckpointActive()) Com_Error(ERR_DROP, "Occlusion publication requires completed worker gate");
    memcpy(g_snd.occlusionTraces, s.traces, sizeof(s.traces));
    for (auto &trace : g_snd.occlusionTraces) trace.cache = 0;
    g_snd.traceSequence = s.sequence; g_snd.lastTraceSequence = s.completed;
    g_snd.framesSinceLastOcclusionUpdate = s.framesSince;
    Com_Printf(15, "Client audio occlusion: PUBLISHED logical trace results; actual native worker gated, no saved thread/event/brush owners\n");
}
