// HUF1: HUD deadlines belong to the saved cgame clock, not map loading.
#include <ui/ui_shared.h>
#include <ui/ui_utils.h>
#include <client_mp/cl_scrn_mp.h>
#define SP_HUF_FIELDS(X) \
    X(compassFadeTime) X(healthFadeTime) X(ammoFadeTime) X(stanceFadeTime) \
    X(sprintFadeTime) X(offhandFadeTime) X(offhandFlashTime) X(waypointFadeTime) \
    X(vehicleControlsFadeTime) X(scoreFadeTime)
static const char *const s_hudGroupNames[] = {
    "Health", "Health_mp", "weaponinfozombies", "weaponinfo", "weaponinfo_lowdef", "weaponinfo_mp",
    "Compass", "Compass_mp", "stance", "stance_mp", "sprintMeter", "sprintMeter_mp",
    "offhandinfo", "offhandinfo_mp", "objectiveinfo", "RCBombHints", "heatinfo"
};
struct SPClientHudState
{
#define HUF_FIELD(n) decltype(cg_s::n) n{};
    SP_HUF_FIELDS(HUF_FIELD)
#undef HUF_FIELD
    struct Group { bool present = false, visible = false; } groups[ARRAY_COUNT(s_hudGroupNames)];
};
static SPClientHudState s_clientHud;
static bool s_clientHudPresent;
static void SP_ClientHudCodec(SPNativeCheckpointCodec &c, SPClientHudState &v)
{
    if (c.Int(0x31465548) != 0x31465548) Com_Error(ERR_DROP, "Client HUD checkpoint schema invalid");
#define HUF_FIELD(n) static_assert(sizeof(v.n) == 4); c.Field(v.n);
    SP_HUF_FIELDS(HUF_FIELD)
#undef HUF_FIELD
    const int count = c.Int(ARRAY_COUNT(s_hudGroupNames));
    if (count != ARRAY_COUNT(s_hudGroupNames)) Com_Error(ERR_DROP, "Client HUD checkpoint group inventory invalid");
    for (unsigned i = 0; i < ARRAY_COUNT(s_hudGroupNames); ++i)
    {
        if (strcmp(c.Name(s_hudGroupNames[i]), s_hudGroupNames[i])) Com_Error(ERR_DROP, "Client HUD checkpoint group identity invalid");
        c.Field(v.groups[i].present); c.Field(v.groups[i].visible);
        if (!v.groups[i].present && v.groups[i].visible) Com_Error(ERR_DROP, "Client HUD checkpoint absent group is visible");
    }
}
static void SP_ReadClientHud(SPClientHudState &v)
{
    const auto &cg = *CG_GetLocalClientGlobals(0); const auto &dc = cgDC[0];
    if (dc.contextIndex != 0) Com_Error(ERR_DROP, "Client HUD checkpoint context mismatch");
#define HUF_FIELD(n) v.n = cg.n;
    SP_HUF_FIELDS(HUF_FIELD)
#undef HUF_FIELD
    for (unsigned i = 0; i < ARRAY_COUNT(s_hudGroupNames); ++i)
    {
        const auto *menu = Menus_FindByName(&dc, s_hudGroupNames[i]);
        v.groups[i].present = menu != nullptr;
        v.groups[i].visible = menu && (Window_GetDynamicFlags(dc.contextIndex, &menu->window) & 4) != 0;
    }
}
static void SP_TraceClientHud(const char *phase, const SPClientHudState &v)
{
    int present = 0, visible = 0;
    for (const auto &g : v.groups) { present += g.present; visible += g.visible; }
    Com_Printf(15, "Client HUD state %s: compass %i health %i ammo %i stance %i sprint %i offhand %i flash %i waypoint %i vehicle %i objective %i groups %i visible %i\n",
        phase, v.compassFadeTime, v.healthFadeTime, v.ammoFadeTime, v.stanceFadeTime, v.sprintFadeTime,
        v.offhandFadeTime, v.offhandFlashTime, v.waypointFadeTime, v.vehicleControlsFadeTime, v.scoreFadeTime, present, visible);
}
static void SP_WriteClientHud(MemoryFile *file)
{
    SP_ReadClientHud(s_clientHud); SPNativeCheckpointCodec c{nullptr, file}; SP_ClientHudCodec(c, s_clientHud);
    SP_TraceClientHud("captured", s_clientHud);
    Com_Printf(15, "Client HUD checkpoint: captured ammo %i health %i compass %i objective %i; ten deadlines and %u group identities\n",
        s_clientHud.ammoFadeTime, s_clientHud.healthFadeTime, s_clientHud.compassFadeTime, s_clientHud.scoreFadeTime, (unsigned)ARRAY_COUNT(s_hudGroupNames));
}
static void SP_PreflightClientHud()
{
    if (!s_clientHudPresent) return;
    if (cgDC[0].contextIndex != 0) Com_Error(ERR_DROP, "Client HUD checkpoint publication context mismatch");
    for (unsigned i = 0; i < ARRAY_COUNT(s_hudGroupNames); ++i)
        if ((Menus_FindByName(&cgDC[0], s_hudGroupNames[i]) != nullptr) != s_clientHud.groups[i].present)
            Com_Error(ERR_DROP, "Client HUD checkpoint menu definition inventory differs");
}
static void SP_DecodeClientHud(SPNativeCheckpointCodec &c)
{
    s_clientHudPresent = false; s_clientHud = {}; SP_ClientHudCodec(c, s_clientHud);
    s_clientHudPresent = true; SP_PreflightClientHud();
}
static void SP_PublishClientHud()
{
    if (!s_clientHudPresent)
    {
        // Experimental CLP1..13 never stored these deadlines/visibility bits.
        // Do not invent historical values or claim exact legacy HUD recovery.
        Com_Printf(15, "Client HUD checkpoint: LEGACY has no durable HUD deadlines/group visibility; exact HUD recovery unavailable\n");
        return;
    }
    SP_PreflightClientHud();
    auto &cg = *CG_GetLocalClientGlobals(0);
#define HUF_FIELD(n) cg.n = s_clientHud.n;
    SP_HUF_FIELDS(HUF_FIELD)
#undef HUF_FIELD
    for (unsigned i = 0; i < ARRAY_COUNT(s_hudGroupNames); ++i) if (s_clientHud.groups[i].present)
    {
        if (s_clientHud.groups[i].visible) Menus_ShowByName(&cgDC[0], s_hudGroupNames[i]);
        else Menus_HideByName(&cgDC[0], s_hudGroupNames[i]);
    }
    SPClientHudState actual; SP_ReadClientHud(actual);
    SP_TraceClientHud("published", actual);
#define HUF_FIELD(n) if (actual.n != s_clientHud.n) Com_Error(ERR_DROP, "Client HUD checkpoint deadline readback differs");
    SP_HUF_FIELDS(HUF_FIELD)
#undef HUF_FIELD
    for (unsigned i = 0; i < ARRAY_COUNT(s_hudGroupNames); ++i)
        if (actual.groups[i].present != s_clientHud.groups[i].present || actual.groups[i].visible != s_clientHud.groups[i].visible)
            Com_Error(ERR_DROP, "Client HUD checkpoint group visibility readback differs");
    Com_Printf(15, "Client HUD checkpoint: ten deadlines / %u HUD group identities LIVE readback IDENTICAL at clock %i; ammo %i health %i compass %i objective %i\n",
        (unsigned)ARRAY_COUNT(s_hudGroupNames), cg.physicsTime, cg.ammoFadeTime, cg.healthFadeTime, cg.compassFadeTime, cg.scoreFadeTime);
}
#undef SP_HUF_FIELDS
