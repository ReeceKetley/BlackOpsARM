// Private fresh-process proof. The caller never runs restored CLIENT frames.
#include <chrono>
#include <thread>
static SPAudioCheckpointStage s_decodedClientAudio;
static bool s_clientAudioPrepared, s_clientAudioRetired;

static bool SP_UnpauseSavedAudio(SPAudioCheckpointStage &stage)
{
    if (stage.time < 0 || stage.pausetime < 0 || stage.pausetime > stage.time)
        return false;
    const long long shift = stage.pausetime ? static_cast<long long>(stage.time) - stage.pausetime : 0;
    const auto fits = [shift](int value) {
        const auto result = static_cast<long long>(value) + shift;
        return result >= (std::numeric_limits<int>::min)() && result <= (std::numeric_limits<int>::max)();
    };
    // Match SND_UnpauseSounds: only held voices shift start/end; sample cursors,
    // playback IDs and voiceStartTime retain their canonical meaning. Validate
    // every change before publishing any, including the zero-time sentinel.
    for (int i = 0; i < 74; ++i)
        if (stage.active[i] && stage.voices[i].paused
            && (!fits(stage.voices[i].startTime) || !fits(stage.voices[i].soundFileInfo.endtime)))
            return false;
    for (int i = 0; i < 74; ++i)
        if (stage.active[i] && stage.voices[i].paused)
        {
            auto &voice = stage.voices[i];
            voice.startTime = static_cast<int>(voice.startTime + shift);
            voice.soundFileInfo.endtime = static_cast<int>(voice.soundFileInfo.endtime + shift);
            voice.paused = false;
        }
    stage.pausetime = 0;
    if (stage.hasMixer)
    {
        stage.mixer.paused = stage.mixer.forcePause = 0;
        stage.mixer.gameState.gamePaused = false;
    }
    return true;
}

static bool SP_RebaseAudioClock(SPAudioCheckpointStage &stage, int now)
{
    if (stage.time < 0 || now < 0 || stage.pausetime) return false;
    const auto delta = static_cast<long long>(now) - stage.time;
    const auto fits = [&](int value) {
        const auto shifted = static_cast<long long>(value) + delta;
        return shifted >= std::numeric_limits<int>::min() && shifted <= std::numeric_limits<int>::max();
    };
    for (int i = 0; i < 74; ++i)
        if (stage.active[i])
        {
            const auto &v = stage.voices[i];
            const auto intervalFits = [](long long value) { return value >= std::numeric_limits<int>::min() && value <= std::numeric_limits<int>::max(); };
            if (!fits(v.startTime) || !fits(v.soundFileInfo.endtime) || !fits(v.voiceStartTime)
                || !intervalFits(static_cast<long long>(stage.time) - v.startTime)
                || !intervalFits(static_cast<long long>(v.soundFileInfo.endtime) - stage.time)
                || !intervalFits(static_cast<long long>(stage.time) - v.voiceStartTime)) return false;
        }
    for (int i = 0; i < 74; ++i)
        if (stage.active[i])
        {
            auto &v = stage.voices[i];
            v.startTime = static_cast<int>(v.startTime + delta);
            v.soundFileInfo.endtime = static_cast<int>(v.soundFileInfo.endtime + delta);
            v.voiceStartTime = static_cast<int>(v.voiceStartTime + delta);
        }
    stage.time = now;
    return true;
}

static bool SP_ValidateSavedAudioRegistry(const SPAudioCheckpointStage &stage, bool continuous)
{
    if (stage.pausetime || stage.time < 0 || stage.counter >= 0x7FFFFFFEu || stage.used > 148)
        return false;
    // Spare capacity and at least one prior ID are requirements of the private
    // allocator exercise, not of a valid silent or fully occupied live registry.
    if (!continuous && (!stage.counter || stage.used == 148)) return false;
    unsigned used = 0;
    for (int i = 0; i < 148; ++i)
    {
        const int id = stage.playbacks[i].id;
        if (id == -1) continue;
        if (id <= 0 || static_cast<unsigned>(id) > stage.counter) return false;
        for (int j = 0; j < i; ++j) if (stage.playbacks[j].id == id) return false;
        ++used;
    }
    return used == stage.used;
}

