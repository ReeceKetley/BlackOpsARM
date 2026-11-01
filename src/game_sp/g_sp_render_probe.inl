// Diagnostic continuation through the real Com_Frame loop. Never a public load.
#include <cgame/cg_weapons.h>
#include <client_mp/cl_main_mp.h>
#include <cgame/cg_scr_main.h>
#include <bgame/bg_weapons_def.h>
#include <qcommon/actor_model_state.h>
#include <game_sp/g_scr_sp_entity.h>
#include <game_mp/g_utils_mp.h>
#include <clientscript/cscr_stringlist.h>
#include <xanim/xanim.h>
#include <cgame_mp/cg_local_mp.h>
#include <game_sp/actor_sp_ext.h>
#include <game_mp/actor_mp.h>
#include <ragdoll/ragdoll.h>

static void SP_ClearAbandonedClientAnimationNodes()
{
    // DObjs continue borrowing their tree headers until the fresh gamestate
    // shuts down the old entities. Preserve those header identities; only the
    // nodes and their old-VM notify/bone-map references are retired here.
    const auto clear = [](XAnimTree_s *tree) { if (tree) XAnimClearTree(tree); };
    cg_s *cg = CG_GetLocalClientGlobals(0);
    cgs_t *cgs = CG_GetLocalClientStaticGlobals(0);
    for (int i = 0; i < com_maxclients->current.integer; ++i)
        clear(cg->bgs.clientinfo[i].pXAnimTree);
    for (int i = 0; i < 4; ++i)
        clear(cgs->corpseinfo[i].pXAnimTree);
    for (int i = 0; i < MAX_ACTORS; ++i)
        clear(BG_SP_GetActorInfo(&cg->bgs, i)->pXAnimTree);
    for (int i = 0; i < MAX_ACTOR_CORPSES_SP; ++i)
        clear(cgs->actorCorpseInfo[i].pXAnimTree);
}

// Capture while the server is joined, before allowing any restored frames.
// Inline tree headers own deep-cloned canonical XAnim nodes, never a borrower
// of the server tree that the next worker frame can change or retire.
struct SPRenderedCorpseCheckpoint
{
    bool pending;
    int entityNum;
    int useCount;
    int animtreeIndex;
    SPActorModelInfo models;
    XAnimTree_s tree;
};
static SPRenderedCorpseCheckpoint s_renderCorpses[MAX_ACTOR_CORPSES_SP];

static void SP_ClearServerNotificationsFromClientPose(XAnimTree_s *tree, unsigned int node)
{
    for (; node; node = GetAnimInfo(node)->next)
    {
        XAnimInfo *info = GetAnimInfo(node);
        // DObjCreateExt changes the receiving tree's instance to CLIENT.
        // Server notify handles cannot follow that change. Like the canonical
        // XAnimSyncServerTree_r path, the client pose starts without server
        // callbacks; snapshot commands supply the client's own notetracks.
        XAnimClearServerNotify(info, SCRIPTINSTANCE_SERVER);
        info->notifyChild = 0;
        info->notifyType = 0;
        SP_ClearServerNotificationsFromClientPose(tree, info->children);
    }
}

static void SP_ClearRenderedCorpses()
{
    for (SPRenderedCorpseCheckpoint &corpse : s_renderCorpses)
    {
        if (corpse.tree.anims)
            XAnimFreeTree(&corpse.tree, nullptr, SCRIPTINSTANCE_SERVER);
        corpse = {};
    }
}

