// Included after g_sp_save_input.inl. Runtime ownership only, never serialized.
#include <atomic>
#include <qcommon/threads.h>
#include <client_mp/sv_client_mp.h>
#include <server_mp/sv_init_mp.h>

struct SPSavedClientHandshake
{
    std::atomic<bool> pending{false};
    std::atomic<bool> worldFrozen{false};
    client_t *connection;
    gentity_s *entity;
    gclient_s *player;
    int serverId;
    int challenge;
    unsigned int scriptId;
    int frozenTime;
    int frozenFrame;
    unsigned int frozenScriptTime;
};
static SPSavedClientHandshake s_savedClientHandshake;

bool G_SP_IsSavedClientHandshakePending()
{
    return s_savedClientHandshake.pending.load(std::memory_order_acquire);
}

bool G_SP_IsSavedWorldFrozen()
{
    return s_savedClientHandshake.worldFrozen.load(std::memory_order_acquire);
}

static void G_RestoreSavedClientInputEpoch()
{
    const auto &saved = s_savedClientHandshake;
    if (!Sys_IsMainThread() || !G_SP_IsSavedWorldFrozen() || G_SP_IsSavedClientHandshakePending()
        || !s_savedPairedInputEpoch || saved.connection != &svs.clients[0]
        || saved.connection->header.state != CS_ACTIVE || saved.player != &level.clients[0]
        || saved.entity != &level.gentities[0] || saved.entity->client != saved.player
        || saved.connection->gentity != saved.entity || level.time != saved.frozenTime)
        Com_Error(ERR_DROP, "Saved CLIENT input epoch lost its frozen solo owner");
    // The first fresh command needed a fresh angle reference to reconnect.
    // Once the original CLIENT commands/viewangles return, that temporary
    // server delta would rotate their next real command by the fresh-map yaw.
    // Restore the original decoded SERVER delta and a neutral old-epoch
    // reference together; never restore held buttons or movement intent.
    auto *player = saved.player;
    usercmd_s neutral{};
    neutral.serverTime = player->ps.commandTime;
    neutral.weapon = player->sess.cmd.weapon;
    neutral.offHandIndex = player->sess.cmd.offHandIndex;
    memcpy(neutral.angles, s_savedPairedCommandAngles, sizeof(neutral.angles));
    memcpy(player->ps.delta_angles, s_savedPairedInputDelta, sizeof(player->ps.delta_angles));
    player->sess.cmd = player->sess.oldcmd = neutral;
    saved.connection->lastUsercmd = neutral;
    s_savedPairedInputEpoch = false;
    Com_Printf(15, "Client input handoff: original SERVER delta %.3f %.3f %.3f restored for saved CLIENT commands; neutral input retained\n",
        player->ps.delta_angles[0], player->ps.delta_angles[1], player->ps.delta_angles[2]);
}

void G_SP_ReleaseSavedWorldFreeze()
{
    auto &saved = s_savedClientHandshake;
    if (!saved.worldFrozen.load(std::memory_order_acquire)) return;
    if (!Sys_IsMainThread() || saved.pending.load(std::memory_order_acquire)
        || saved.connection != &svs.clients[0] || saved.connection->header.state != CS_ACTIVE
        || saved.entity != &level.gentities[0] || saved.connection->gentity != saved.entity
        || level.time != saved.frozenTime || level.framenum != saved.frozenFrame
        || gScrVarPub[SCRIPTINSTANCE_SERVER].time != saved.frozenScriptTime)
        Com_Error(ERR_DROP, "Saved world freeze lost its exact simulation/connection boundary");
    Com_Printf(15, "loadgame freeze: RELEASE exact saved server time %i/frame %i/script %u after actual reconnect and CLIENT publication\n",
        level.time, level.framenum, saved.frozenScriptTime);
    saved.worldFrozen.store(false, std::memory_order_release);
}

void G_SP_ClearSavedClientHandshake()
{
    // G_InitGame/G_ShutdownGame use the synchronized world lifecycle boundary.
    s_savedClientHandshake.pending.store(false, std::memory_order_release);
    s_savedClientHandshake.worldFrozen.store(false, std::memory_order_release);
    s_savedPairedInputEpoch = false;
}

void G_SP_CancelSavedClientHandshake(client_t *client)
{
    if ((G_SP_IsSavedClientHandshakePending() || G_SP_IsSavedWorldFrozen())
        && s_savedClientHandshake.connection == client)
        G_SP_ClearSavedClientHandshake();
}

