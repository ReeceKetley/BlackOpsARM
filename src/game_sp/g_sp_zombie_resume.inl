// Normal solo disk save/resume entry. Uses the same canonical serializers and
// native handoff as the integration tests, with no diagnostic frame lifetime.
#include <platform/android/launcher_session.h>
#include <cgame/cg_ammocounter.h>
#include <bgame/bg_mantle.h>
#include <bgame/bg_weapons_ammo.h>
#include <ui/ui_shared.h>
#include "g_sp_fx_physics_probe.inl"
#include "g_sp_map_owner_inventory.inl"

static bool s_pauseOnRagdollVelocity;
static bool s_pauseOnRagdollAttachment;
static bool s_pauseOnServerBody;
static int s_serverBodyProbeIds[MAX_GENTITIES], s_serverBodyProbeGenerations[MAX_GENTITIES];
void G_SP_InspectServerBodies()
{
    if (!Sys_IsMainThread() || CL_GetLocalClientConnectionState(0) != CA_ACTIVE
        || !G_SP_IsSPLevel() || SP_IsFrontendMapForSave()) return;
    Com_SyncThreads(); int count = 0;
    for (const auto &ent : g_entities)
        if (ent.r.inuse && ent.physObjId && ent.physObjId != -1)
        {
            ++count; SPServerBodyRecord r; const bool supported = SP_ReadLiveServerBody(ent, r);
            Com_Printf(15, "Server body observation: time %i entity %i generation %i id %i supported %i model %s matrix-origin %.9g %.9g %.9g velocity %.9g %.9g %.9g flags %u\n",
                level.time, ent.s.number, ent.useCount, ent.physObjId, supported, r.modelName.c_str(),
                r.state.matrix[3][0], r.state.matrix[3][1], r.state.matrix[3][2],
                r.state.m_t_vel[0], r.state.m_t_vel[1], r.state.m_t_vel[2], r.state.bodyFlags);
        }
    for (const auto &r : s_serverBodies)
    {
        const auto &ent = g_entities[r.number];
        if (SP_ServerBodyMatchesEntity(r, ent) && (!ent.physObjId || ent.physObjId == -1))
            Com_Printf(15, "Server body observation: time %i saved entity %i generation %i native body retired id %i origin %.9g %.9g %.9g\n",
                level.time, r.number, r.generation, ent.physObjId, ent.r.currentOrigin[0], ent.r.currentOrigin[1], ent.r.currentOrigin[2]);
    }
    Com_Printf(15, "Server body observation: time %i active %i saved owners %u; no state changed\n", level.time, count, (unsigned)s_serverBodies.size());
}
void G_SP_ArmServerBodyPause()
{
    if (!Sys_IsMainThread() || CL_GetLocalClientConnectionState(0) != CA_ACTIVE
        || !G_SP_IsSPLevel() || SP_IsFrontendMapForSave() || s_publicZombieCapture || s_continuousZombieResume) return;
    if (Cmd_Argc() > 1 && !strcmp(Cmd_Argv(1), "0")) { s_pauseOnServerBody = false; return; }
    Com_SyncThreads();
    for (int i = 0; i < MAX_GENTITIES; ++i)
    {
        s_serverBodyProbeIds[i] = g_entities[i].r.inuse ? g_entities[i].physObjId : 0;
        s_serverBodyProbeGenerations[i] = g_entities[i].useCount;
    }
    s_pauseOnServerBody = true;
    Com_Printf(15, "Server body boundary probe: armed for next naturally created owner; no body/event injected\n");
}
static void SP_PollServerBodyPause()
{
    if (!s_pauseOnServerBody || Dvar_GetInt("cl_paused") || s_continuousZombieResume
        || CL_GetLocalClientConnectionState(0) != CA_ACTIVE || !G_SP_IsSPLevel() || SP_IsFrontendMapForSave()) return;
    Com_SyncThreads();
    for (int i = 0; i < MAX_GENTITIES; ++i)
    {
        const auto &ent = g_entities[i];
        if (!ent.r.inuse || !ent.physObjId || ent.physObjId == -1
            || (ent.physObjId == s_serverBodyProbeIds[i] && ent.useCount == s_serverBodyProbeGenerations[i])) continue;
        s_pauseOnServerBody = false;
        SPServerBodyRecord record; const bool supported = SP_ReadLiveServerBody(ent, record);
        Com_Printf(15, "Server body boundary probe: actual new entity %i generation %i owner %i supported %i; invoking canonical pause\n",
            i, ent.useCount, ent.physObjId, supported);
        char command[] = "pause"; Cmd_ExecuteSingleCommand(0, 0, command); return;
    }
}
void G_SP_ArmSaveCaptureErrorProbe()
{
    if (!Sys_IsMainThread() || CL_GetLocalClientConnectionState(0) != CA_ACTIVE
        || !G_SP_IsSPLevel() || SP_IsFrontendMapForSave() || s_publicZombieCapture) return;
    s_publicZombieCaptureErrorProbe = true;
    Com_Printf(15, "Save capture recovery probe: armed once for the next public capture after mixer freeze\n");
}
void G_SP_RequestResumeLifecycleTrace()
{
    if (!Sys_IsMainThread() || CL_GetLocalClientConnectionState(0) != CA_ACTIVE
        || !G_SP_IsSPLevel() || SP_IsFrontendMapForSave()) return;
    G_SP_BeginResumeLifecycleTrace();
    Com_Printf(15, "Resume lifecycle: explicit read-only observation requested\n");
}
void G_SP_ArmRagdollAttachmentPause()
{
    if (!Sys_IsMainThread() || CL_GetLocalClientConnectionState(0) != CA_ACTIVE
        || !G_SP_IsSPLevel() || SP_IsFrontendMapForSave() || Dvar_GetInt("cl_paused")) return;
    s_pauseOnRagdollAttachment = true;
    Com_Printf(15, "Ragdoll attachment boundary probe: armed for next real attachment event\n");
}
void G_SP_NotifyRagdollAttachmentBoundary(int entity, int target, int hitloc)
{
    if (!s_pauseOnRagdollAttachment || Dvar_GetInt("cl_paused")) return;
    s_pauseOnRagdollAttachment = false;
    Com_Printf(15, "Ragdoll attachment boundary probe: real entity %i target %i hitloc %i invoking canonical pause\n", entity, target, hitloc);
    char command[] = "pause";
    Cmd_ExecuteSingleCommand(0, 0, command);
}
void G_SP_ArmRagdollVelocityPause()
{
    if (!Sys_IsMainThread() || CL_GetLocalClientConnectionState(0) != CA_ACTIVE
        || !G_SP_IsSPLevel() || SP_IsFrontendMapForSave() || Dvar_GetInt("cl_paused")) return;
    s_pauseOnRagdollVelocity = true;
    Com_Printf(15, "Ragdoll boundary probe: armed for next real velocity-capture owner\n");
}
void G_SP_NotifyRagdollVelocityBoundary(int entity)
{
    if (!s_pauseOnRagdollVelocity || Dvar_GetInt("cl_paused")) return;
    s_pauseOnRagdollVelocity = false;
    Com_Printf(15, "Ragdoll boundary probe: real entity %i in velocity capture, invoking canonical pause\n", entity);
    char command[] = "pause";
    Cmd_ExecuteSingleCommand(0, 0, command);
}