static void SP_PreflightFreshAudio()
{
    s_clientAudioPrepared = s_clientAudioRetired = false;
    auto &stage = s_decodedClientAudio;
    // Resume is an explicit return to gameplay. Normalize the saved pause via
    // the canonical unpause time shift before rebasing to this process's clock.
    // The original durable audio graph has already passed exact decoding.
    if (s_continuousZombieResume && !SP_UnpauseSavedAudio(stage))
        Com_Error(ERR_DROP, "Zombies resume audio pause interval cannot be restored");
    if (!SP_ValidateSavedAudioRegistry(stage, s_continuousZombieResume))
        Com_Error(ERR_DROP, "Fresh audio checkpoint rejected its clock or playback registry");
    if (stage.hasMixer && (stage.mixer.paused || stage.mixer.forcePause || stage.mixer.gameState.gamePaused))
        Com_Error(ERR_DROP, "Fresh audio mixer probe requires an unpaused saved game state");
    if (stage.schema == 0x34504453 && stage.occlusion.sequence != stage.occlusion.completed)
        Com_Error(ERR_DROP, "Fresh occlusion probe requires a completed saved trace batch");
    int voices = 0;
    for (int i = 0; i < 74; ++i)
        if (stage.active[i])
        {
            if (!SD_PreflightCheckpointVoice(i, stage.voices[i], stage.cursors[i]))
                Com_Error(ERR_DROP, "Fresh audio asset/voice preflight refused slot %i alias %s cursor %lld", i,
                    stage.voices[i].alias->name, static_cast<long long>(stage.cursors[i]));
            ++voices;
        }
    static SPAudioCheckpointStage clockCheck;
    memcpy(&clockCheck, &stage, sizeof(stage));
    if ((!s_continuousZombieResume && !voices) || !SP_RebaseAudioClock(clockCheck, Sys_Milliseconds()))
        Com_Error(ERR_DROP, "Fresh audio probe has no supported voices or clock cannot rebase");
    s_clientAudioPrepared = true;
    Com_Printf(15, "Client fresh audio: PREFLIGHT %i voices / %u playback IDs, independent assets and rebased clock validated BEFORE old teardown\n", voices, stage.used);
}

static bool SP_DrainAudioCheckpointQueues()
{
    for (int pass = 0; pass < 8; ++pass)
    {
        SND_NotifyPump();
        if (!SND_CommandPeek() && !SND_NotifyPeek()) return true;
        if (!SND_GetProcessCommandFlag()) return false;
        SND_CommandPump();
    }
    return false;
}

static void SP_BeginFreshAudioTeardown()
{
    if (!s_clientAudioPrepared || !SP_QuiesceClientAudioCheckpoint() || !SP_FreezeAudioCheckpoint())
        Com_Error(ERR_DROP, "Fresh audio could not join/freeze old consumers before CG teardown");
}

static void SP_RetireFreshAudioConsumers()
{
    // CG_Shutdown can enqueue GameReset. Deliver it while the OLD VM is still
    // alive, without scheduling another client-dependent sound frame.
    if (!SP_DrainAudioCheckpointQueues()) Com_Error(ERR_DROP, "Fresh audio old teardown commands remain busy");
    SND_ShutdownVoices();
    if (!SP_DrainAudioCheckpointQueues()) Com_Error(ERR_DROP, "Fresh audio old playback retirement remains busy");
    for (auto &p : g_snd.playbacks) if (p.id != -1) SND_FreePlayback(&p);
    if (g_snd.playbacksInUse) Com_Error(ERR_DROP, "Fresh audio failed canonical old registry retirement");
    for (int i = 0; i < 74; ++i)
        if (g_snd.voiceAliasHash[i]) Com_Error(ERR_DROP, "Fresh audio old voice still owns a slot");
    s_clientAudioRetired = true;
}