static void SP_CaptureRenderedCorpses()
{
    SP_ClearRenderedCorpses();
    for (int i = 0; i < MAX_ACTOR_CORPSES_SP; ++i)
    {
        const int number = g_scr_data.actorCorpseInfo[i].entnum;
        if (number < 0)
            continue;
        if (number >= level.num_entities)
            Com_Error(ERR_DROP, "Saved corpse has invalid entity %i", number);
        const gentity_s &ent = g_entities[number];
        if (!ent.r.inuse || ent.s.eType != ET_ACTOR_CORPSE || ent.s.lerp.u.actor.corpseNum != i)
            Com_Error(ERR_DROP, "Saved corpse slot %i lost its entity owner", i);
        XAnimTree_s *tree = g_scr_data.actorCorpseInfo[i].tree;
        if (!tree || !tree->anims || !ent.model)
            Com_Error(ERR_DROP, "Saved corpse %i has no logical model/animation owner", number);
        SPRenderedCorpseCheckpoint &corpse = s_renderCorpses[i];
        corpse.entityNum = number;
        corpse.useCount = ent.s.lerp.useCount;
        corpse.animtreeIndex = ent.s.animtreeIndex;
        corpse.models.attachIgnoreCollision = ent.attachIgnoreCollision;
        I_strncpyz(corpse.models.model, SL_ConvertToString(G_ModelName(ent.model), SCRIPTINSTANCE_SERVER), 64);
        for (int j = 0; j < 6; ++j)
        {
            if (!ent.attachModelNames[j])
                continue;
            I_strncpyz(corpse.models.attachModelNames[j], SL_ConvertToString(G_ModelName(ent.attachModelNames[j]), SCRIPTINSTANCE_SERVER), 64);
            I_strncpyz(corpse.models.attachTagNames[j], SL_ConvertToString(ent.attachTagNames[j], SCRIPTINSTANCE_SERVER), 64);
        }
        corpse.tree.anims = tree->anims;
        XAnimCloneAnimTree(tree, &corpse.tree);
        SP_ClearServerNotificationsFromClientPose(&corpse.tree, corpse.tree.children);
        corpse.pending = true;
        Com_Printf(15, "loadgame corpse: captured ent %i slot %i generation %i model %s tree nodes %u\n",
            number, i, corpse.useCount, corpse.models.model, corpse.tree.children);
    }
}

static void SP_PrepareRenderedSaveClient()
{
    if (Sys_IsHeadless() || CL_GetLocalClientConnectionState(0) != CA_ACTIVE
        || svs.clients[0].header.state != CS_ACTIVE
        || svs.clients[0].header.netchan.remoteAddress.type != NA_LOOPBACK
        || (!G_SP_IsContinuousZombieResume() && !Dvar_GetBool("bo1_restore_verify_quit"))
        || !G_SP_CheckpointOption("bo1_restore_live_tables")
        || !G_SP_CheckpointOption("bo1_restore_animation") || !G_SP_CheckpointOption("bo1_restore_input")
        || !G_SP_CheckpointOption("bo1_restore_handshake") || Dvar_GetInt("bo1_restore_probe_frames"))
        Com_Error(ERR_DROP, "Rendered save probe requires an active local solo client before restart");
    // The renderer can have registered additional weapons in a different order.
    // Retire its real viewmodel/tree before replacing the server VM, then release
    // whichever canonical weapon-table owner is active. SP fastfile definitions
    // remain asset-owned; G_SetupWeaponDef and the saved names rebuild indices.
    const unsigned int oldWeapons = BG_GetNumWeapons();
    // Each cached translation owns refs in BOTH script VMs. Retire these
    // cross-VM borrowers while the abandoned server table is still live.
    CScr_ClearConstServerStringCache();
    CG_FreeWeapons(0);
    // Same-map CL_RestartCGame keeps these tree headers. Their nodes can still
    // own notify strings in the abandoned server VM, so retire them BEFORE
    // replacing that VM, retaining the headers borrowed by existing DObjs.
    SP_ClearAbandonedClientAnimationNodes();
    Com_FreeWeaponInfoMemory(2);
    Com_FreeWeaponInfoMemory(1);
    if (BG_GetNumWeapons() != 1)
        Com_Error(ERR_DROP, "Rendered save probe failed to release the previous weapon-table owner");
    Com_Printf(15, "loadgame render: retired viewmodel and %u old weapon slots before VM restart\n", oldWeapons);
}

struct SPRenderedSaveProbe
{
    bool active;
    bool firstSnapshot;
    int frames;
    int startFrame;
    int startTime;
    int lastLogged;
    unsigned int startMs;
    bool physicsChecked;
    bool nativeOwnersChecked;
    bool savedClientInstalled;
    unsigned int savedClientScriptTime;
};
static SPRenderedSaveProbe s_renderSaveProbe;

void SP_ClearRenderedSaveProbe()
{
    // Called at the synchronized game init/shutdown boundary, including errors.
    SP_ClearRenderedCorpses();
    s_renderSaveProbe = {};
}