void G_SP_InspectZombiesResumeState()
{
    if (!Sys_IsMainThread() || CL_GetLocalClientConnectionState(0) != CA_ACTIVE
        || !G_SP_IsSPLevel() || SP_IsFrontendMapForSave()) return;
    Com_SyncThreads();
    auto *cg = CG_GetLocalClientGlobals(0);
    SPClientHudState hud; SP_ReadClientHud(hud); SP_TraceClientHud("query", hud);
    const auto &ps = cg->predictedPlayerState;
    const unsigned weapon = GetWeaponIndex(cg), counter = ClipCounterWeapIdx(cg, weapon);
    Com_Printf(15, "Zombies resume state: time %i physics %i ammoFade %i fadeSeconds %.3f alpha %.3f hidden %i hud %i failed %i weapon %u selected %u counter %u state %i eFlags %x weapFlags %x clip %i reserve %i\n",
        cg->time, cg->physicsTime, cg->ammoFadeTime, Dvar_GetFloat("hud_fade_ammodisplay"),
        AmmoCounterFadeAlpha(0, cg), Dvar_GetBool("ammoCounterHide"), Dvar_GetBool("hud_enable"),
        Dvar_GetBool("hud_missionFailed"), ps.weapon, cg->weaponSelect, counter, ps.weaponstate,
        ps.eFlags, ps.weapFlags, counter ? BG_GetAmmoInClipForWeaponDef(&ps, BG_GetWeaponVariantDef(counter)) : -1,
        counter ? BG_GetTotalAmmoReserve(&ps, counter) : -1);
    for (const char *name : {"weaponinfozombies", "weaponinfo", "weaponinfo_lowdef", "weaponinfo_mp"})
    {
        const auto *menu = Menus_FindByName(&cgDC[0], name);
        Com_Printf(15, "Zombies resume HUD: menu %s present %i flags %x items %i\n", name, menu != nullptr,
            menu ? menu->window.dynamicFlags[0] : 0, menu ? menu->itemCount : 0);
    }
    int missiles = 0, states[6]{};
    for (int i = 0; i < MAX_GENTITIES; ++i)
        missiles += level.gentities[i].r.inuse && level.gentities[i].s.eType == ET_MISSILE;
    for (const auto *body : g_ragdoll_body_pool)
        if (static_cast<unsigned>(body->state) < 6) ++states[body->state];
    Com_Printf(15, "Zombies resume server: time %i frame %i script %u health %i score %i godFlags %u missiles %i; ragdolls wait %i velocity %i tunnel %i running %i idle %i\n",
        level.time, level.framenum, gScrVarPub[SCRIPTINSTANCE_SERVER].time, level.gentities[0].health,
        level.clients[0].sess.cs.score.score, level.gentities[0].flags & 3u, missiles,
        states[1], states[2], states[3], states[4], states[5]);
}