static void SP_VerifyFreshClientAudioPublication(bool retain = false)
{
    if (!s_clientAudioPrepared || !s_clientAudioRetired || !SD_BeginCheckpoint())
        Com_Error(ERR_DROP, "Fresh audio publication has no retired/staged owner graph");
    auto &stage = s_decodedClientAudio;
    Sys_EnterCriticalSection(CRITSECT_SOUND_PLAYBACK_ALLOC);
    memcpy(g_snd.playbacks, stage.playbacks, sizeof(stage.playbacks));
    g_snd.playbacksInUse = stage.used; g_snd.playbackIdCounter = stage.counter;
    Sys_LeaveCriticalSection(CRITSECT_SOUND_PLAYBACK_ALLOC);
    g_snd.looptime = stage.looptime; g_snd.pausetime = stage.pausetime;
    if (stage.schema >= 0x33504453) SP_PublishAudioOwners(stage.owners);
    if (stage.schema == 0x34504453) SP_PublishAudioOcclusion(stage.occlusion);
    else
    {
        // Older formats have no saved trace owners. Do not feed the old fresh
        // cgame's playback identities into the restored voice registry.
        for (auto &trace : g_snd.occlusionTraces) { trace.id = -1; trace.cache = 0; }
        g_snd.traceSequence = g_snd.lastTraceSequence = 0;
    }
    if (stage.hasMixer)
    {
        SP_PublishAudioMixer(stage.mixer);
        if (!SD_RefreshCheckpointMix()) Com_Error(ERR_DROP, "Fresh audio mixer failed actual DSP parameter readback");
        Com_Printf(15, "Client fresh audio: SDP2 mixer environment/snapshots/contexts/listener published, actual reverb/master/compressor DSP parameters verified\n");
    }
    int residents = 0, streams = 0;
    for (int i = 0; i < 74; ++i)
        if (stage.active[i])
        {
            memcpy(&g_snd.voice[i], &stage.voices[i], sizeof(snd_voice_t));
            g_snd.voiceAliasHash[i] = stage.voices[i].alias->id;
            const auto slot = stage.slots[i];
            g_snd.voice[i].playback = slot >= 0 ? &g_snd.playbacks[slot] : nullptr;
            if (!SD_RestoreCheckpointVoice(i, stage.cursors[i]))
            {
                SND_ShutdownVoices(); SP_DrainAudioCheckpointQueues(); SD_EndCheckpoint();
                Com_Error(ERR_DROP, "Fresh audio failed actual source creation at slot %i", i);
            }
            if (i < SND_MAX_STREAM_VOICES) ++streams; else ++residents;
        }
    // File/codec preparation can take time. Anchor at the final frozen boundary
    // so loader time is not charged to the resumed voices or their faders.
    const int savedTime = stage.time;
    if (!SP_RebaseAudioClock(stage, Sys_Milliseconds())) Com_Error(ERR_DROP, "Fresh audio final clock rebase overflow");
    g_snd.time = stage.time;
    int playbackQueries = 0;
    for (int i = 0; i < 74; ++i)
        if (stage.active[i])
        {
            g_snd.voice[i].startTime = stage.voices[i].startTime;
            g_snd.voice[i].soundFileInfo.endtime = stage.voices[i].soundFileInfo.endtime;
            g_snd.voice[i].voiceStartTime = stage.voices[i].voiceStartTime;
            if (stage.slots[i] >= 0 && SND_FindPlayback(stage.voices[i].playbackId) != &g_snd.playbacks[stage.slots[i]])
                Com_Error(ERR_DROP, "Fresh audio playback lookup did not rebind its real slot");
            if (stage.slots[i] >= 0)
            {
                if (SND_GetPlaybackTime(stage.voices[i].playbackId) != stage.playbacks[stage.slots[i]].playedMs
                    || SNDL_GetPlaybackTime(stage.voices[i].playbackId) != stage.time - stage.voices[i].startTime)
                    Com_Error(ERR_DROP, "Fresh audio playback APIs lost saved elapsed time");
                ++playbackQueries;
            }
        }
    static unsigned char expectedBytes[0x40000], actualBytes[0x40000];
    static MemoryFile expected, actual;
    MemFile_InitForWriting(&expected, sizeof(expectedBytes), expectedBytes, true, true);
    SPNativeCheckpointCodec normalize{nullptr, &expected}; SP_ClientAudioCodec(normalize, nullptr, &stage);
    MemFile_StartSegment(&expected, -1);
    MemFile_InitForWriting(&actual, sizeof(actualBytes), actualBytes, true, true);
    // Compare a legacy record against the same voice-only schema. The newly
    // created backend's mixer has no saved SDP1 counterpart to compare.
    SPNativeCheckpointCodec query{nullptr, &actual}; SP_ClientAudioCodec(query, nullptr, nullptr, stage.schema);
    MemFile_StartSegment(&actual, -1);
    if (expected.bytesUsed != actual.bytesUsed || memcmp(expectedBytes, actualBytes, expected.bytesUsed))
        Com_Error(ERR_DROP, "Fresh audio actual owners differ from clock-normalized saved record");
    Com_Printf(15, "Client fresh audio: FRESH PROCESS LIVE RESERIALIZATION IDENTICAL after clock rebase %i -> %i, %i resident / %i streamed sources / %u playback IDs; NO CLIENT FRAMES\n",
        savedTime, stage.time, residents, streams, stage.used);
    Com_Printf(15, "Client fresh audio: actual playback lookup/elapsed-time APIs verified for %i voice bindings\n", playbackQueries);
    if (s_continuousZombieResume && retain)
    {
        // The durable graph and real source/DSP readback already matched. A
        // normal resume must neither allocate a test ID nor advance sounds
        // during a diagnostic sleep while the saved game is still frozen.
        if (!SND_EndOcclusionCheckpoint() || !SD_EndCheckpoint())
            Com_Error(ERR_DROP, "Zombies resume audio mixer restart failed");
        s_clientAudioPrepared = s_clientAudioRetired = false;
        Com_Printf(15, "Client audio resume: %i resident / %i streamed sources / %u playback IDs retained; canonical mixer restarted\n",
            residents, streams, stage.used);
        return;
    }
    // Exercise the real allocator without colliding with any saved IDs.
    auto *allocated = SND_AllocatePlayback();
    if (!allocated || allocated->id != static_cast<int>(stage.counter + 1) || SND_FindPlayback(allocated->id) != allocated)
        Com_Error(ERR_DROP, "Fresh audio playback counter/allocator failed after publication");
    const int nextId = allocated->id; SND_FreePlayback(allocated);
    if (!SD_EndCheckpoint()) Com_Error(ERR_DROP, "Fresh audio real mixer could not restart");
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    if (!SD_BeginCheckpoint()) Com_Error(ERR_DROP, "Fresh audio real mixer could not refreeze");
    int advanced = 0;
    for (int i = 0; i < 74; ++i)
        if (stage.active[i] && (stage.voices[i].alias->flags & 1) && !stage.voices[i].paused && !stage.voices[i].startDelay)
        {
            const auto cursor = SD_BO1SamplesPlayed(i);
            if (cursor <= stage.cursors[i]) Com_Error(ERR_DROP, "Fresh audio source %i failed actual mixer sample advancement", i);
            ++advanced;
        }
    if (!advanced) Com_Error(ERR_DROP, "Fresh audio probe had no advancing loop sources");
    Com_Printf(15, "Client fresh audio: mixer advanced %i saved loop sources, real allocator issued next ID %i; NO CLIENT FRAMES\n", advanced, nextId);
    if (retain)
    {
        if (!SND_EndOcclusionCheckpoint() || !SD_EndCheckpoint()) Com_Error(ERR_DROP, "Fresh audio retained mixer restart failed");
        s_clientAudioPrepared = s_clientAudioRetired = false;
        Com_Printf(15, "Client fresh audio: voices/registry/mixer retained for bounded saved CLIENT frames\n");
        return;
    }
    SND_ShutdownVoices();
    if (!SP_DrainAudioCheckpointQueues()) Com_Error(ERR_DROP, "Fresh audio probe retirement queues remain busy");
    for (auto &p : g_snd.playbacks) if (p.id != -1) SND_FreePlayback(&p);
    if (g_snd.playbacksInUse || !SND_EndOcclusionCheckpoint() || !SD_EndCheckpoint()) Com_Error(ERR_DROP, "Fresh audio final registry/mixer retirement failed");
    s_clientAudioPrepared = s_clientAudioRetired = false;
    Com_Printf(15, "Client fresh audio: new sources/registry canonically retired, normal quit\n");
}