bool G_SP_TryEnterSavedClient(client_t *client, const usercmd_s *firstCommand)
{
    SPSavedClientHandshake &saved = s_savedClientHandshake;
    if (!saved.pending.load(std::memory_order_acquire) || saved.connection != client)
        return false;
    // A stale connection must never turn into permission to skip ClientBegin.
    if (saved.serverId != sv_serverId_value || saved.challenge != client->challenge
        || (client->header.netchan.remoteAddress.type == NA_LOOPBACK && client->serverId != saved.serverId)
        || saved.scriptId != client->scriptId || client->header.state != CS_CLIENTLOADING
        || saved.entity != &level.gentities[0] || saved.player != &level.clients[0]
        || client != &svs.clients[0] || client->gentity != saved.entity
        || !saved.entity->r.inuse || saved.entity->client != saved.player
        || saved.player->sess.connected != CON_CONNECTED || saved.player->ps.clientNum != 0)
        Com_Error(ERR_DROP, "Saved client handshake: stale or mismatched player/connection");

    // The packet dispatcher serializes entry for this connection. Claim exactly
    // once; a second concurrent claimant must not fall through to a respawn.
    if (!saved.pending.exchange(false, std::memory_order_acq_rel))
        Com_Error(ERR_DROP, "Saved client handshake: duplicate entry");
    usercmd_s reference = *firstCommand;
    G_RebaseSaveClientInput(saved.entity, &reference, level.time);
    client->lastUsercmd = reference;
    client->header.deltaMessage = -1;
    client->nextSnapshotTime = level.time;
    client->lastSnapshotTime = -1;
    client->header.state = CS_ACTIVE;
    // No ClientConnect/ClientBegin: the saved VM and player already own their
    // session. The next full snapshot establishes the client's real clock.
    Com_Printf(15, "loadgame handshake: first command rebased at %i, restored player entered without respawn; snapshot delivery still unverified\n",
        saved.player->ps.commandTime);
    return true;
}

static void G_BeginSavedClientHandshake()
{
    // This diagnostic supports only the one local solo player. Validate the
    // entire set before sending anything; other connected slots are not saves
    // we can reconnect safely yet. NA_BOT exercises real gamestate encoding in
    // the headless verifier, not a rendered client or actual packet reception.
    if (s_savedClientHandshake.pending.load(std::memory_order_acquire))
        Com_Error(ERR_DROP, "Saved client handshake: already pending");
    int connected = 0;
    for (int i = 0; i < level.maxclients; ++i)
    {
        if (level.clients[i].sess.connected == CON_DISCONNECTED
            && svs.clients[i].header.state < CS_CONNECTED)
            continue;
        if (i != 0 || level.clients[i].sess.connected != CON_CONNECTED)
            Com_Error(ERR_DROP, "Saved client handshake: only connected solo client 0 is supported");
        ++connected;
    }
    client_t *client = &svs.clients[0];
    gentity_s *ent = &level.gentities[0];
    const netadrtype_t address = client->header.netchan.remoteAddress.type;
    if (connected != 1 || client->header.state != CS_ACTIVE
        || (address != NA_LOOPBACK && address != NA_BOT)
        || client->gentity != ent || !ent->r.inuse || ent->client != &level.clients[0]
        || level.clients[0].ps.clientNum != 0)
        Com_Error(ERR_DROP, "Saved client handshake: missing local solo player ownership");
    SPSavedClientHandshake &saved = s_savedClientHandshake;
    saved.connection = client;
    saved.entity = ent;
    saved.player = ent->client;
    saved.serverId = sv_serverId_value;
    saved.challenge = client->challenge;
    saved.scriptId = client->scriptId;
#if defined(__ANDROID__)
    if (G_SP_CheckpointOption("bo1_restore_freeze_world"))
    {
        if (address != NA_LOOPBACK || !G_SP_CheckpointOption("bo1_restore_saved_client")
            || (!G_SP_IsContinuousZombieResume() && !Dvar_GetInt("bo1_restore_client_frames")))
            Com_Error(ERR_DROP, "Saved world freeze requires retained paired local CLIENT restore");
        saved.frozenTime = level.time; saved.frozenFrame = level.framenum;
        saved.frozenScriptTime = gScrVarPub[SCRIPTINSTANCE_SERVER].time;
        saved.worldFrozen.store(true, std::memory_order_release);
        Com_Printf(15, "loadgame freeze: BEGIN saved server time %i/frame %i/script %u; real reconnect transport remains active\n",
            saved.frozenTime, saved.frozenFrame, saved.frozenScriptTime);
    }
#endif
    saved.pending.store(true, std::memory_order_release);
    SV_SendClientGameState(client);
    if (client->header.state != CS_CLIENTLOADING)
        Com_Error(ERR_DROP, "Saved client handshake: gamestate was not sent");
    Com_Printf(15, "loadgame handshake: fresh gamestate sent for restored solo player; awaiting actual first command, NO gameplay resumed\n");
}