static bool s_publicZombiePreviewRead, s_publicZombieAvailable;
static std::string s_publicZombieMap;
static void SP_ZombieSessionResult(bool success, const char *message)
{
    ++s_publicZombieResult; s_publicZombieSuccess = success; s_publicZombieMessage = message;
}
static bool SP_ReadPublicZombieSnapshot(bo1::android::save_store::Snapshot &snapshot, SPMemorySaveHeader &header)
{
    std::string error;
    const auto root = std::string(Sys_EngineUserRoot()) + "/zombies-resume";
    if (!bo1::android::save_store::Read(root.c_str(), snapshot, error)
        || snapshot.flags != bo1::android::save_store::SoloResumePayload
        || snapshot.metadata.size() != sizeof(header) || snapshot.client.empty())
        return false;
    memcpy(&header, snapshot.metadata.data(), sizeof(header));
    if (header.saveVersion != 0x134 || header.bodySize <= 0 || header.pad <= 0 || header.health <= 0
        || static_cast<size_t>(header.bodySize) != snapshot.game.size()
        || static_cast<size_t>(header.pad) != snapshot.client.size()
        || !memchr(header.mapName, 0, sizeof(header.mapName)) || !header.mapName[0]
        || strspn(header.mapName, "abcdefghijklmnopqrstuvwxyz0123456789_") != strlen(header.mapName))
        return false;
    return true;
}
static void SP_RefreshPublicZombiePreview()
{
    bo1::android::save_store::Snapshot snapshot; SPMemorySaveHeader header{};
    s_publicZombieAvailable = SP_ReadPublicZombieSnapshot(snapshot, header);
    s_publicZombieMap = s_publicZombieAvailable ? header.mapName : "";
    s_publicZombiePreviewRead = true;
}

static void SP_SavePublicZombiesSession(bool leave)
{
    if (!Sys_IsMainThread() || s_publicZombieCapture || s_continuousZombieResume) return;
    s_publicZombieCapture = true;
    G_SP_CaptureClientCheckpoint();
    s_publicZombieCapture = false;
    if (!s_publicZombieCaptureSucceeded)
    {
        Com_Printf(15, "Zombies save: current session cannot be saved at this boundary; previous disk save retained\n");
        SP_ZombieSessionResult(false, "This session cannot be saved right now");
        return;
    }
    SP_RefreshPublicZombiePreview();
    SP_ZombieSessionResult(true, "Session saved");
    Com_Printf(15, "Zombies save: DURABLE normal solo archive; %s\n", leave ? "leaving after confirmed save" : "continuous gameplay continues");
    if (leave) Com_Quit_f();
}
void G_SP_SaveZombiesSession() { SP_SavePublicZombiesSession(false); }
void G_SP_SaveAndLeaveZombiesSession() { SP_SavePublicZombiesSession(true); }