bool SP_TakeRenderedSavedCorpse(const entityState_s *state, SPActorModelInfo *models, XAnimTree_s *clientTree)
{
    if (!s_renderSaveProbe.active || state->eType != ET_ACTOR_CORPSE)
        return false;
    const unsigned int slot = state->lerp.u.actor.corpseNum;
    if (slot >= MAX_ACTOR_CORPSES_SP)
        Com_Error(ERR_DROP, "Saved corpse receiver has invalid slot");
    SPRenderedCorpseCheckpoint &corpse = s_renderCorpses[slot];
    if (!corpse.pending)
        return false;
    if (state->number != corpse.entityNum || state->lerp.useCount != corpse.useCount)
    {
        // The corpse expired before being visible; a reused slot must never
        // inherit its predecessor's model or pose.
        XAnimFreeTree(&corpse.tree, nullptr, SCRIPTINSTANCE_SERVER);
        corpse = {};
        return false;
    }
    if (state->animtreeIndex != corpse.animtreeIndex || clientTree->anims != corpse.tree.anims)
        Com_Error(ERR_DROP, "Saved corpse receiver animation identity mismatch");
    *models = corpse.models;
    XAnimCloneAnimTree(&corpse.tree, clientTree);
    XAnimFreeTree(&corpse.tree, nullptr, SCRIPTINSTANCE_SERVER);
    corpse = {};
    return true;
}

static void SP_BeginRenderedSaveProbe(SPSaveGame *save, int frames)
{
    if (Sys_IsHeadless() || !save || save->state != SP_SAVE_STATE_READING
        || frames < 1 || frames > 200 || Dvar_GetInt("bo1_restore_probe_frames")
        || !G_SP_CheckpointOption("bo1_restore_partial")
        || (!G_SP_IsContinuousZombieResume() && !Dvar_GetBool("bo1_restore_verify_quit"))
        || !G_SP_CheckpointOption("bo1_restore_live_tables") || !G_SP_CheckpointOption("bo1_restore_animation")
        || !G_SP_CheckpointOption("bo1_restore_input") || !G_SP_CheckpointOption("bo1_restore_handshake")
        || !G_SP_IsSavedClientHandshakePending() || !level.initializing || svs.time != level.time
        || svs.clients[0].bIsTestClient || svs.clients[0].header.netchan.remoteAddress.type != NA_LOOPBACK
        || CL_GetLocalClientConnectionState(0) != CA_ACTIVE)
        Com_Error(ERR_DROP, "Rendered save probe requires the existing local solo client and verified world");
    Com_SyncThreads();
    SP_FinishSavedProbeInitialization(save);
    SP_CaptureRenderedCorpses();
    Dvar_SetIntByName("cl_paused", 0);
    s_renderSaveProbe = {true, false, frames, level.framenum, level.time, -1, (unsigned)Sys_Milliseconds()};
    Com_Printf(15, "loadgame render: BEGIN bounded %i frames time %i frame %i; real client receive/render pending\n",
        frames, level.time, level.framenum);
}

