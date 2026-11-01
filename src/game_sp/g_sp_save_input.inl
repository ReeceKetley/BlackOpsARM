// Included after the client codec. Only server-owned input is rebased here;
// client prediction, clocks and the gamestate handshake remain separate work.
#include <limits>
static bool s_savedPairedInputEpoch;
static float s_savedPairedInputDelta[3];
static int s_savedPairedCommandAngles[3];
static void G_RebaseSavedTransportClock(client_t *clients, int count, int previousTime, int savedTime)
{
    if (!clients || count < 1 || count > 32 || previousTime < 0 || savedTime < 0)
        Com_Error(ERR_DROP, "Saved transport clock has invalid bounds");
    struct Times { int packet, connect; } times[32]{};
    const auto rebase = [&](int stamp) {
        const long long value = static_cast<long long>(savedTime) + stamp - previousTime;
        if (value < (std::numeric_limits<int>::min)() || value > (std::numeric_limits<int>::max)())
            Com_Error(ERR_DROP, "Saved transport clock timestamp overflow");
        return static_cast<int>(value);
    };
    // These timestamps describe this live connection, not the archived world.
    // Preserve their real ages across the simulation-clock epoch change. No
    // packet/ack/sequence is manufactured and timeout policy stays unchanged.
    for (int i = 0; i < count; ++i) if (clients[i].header.state != CS_FREE)
        times[i] = {rebase(clients[i].lastPacketTime), rebase(clients[i].lastConnectTime)};
    for (int i = 0; i < count; ++i) if (clients[i].header.state != CS_FREE)
    {
        auto &client = clients[i];
        Com_Printf(15, "loadgame transport: client %i clock %i -> %i, actual packet/connect ages %lld/%lld preserved, timeout count %i\n",
            i, previousTime, savedTime, static_cast<long long>(previousTime) - client.lastPacketTime,
            static_cast<long long>(previousTime) - client.lastConnectTime, client.timeoutCount);
        client.lastPacketTime = times[i].packet; client.lastConnectTime = times[i].connect;
    }
}
static void G_RebaseSaveClientInput(gentity_s *ent, usercmd_s *transportCmd, int levelTime)
{
    gclient_s *client = ent->client;
    iassert(client);
    // The current connection owns the angle reference. The archived command's
    // angles and held buttons belong to the previous input session. Retain only
    // its name-resolved weapon selection, not movement, firing or melee intent.
    usercmd_s neutral = {};
    neutral.serverTime = client->ps.commandTime;
    memcpy(neutral.angles, transportCmd->angles, sizeof(neutral.angles));
    neutral.weapon = client->sess.cmd.weapon;
    neutral.offHandIndex = client->sess.cmd.offHandIndex;
    client->sess.cmd = neutral;
    client->sess.oldcmd = neutral;
    *transportCmd = neutral;
    client->button_bits = bitarray<51>();
    client->oldbutton_bits = bitarray<51>();
    client->latched_button_bits = bitarray<51>();
    client->button_bitsSinceLastFrame = bitarray<51>();
    client->lastCmdTime = levelTime;
    // Do not zero weapon timers/state, ADS fraction, sprint state or event
    // sequences: these describe the saved simulation, not pending input.
    client->ps.eFlags |= 2; // canonical live G_ReadClient tail
    SetClientViewAngle(ent, client->ps.viewangles);
}

static void G_RebaseInstalledSaveInput()
{
    s_savedPairedInputEpoch = false;
    int count = 0;
    for (int i = 0; i < level.maxclients; ++i)
    {
        gclient_s *client = &level.clients[i];
        if (client->sess.connected == CON_DISCONNECTED)
            continue;
        gentity_s *ent = &level.gentities[i];
        if (!ent->r.inuse || ent->client != client || client->ps.clientNum != i
            || svs.clients[i].header.state < CS_CONNECTED
            || (svs.clients[i].gentity && svs.clients[i].gentity != ent))
            Com_Error(ERR_DROP, "Saved input: client %i has no matching live connection/entity", i);
        if (i == 0)
        {
            memcpy(s_savedPairedInputDelta, client->ps.delta_angles, sizeof(s_savedPairedInputDelta));
            memcpy(s_savedPairedCommandAngles, client->sess.cmd.angles, sizeof(s_savedPairedCommandAngles));
            s_savedPairedInputEpoch = true;
        }
        G_RebaseSaveClientInput(ent, &svs.clients[i].lastUsercmd, level.time);
        // SV_InitGameVM clears this pointer. Rebind the decoded player using
        // the connection/snapshot prefix of SV_ClientEnterWorld, without its
        // ClientBegin call (which would respawn and overwrite the saved world).
        // Keep the real transport state and sequence numbers; this prepares a
        // non-delta server snapshot but does not complete a client handshake.
        svs.clients[i].gentity = ent;
        svs.clients[i].header.deltaMessage = -1;
        svs.clients[i].nextSnapshotTime = level.time;
        svs.clients[i].lastSnapshotTime = -1;
        ++count;
        Com_Printf(15, "loadgame input: client %i neutral command time %i, weapon %u offhand %u, view %.3f %.3f %.3f; client snapshots pending\n",
            i, client->ps.commandTime, (unsigned)client->sess.cmd.weapon,
            (unsigned)client->sess.cmd.offHandIndex, client->ps.viewangles[0],
            client->ps.viewangles[1], client->ps.viewangles[2]);
    }
    Com_Printf(15, "loadgame input: rebased %i server input owners; NO gameplay resumed\n", count);
}