void G_SP_ResumeZombiesSession()
{
    if (!Sys_IsMainThread() || Sys_IsHeadless() || s_publicZombieCapture || s_continuousZombieResume
        || s_clientModelProbeActive || CL_GetLocalClientConnectionState(0) != CA_ACTIVE
        || !G_SP_IsSPLevel() || SP_IsFrontendMapForSave())
    {
        Com_Printf(15, "Zombies resume: requires a fresh local map session\n");
        SP_ZombieSessionResult(false, "Start Resume from the launcher");
        return;
    }
    bo1::android::save_store::Snapshot snapshot;
    SPMemorySaveHeader header{};
    if (!SP_ReadPublicZombieSnapshot(snapshot, header))
    {
        Com_Printf(15, "Zombies resume: no intact normal solo save; diagnostic archives are excluded\n");
        SP_ZombieSessionResult(false, "No supported saved session was found");
        return;
    }
    if (strcmp(header.mapName, Dvar_GetString("mapname")))
    {
        Com_Printf(15, "Zombies resume: load saved map %s before restoring\n", header.mapName);
        SP_ZombieSessionResult(false, "Load the saved map before restoring");
        return;
    }
    Com_SyncThreads();
    s_continuousZombieResume = true;
    s_savedClientPublicationReady = false;
    if (!G_SP_ImportSaveArchiveForVerification())
    {
        s_continuousZombieResume = false;
        Com_Printf(15, "Zombies resume: archive rejected before world replacement\n");
        SP_ZombieSessionResult(false, "The save does not match the installed map or scripts");
        return;
    }
    Cbuf_AddText(0, "loadgame_continue\n");
    Com_Printf(15, "Zombies resume: normal solo generation %llu queued with complete logical restore and exact world freeze\n",
        static_cast<unsigned long long>(snapshot.generation));
}

void G_SP_PollZombiesSession()
{
    if (!Sys_IsMainThread() || Sys_IsHeadless()) return;
    SP_PollServerBodyPause();
    SP_PollDynEntMotionPause();
    namespace session = bo1::android::launcher_session;
    if (!s_publicZombiePreviewRead) SP_RefreshPublicZombiePreview();
    const int action = session::TakeResumeAction();
    if (action == 1) G_SP_SaveZombiesSession();
    if (action == 2) G_SP_SaveAndLeaveZombiesSession();
    if (action == 3)
    {
        SP_RefreshPublicZombiePreview();
        if (!s_publicZombieAvailable) SP_ZombieSessionResult(false, "No supported saved session was found");
        else
        {
            s_publicResumeWaitingForMap = true;
            if (SP_IsFrontendMapForSave() && !session::RequestMap(s_publicZombieMap))
            {
                s_publicResumeWaitingForMap = false;
                SP_ZombieSessionResult(false, "The saved map could not be started");
            }
        }
    }
    const bool match = CL_GetLocalClientConnectionState(0) == CA_ACTIVE
        && G_SP_IsSPLevel() && !SP_IsFrontendMapForSave() && Dvar_GetBool("sv_running");
    if (s_publicResumeWaitingForMap && match)
    {
        s_publicResumeWaitingForMap = false;
        G_SP_ResumeZombiesSession();
        if (!s_continuousZombieResume) Cbuf_AddText(0, "disconnect\n");
    }
    session::ResumeInfo info;
    info.available = s_publicZombieAvailable; info.map = s_publicZombieMap;
    info.loading = s_publicResumeWaitingForMap || s_continuousZombieResume;
    info.busy = info.loading || s_publicZombieCapture;
    info.canSave = match && !info.busy && CG_GetLocalClientGlobals(0)->predictedPlayerState.stats[0] > 0;
    info.paused = match && Dvar_GetInt("cl_paused") != 0;
    info.result = s_publicZombieResult; info.success = s_publicZombieSuccess; info.message = s_publicZombieMessage;
    session::PublishResumeInfo(info);
}