static void SP_PollRenderedSaveProbe()
{
    // Wait for the previous worker frame before reading its permanent owners.
    Com_SyncThreads();
    SPRenderedSaveProbe &probe = s_renderSaveProbe;
    const int completed = level.framenum - probe.startFrame;
    clientActive_t *client = CL_GetLocalClientGlobals(0);
    const int state = CL_GetLocalClientConnectionState(0);
    if (completed < 0 || level.time < probe.startTime || !g_entities[0].r.inuse
        || g_entities[0].client != &level.clients[0] || svs.clients[0].gentity != &g_entities[0])
        Com_Error(ERR_DROP, "Rendered save probe lost its restored world/player owner");
    if (state == CA_ACTIVE && client && client->snap.valid && client->snap.serverTime >= probe.startTime
        && client->snap.ps.clientNum == 0 && !G_SP_IsSavedClientHandshakePending())
    {
        if (!probe.firstSnapshot)
            Com_Printf(15, "loadgame render: FIRST restored snapshot time %i command %i clientClock %i delta %i weapon %u view %.3f %.3f %.3f\n",
                client->snap.serverTime, client->snap.ps.commandTime, client->serverTime, client->serverTimeDelta,
                client->snap.ps.weapon, client->snap.ps.viewangles[0], client->snap.ps.viewangles[1], client->snap.ps.viewangles[2]);
        probe.firstSnapshot = true;
    }
    if (completed / 20 != probe.lastLogged)
    {
        probe.lastLogged = completed / 20;
        Com_Printf(15, "loadgame render: progress %i/%i time %i clientState %i snapshot %i health %i points %i\n",
            completed, probe.frames, level.time, state, client ? client->snap.serverTime : -1,
            g_entities[0].health, level.clients[0].sess.cs.score.score);
        if (probe.firstSnapshot)
            for (int i = 0; i < MAX_ACTOR_CORPSES_SP; ++i)
            {
                const int number = g_scr_data.actorCorpseInfo[i].entnum;
                if (number < 0 || number >= level.num_entities || !g_entities[number].r.inuse)
                    continue;
                const centity_s *cent = CG_GetEntity(0, number);
                if (!cent->nextValid || cent->nextState.eType != ET_ACTOR_CORPSE
                    || cent->nextState.lerp.useCount != g_entities[number].s.lerp.useCount
                    || !cent->pose.ragdollHandle)
                    continue;
                RagdollBody *body = Ragdoll_HandleBody(cent->pose.ragdollHandle);
                Com_Printf(15, "loadgame corpse: observed ent %i slot %i state %i bones %i modelOwner %i\n",
                    number, i, body->state, body->numBones, Ragdoll_BodyDObj(body) != nullptr);
            }
    }
    // Allow a full minute of resumed play plus real pause/menu observation.
    // The reconnect/publication phase retains its original shorter bound.
    const unsigned wallLimit = probe.savedClientInstalled ? 120000 : 60000;
    if ((unsigned)Sys_Milliseconds() - probe.startMs >= wallLimit)
        Com_Error(ERR_DROP, "Rendered save probe exceeded its bounded wall-clock limit");
#if defined(__ANDROID__)
    if (probe.firstSnapshot && !probe.savedClientInstalled && G_SP_CheckpointOption("bo1_restore_saved_client"))
    {
        auto *cg = CG_GetLocalClientGlobals(0);
        if (cg->snap && cg->nextSnap && Scr_IsStackClear(SCRIPTINSTANCE_CLIENT) && Scr_IsStackClear(SCRIPTINSTANCE_SERVER))
        {
            Com_Printf(15, "loadgame paired: saved server LIVE time %i frame %i, client snapshot %i; applying matching durable CLIENT before saved client frames\n",
                level.time, level.framenum, cg->snap->serverTime);
            G_SP_VerifyClientCheckpoint();
            if (G_SP_IsContinuousZombieResume())
            {
                // The verified owners are now owned by canonical gameplay.
                // End the loader, not the session; no probe timer survives.
                probe.active = false;
                SP_ClearRenderedCorpses();
                // Keep the exact saved world frozen while normal transport
                // establishes the pause menu's real snapshot acknowledgement.
                if (!CL_PauseRestoredSoloSession())
                    Com_Error(ERR_DROP, "Restored solo session could not hold its pause boundary");
                return;
            }
            if (!Dvar_GetInt("bo1_restore_client_frames")) Com_Error(ERR_DROP, "Paired saved client verification unexpectedly returned");
            probe.savedClientInstalled = true;
            probe.savedClientScriptTime = gScrVarPub[SCRIPTINSTANCE_CLIENT].time;
            probe.startFrame = level.framenum; probe.startTime = level.time;
            probe.frames = Dvar_GetInt("bo1_restore_client_frames"); probe.startMs = Sys_Milliseconds(); probe.lastLogged = -1;
            Com_Printf(15, "loadgame saved CLIENT: BEGIN %i real frames at server %i/script %u with retained native owners\n",
                probe.frames, level.time, probe.savedClientScriptTime);
            return;
        }
    }
    if (probe.firstSnapshot && completed >= 60 && !probe.nativeOwnersChecked && Dvar_GetBool("bo1_restore_native_owners"))
    {
        G_SP_VerifyRenderedNativeOwners();
        probe.nativeOwnersChecked = true;
    }
    if (probe.firstSnapshot && completed >= 40 && !probe.physicsChecked && Dvar_GetBool("bo1_restore_physics"))
    {
        G_SP_VerifyRenderedRagdollCheckpoint();
        probe.physicsChecked = true;
    }
#endif
    if (completed >= probe.frames)
    {
        if (!probe.firstSnapshot || state != CA_ACTIVE || svs.clients[0].header.state != CS_ACTIVE)
            Com_Error(ERR_DROP, "Rendered save probe completed without an active restored client snapshot");
        probe.active = false;
        if (probe.savedClientInstalled)
        {
            if (gScrVarPub[SCRIPTINSTANCE_CLIENT].time <= probe.savedClientScriptTime)
                Com_Error(ERR_DROP, "Saved CLIENT frame probe failed to advance its actual script VM");
            Com_Printf(15, "loadgame saved CLIENT: END %i real server/client frames, CLIENT script %u -> %u, prediction command %i, snapshot %i; normal diagnostic quit\n",
                completed, probe.savedClientScriptTime, gScrVarPub[SCRIPTINSTANCE_CLIENT].time,
                CG_GetLocalClientGlobals(0)->predictedPlayerState.commandTime, client->snap.serverTime);
        }
        Com_Printf(15, "loadgame render: END %i real server frames with active restored local snapshots; normal diagnostic quit, public Resume incomplete\n", completed);
        Com_Quit_f();
        Com_Error(ERR_FATAL, "Rendered save probe quit unexpectedly returned");
    }
}
