// CPM1: replay real pending commands through canonical client Pmove. No frames.
#include <bgame/bg_pmove.h>
#include <bgame/bg_slidemove.h>
#include <bgame/bg_mantle.h>
#include <bgame/bg_dog_animations_mp.h>
#include <bgame/bg_weapons_def.h>
#include <platform/android/engine_random.h>
extern gjkcc_info_database_t g_gjkcc_info_client_database;

#define SP_CPM_FIELDS(X) \
    X(tracemask) X(numtouch) X(touchents) X(numGlassTouch) X(touchGlasses) \
    X(mins) X(maxs) X(xyspeed) X(currentPitch) X(averagePitch) X(nextPitch) X(pitchHistory) \
    X(proneChange) X(maxSprintTimeMultiplier) X(mantleStarted) X(mantleEndPos) X(mantleDuration) \
    X(viewChangeTime) X(viewChange) X(vehicleAngles) X(vehAnimState) X(spDisabledStances)
struct SPClientMovementScalars
{
#define CPM_FIELD(n) decltype(pmove_t::n) n{};
    SP_CPM_FIELDS(CPM_FIELD)
#undef CPM_FIELD
};
struct SPClientMovement
{
    SPClientMovementScalars scalars;
    SPClientPlayerImage base, result;
    std::vector<std::string> weapons;
    unsigned seed = 0, finalSeed = 0;
    int steps = 0;
};
static SPClientMovement s_clientMovement;
static bool s_clientMovementPresent;
static void SP_CPMScalarsCodec(SPNativeCheckpointCodec &c, SPClientMovementScalars &v)
{
#define CPM_FIELD(n) SP_CNSValue(c, v.n);
    SP_CPM_FIELDS(CPM_FIELD)
#undef CPM_FIELD
    if (v.numtouch < 0 || v.numtouch > 32 || v.numGlassTouch < 0 || v.numGlassTouch > 32
        || (v.spDisabledStances & ~0x1C00000u) || v.vehAnimState < 0 || v.vehAnimState > 2)
        Com_Error(ERR_DROP, "Client movement scalar range invalid");
}
static void SP_ClientMovementCodec(SPNativeCheckpointCodec &c, SPClientMovement &v)
{
    if (c.Int(0x314D5043) != 0x314D5043) Com_Error(ERR_DROP, "Client movement schema invalid");
    SP_CPMScalarsCodec(c, v.scalars); SP_CNSPlayerCodec(c, v.base); SP_CNSPlayerCodec(c, v.result);
    c.Field(v.seed); c.Field(v.finalSeed); c.Field(v.steps);
    int count = c.Int(v.weapons.size());
    if (count < 1 || count > 2048 || v.steps < 0 || v.steps > 128
        || v.base.value.clientNum != v.result.value.clientNum
        || v.result.value.commandTime < v.base.value.commandTime
        || (v.steps != 0 && v.result.value.commandTime == v.base.value.commandTime)
        || (v.steps == 0 && v.result.value.commandTime != v.base.value.commandTime)
        || static_cast<long long>(v.result.value.commandTime) - v.base.value.commandTime > 1000)
        Com_Error(ERR_DROP, "Client movement replay range invalid");
    if (c.reader) v.weapons.resize(count);
    for (auto &name : v.weapons) { const char *s = c.Name(name.c_str()); if (c.reader) name = s; }
}
static bool SP_CPMPlayerSupported(const playerState_s &ps)
{
    // ClientEndFrame uses 6 for last stand and 7 while being revived in
    // Zombies. Both run the original Pmove path with the same trace, weapon,
    // animation and GJK services as normal movement; retain the saved mode.
    return (ps.pm_type == 0 || ps.pm_type == 6 || ps.pm_type == 7)
        && ps.clientNum == 0 && !(ps.eFlags & 0x4000);
}
static void SP_CPMCheckWeapons()
{
    if (BG_GetNumWeapons() != s_clientMovement.weapons.size()) Com_Error(ERR_DROP, "Client movement weapon table count differs");
    for (unsigned i = 0; i < s_clientMovement.weapons.size(); ++i)
        if (s_clientMovement.weapons[i] != BG_GetWeaponVariantDef(i)->szInternalName)
            Com_Error(ERR_DROP, "Client movement weapon table identity differs at %u", i);
}
static pmove_t s_clientMovementPmove; // Real constructor retains visitor vtable/storage.
static bgs_t *s_previousMovementBgs;
static clientInfo_t s_previousMovementClients[36];
static int s_previousMovementProne[32], s_previousMovementGjkCount;
static unsigned s_previousMovementSeed;
static std::vector<float> s_previousMovementAnimRates;
static bool s_clientMovementActive, s_clientMovementServices;
static void SP_AbortClientMovement()
{
    if (!s_clientMovementActive) return;
    s_clientMovementActive = false;
    // Recoverable errors unwind through the engine's main-thread abort hook.
    // Complete the same canonical GJK epilog if Pmove did not return normally.
    if (s_clientMovementPmove.m_gjkcc_input)
    {
        gjkcc_input_t input{}; setup_gjkcc_input(&s_clientMovementPmove, &input);
        gjkcc_epilog(&input, s_clientMovementPmove.ps->origin);
        s_clientMovementPmove.m_gjkcc_input = nullptr;
    }
    g_gjkcc_info_client_database.gjkcc_info_destroy(reinterpret_cast<uintptr_t>(&s_clientMovementPmove), false);
    s_clientMovementPmove.proximity_data.reset(); s_clientMovementPmove.ps = nullptr;
    for (int i = 0; i < 36; ++i) *SP_CRCClientSlot(i) = s_previousMovementClients[i];
    memcpy(CG_GetLocalClientGlobals(0)->bgs.proneTime, s_previousMovementProne, sizeof(s_previousMovementProne));
    auto *data = CG_GetLocalClientGlobals(0)->bgs.animData;
    for (int i = 0; i < s_previousMovementAnimRates.size(); ++i)
        data->animScriptData.animations[i].forceAnimRate = s_previousMovementAnimRates[i];
    s_previousMovementAnimRates.clear();
    bgs = s_previousMovementBgs; bo1::android::CrtSrand(s_previousMovementSeed);
    if (s_clientMovementServices) { s_clientMovementServices = false; Mantle_ShutdownAnims(); Dog_ShutdownAnims(); }
}
static bool SP_CPMReadPendingCommand(int number, int latestTime, int playerTime, usercmd_s &cmd, usercmd_s &previous)
{
    if (!CL_GetUserCmd(0, number, &cmd)) Com_Error(ERR_DROP, "Client movement pending command missing");
    if (cmd.serverTime <= playerTime || cmd.serverTime > latestTime) return false;
    // Match CG_PredictPlayerState: the oldest retained command cannot run
    // without its predecessor. A long pause can overwrite that predecessor
    // with duplicate-time commands; the next real retained pair is usable.
    if (!CL_GetUserCmd(0, number - 1, &previous)) return false;
    if (cmd.serverTime - static_cast<long long>(playerTime) > 1000)
        Com_Error(ERR_DROP, "Client movement pending command time gap invalid");
    return true;
}
static void SP_CPMReplay(bool fresh, SPClientPlayerImage &result, unsigned &finalSeed, int &steps)
{
    if (s_clientMovementActive) Com_Error(ERR_DROP, "Client movement replay already active");
    auto *cg = CG_GetLocalClientGlobals(0); auto &pm = s_clientMovementPmove;
    SP_CPMCheckWeapons();
    if (!SP_CPMPlayerSupported(s_clientMovement.base.value))
        Com_Error(ERR_DROP, "Client movement checkpoint requires normal, last-stand or revive local movement");
    s_previousMovementBgs = bgs; s_previousMovementSeed = bo1::android::CrtGetState();
    s_previousMovementGjkCount = g_gjkcc_info_client_database.m_gcci_count;
    for (int i = 0; i < 36; ++i) s_previousMovementClients[i] = *SP_CRCClientSlot(i);
    memcpy(s_previousMovementProne, cg->bgs.proneTime, sizeof(s_previousMovementProne));
    s_previousMovementAnimRates.resize(cg->bgs.animData->animScriptData.numAnimations);
    for (int i = 0; i < s_previousMovementAnimRates.size(); ++i)
        s_previousMovementAnimRates[i] = cg->bgs.animData->animScriptData.animations[i].forceAnimRate;
    s_clientMovementActive = true;
    if (fresh)
    {
        s_clientMovementServices = true;
        Mantle_CreateAnims(Hunk_AllocXAnimClient); Dog_CreateAnims(Hunk_AllocXAnimClient);
    }
    bgs = &cg->bgs; bo1::android::CrtSrand(s_clientMovement.seed);
    result = s_clientMovement.base; SP_CNSBindPlayer(result);
#define CPM_FIELD(n) pm.n = s_clientMovement.scalars.n;
    // Arrays need native element copies; the codec above remains fieldwise.
#undef CPM_FIELD
    const auto copy = [](auto &to, const auto &from) { memcpy(&to, &from, sizeof(to)); };
#define CPM_FIELD(n) copy(pm.n, s_clientMovement.scalars.n);
    SP_CPM_FIELDS(CPM_FIELD)
#undef CPM_FIELD
    pm.ps = &result.value; pm.handler = 0; pm.localClientNum = 0; pm.m_gjkcc_input = nullptr;
    pm.proximity_data.reset(); steps = 0;
    const int current = CL_GetCurrentCmdNumber(0);
    usercmd_s latest{}; if (!CL_GetUserCmd(0, current, &latest)) Com_Error(ERR_DROP, "Client movement latest command missing");
    for (int n = std::max(1, current - 127); n <= current; ++n)
    {
        usercmd_s cmd{};
        if (!SP_CPMReadPendingCommand(n, latest.serverTime, pm.ps->commandTime, cmd, pm.oldcmd)) continue;
        pm.cmd = cmd;
        Pmove(&pm); // Original full player movement, weapon, trace and GJK path.
        ++steps;
    }
    finalSeed = bo1::android::CrtGetState();
    SP_AbortClientMovement();
    if (g_gjkcc_info_client_database.m_gcci_count != s_previousMovementGjkCount)
        Com_Error(ERR_DROP, "Client movement GJK owner retirement differs");
}
static void SP_WriteClientMovement(MemoryFile *file)
{
    s_clientMovement = {}; auto &pm = cg_pmove[0];
    const auto copy = [](auto &to, const auto &from) { memcpy(&to, &from, sizeof(to)); };
#define CPM_FIELD(n) copy(s_clientMovement.scalars.n, pm.n);
    SP_CPM_FIELDS(CPM_FIELD)
#undef CPM_FIELD
    auto *cg = CG_GetLocalClientGlobals(0);
    if (!cg->nextSnap) Com_Error(ERR_DROP, "Client movement snapshot unavailable");
    SP_CNSReadPlayer(cg->nextSnap->ps, s_clientMovement.base);
    s_clientMovement.seed = bo1::android::CrtGetState();
    for (unsigned i = 0; i < BG_GetNumWeapons(); ++i) s_clientMovement.weapons.emplace_back(BG_GetWeaponVariantDef(i)->szInternalName);
    SP_CPMReplay(false, s_clientMovement.result, s_clientMovement.finalSeed, s_clientMovement.steps);
    SPNativeCheckpointCodec c{nullptr, file}; SP_ClientMovementCodec(c, s_clientMovement);
    Com_Printf(15, "Client movement: captured %i actual pending Pmove commands %i -> %i, origin %.6f %.6f %.6f -> %.6f %.6f %.6f; canonical GJK owner retired\n",
        s_clientMovement.steps, s_clientMovement.base.value.commandTime, s_clientMovement.result.value.commandTime,
        s_clientMovement.base.value.origin[0], s_clientMovement.base.value.origin[1], s_clientMovement.base.value.origin[2],
        s_clientMovement.result.value.origin[0], s_clientMovement.result.value.origin[1], s_clientMovement.result.value.origin[2]);
}
static void SP_DecodeClientMovement(SPNativeCheckpointCodec &c)
{
    s_clientMovement = {}; SP_ClientMovementCodec(c, s_clientMovement); SP_CPMCheckWeapons();
    // These are the same canonical snapshot and command-ring owners as CNS/CPC.
    auto &snapshot = s_clientNetworkState.snapshots[s_clientNetworkState.next].player;
    static unsigned char aBytes[0x10000], bBytes[0x10000]; static MemoryFile a, b;
    MemFile_InitForWriting(&a, sizeof(aBytes), aBytes, true, true); MemFile_InitForWriting(&b, sizeof(bBytes), bBytes, true, true);
    SPNativeCheckpointCodec ac{nullptr, &a}, bc{nullptr, &b}; SP_CNSPlayerCodec(ac, snapshot); SP_CNSPlayerCodec(bc, s_clientMovement.base);
    MemFile_StartSegment(&a, -1); MemFile_StartSegment(&b, -1);
    if (a.bytesUsed != b.bytesUsed || memcmp(aBytes, bBytes, a.bytesUsed))
        Com_Error(ERR_DROP, "Client movement full base player differs from snapshot");
    const auto &ps = s_clientMovement.base.value;
    const auto count = s_clientMovement.weapons.size();
    if (!SP_CPMPlayerSupported(ps)
        || ps.weapon >= count || static_cast<unsigned>(ps.offHandIndex) >= count)
        Com_Error(ERR_DROP, "Client movement player/weapon identity invalid");
    const auto &commands = s_clientPredictionOwners.history.commands;
    const int latest = commands.back().serverTime;
    int cursor = ps.commandTime, steps = 0;
    for (int i = 1; i < commands.size(); ++i)
    {
        const auto &cmd = commands[i];
        if (cmd.serverTime <= cursor || cmd.serverTime > latest) continue;
        if (cmd.serverTime - static_cast<long long>(cursor) > 1000
            || cmd.weapon >= count || cmd.offHandIndex >= count)
            Com_Error(ERR_DROP, "Client movement pending command/weapon identity invalid");
        cursor = cmd.serverTime; ++steps;
    }
    if (steps != s_clientMovement.steps || cursor != s_clientMovement.result.value.commandTime)
        Com_Error(ERR_DROP, "Client movement pending command result differs");
    s_clientMovementPresent = true;
    Com_Printf(15, "Client movement: saved command/weapon identities validated BEFORE teardown\n");
}
static void SP_VerifyClientMovement(bool servicesRetained = false)
{
    if (!s_clientMovementPresent) return;
    static SPClientPlayerImage actual; unsigned seed = 0; int steps = 0;
    SP_CPMReplay(!servicesRetained, actual, seed, steps);
    if (steps != s_clientMovement.steps || seed != s_clientMovement.finalSeed) Com_Error(ERR_DROP, "Client movement step/random state differs");
    static unsigned char aBytes[0x10000], bBytes[0x10000]; static MemoryFile a, b;
    MemFile_InitForWriting(&a, sizeof(aBytes), aBytes, true, true); MemFile_InitForWriting(&b, sizeof(bBytes), bBytes, true, true);
    SPNativeCheckpointCodec ac{nullptr, &a}, bc{nullptr, &b}; SP_CNSPlayerCodec(ac, s_clientMovement.result); SP_CNSPlayerCodec(bc, actual);
    MemFile_StartSegment(&a, -1); MemFile_StartSegment(&b, -1);
    if (a.bytesUsed != b.bytesUsed || memcmp(aBytes, bBytes, a.bytesUsed)) Com_Error(ERR_DROP, "Client movement original Pmove result differs");
    Com_Printf(15, "Client movement: FRESH PROCESS %i actual pending Pmove commands %i -> %i, original full player result/random stream IDENTICAL; canonical GJK owner retired; NO CLIENT FRAMES\n",
        steps, s_clientMovement.base.value.commandTime, actual.value.commandTime);
}
#undef SP_CPM_FIELDS
