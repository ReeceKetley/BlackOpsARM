// CWS1: logical weapon selection for both cgame and the command producer.
// PlayerState alone does not own the pending selection or alternate-mode history.
#include <cgame/cg_weapons.h>
#define SP_CWS_CG_FIELDS(X) \
    X(weaponSelect) X(weaponSelectTime) X(weaponForcedSelect) \
    X(weaponLatestPrimaryIdx) X(lastWeaponForAlt) X(equippedOffHand)
#define SP_CWS_CL_FIELDS(X) \
    X(cgameUserCmdWeapon) X(cgameUserCmdOffHandIndex) X(cgameUserCmdLastWeaponForAlt)
struct SPClientWeaponSelection
{
#define CWS_FIELD(n) decltype(cg_s::n) n{};
    SP_CWS_CG_FIELDS(CWS_FIELD)
#undef CWS_FIELD
    struct Command
    {
#define CWS_FIELD(n) decltype(clientActive_t::n) n{};
        SP_CWS_CL_FIELDS(CWS_FIELD)
#undef CWS_FIELD
    } command;
};
static SPClientWeaponSelection s_clientWeaponSelection;
static bool s_clientWeaponSelectionPresent;
static void SP_ClientWeaponSelectionCodec(SPNativeCheckpointCodec &c, SPClientWeaponSelection &v)
{
    if (c.Int(0x31535743) != 0x31535743) Com_Error(ERR_DROP, "Client weapon selection schema invalid");
#define CWS_FIELD(n) SP_CNSValue(c, v.n);
    SP_CWS_CG_FIELDS(CWS_FIELD)
#undef CWS_FIELD
#define CWS_FIELD(n) SP_CNSValue(c, v.command.n);
    SP_CWS_CL_FIELDS(CWS_FIELD)
#undef CWS_FIELD
    // CPM already binds every ordinal to its exact weapon name before this
    // record is read. Zero is the canonical no-weapon/offhand selection.
    const auto valid = [](unsigned index) { return index < s_clientMovement.weapons.size(); };
    if (!valid(v.weaponSelect) || !valid(v.weaponLatestPrimaryIdx) || !valid(v.equippedOffHand)
        || !valid(v.command.cgameUserCmdWeapon) || !valid(v.command.cgameUserCmdOffHandIndex)
        || !valid(v.command.cgameUserCmdLastWeaponForAlt))
        Com_Error(ERR_DROP, "Client weapon selection rejected weapon identity");
    for (const auto weapon : v.lastWeaponForAlt)
        if (!valid(weapon)) Com_Error(ERR_DROP, "Client weapon selection rejected alternate history");
}
static void SP_ReadClientWeaponSelection(SPClientWeaponSelection &v)
{
    const auto &cg = *CG_GetLocalClientGlobals(0);
    const auto &cl = *CL_GetLocalClientGlobals(0);
#define CWS_FIELD(n) memcpy(&v.n, &cg.n, sizeof(v.n));
    SP_CWS_CG_FIELDS(CWS_FIELD)
#undef CWS_FIELD
#define CWS_FIELD(n) memcpy(&v.command.n, &cl.n, sizeof(v.command.n));
    SP_CWS_CL_FIELDS(CWS_FIELD)
#undef CWS_FIELD
}
static void SP_WriteClientWeaponSelection(MemoryFile *file)
{
    SP_ReadClientWeaponSelection(s_clientWeaponSelection);
    SPNativeCheckpointCodec c{nullptr, file}; SP_ClientWeaponSelectionCodec(c, s_clientWeaponSelection);
    Com_Printf(15, "Client weapon selection: captured selected %u command %i offhand %i/%i forced %i; alternate history retained\n",
        s_clientWeaponSelection.weaponSelect, s_clientWeaponSelection.command.cgameUserCmdWeapon,
        s_clientWeaponSelection.equippedOffHand, s_clientWeaponSelection.command.cgameUserCmdOffHandIndex,
        s_clientWeaponSelection.weaponForcedSelect);
}
static void SP_DecodeClientWeaponSelection(SPNativeCheckpointCodec &c)
{
    s_clientWeaponSelection = {}; SP_ClientWeaponSelectionCodec(c, s_clientWeaponSelection);
    s_clientWeaponSelectionPresent = true;
}
static void SP_PublishClientWeaponSelection()
{
    auto &cg = *CG_GetLocalClientGlobals(0);
    auto &cl = *CL_GetLocalClientGlobals(0);
    if (!s_clientWeaponSelectionPresent)
    {
        // Older saves did not record selection controls. Seed only the known
        // current weapon/offhand from their saved player, never fresh-map input.
        cg.weaponSelect = cg.predictedPlayerState.weapon;
        cg.equippedOffHand = cg.predictedPlayerState.offHandIndex;
        CL_SetUserCmdWeapons(0, cg.weaponSelect, cg.equippedOffHand,
            CG_GetLastWeaponForAlt(&cg, &cg.predictedPlayerState, cg.weaponSelect));
        Com_Printf(15, "Client weapon selection: LEGACY current weapon %u/offhand %i seeded; pending selection/history unavailable\n",
            cg.weaponSelect, cg.equippedOffHand);
        return;
    }
#define CWS_FIELD(n) memcpy(&cg.n, &s_clientWeaponSelection.n, sizeof(cg.n));
    SP_CWS_CG_FIELDS(CWS_FIELD)
#undef CWS_FIELD
#define CWS_FIELD(n) memcpy(&cl.n, &s_clientWeaponSelection.command.n, sizeof(cl.n));
    SP_CWS_CL_FIELDS(CWS_FIELD)
#undef CWS_FIELD
    static unsigned char expectedBytes[1024], actualBytes[1024];
    static MemoryFile expected, actual;
    static SPClientWeaponSelection check;
    SP_ReadClientWeaponSelection(check);
    MemFile_InitForWriting(&expected, sizeof(expectedBytes), expectedBytes, true, true);
    MemFile_InitForWriting(&actual, sizeof(actualBytes), actualBytes, true, true);
    SPNativeCheckpointCodec a{nullptr, &expected}, b{nullptr, &actual};
    SP_ClientWeaponSelectionCodec(a, s_clientWeaponSelection); SP_ClientWeaponSelectionCodec(b, check);
    MemFile_StartSegment(&expected, -1); MemFile_StartSegment(&actual, -1);
    if (expected.bytesUsed != actual.bytesUsed || memcmp(expectedBytes, actualBytes, expected.bytesUsed))
        Com_Error(ERR_DROP, "Client weapon selection live publication differs");
    Com_Printf(15, "Client weapon selection: selected %u command %i offhand %i/%i forced %i LIVE RESERIALIZATION IDENTICAL before input frames\n",
        cg.weaponSelect, cl.cgameUserCmdWeapon, cg.equippedOffHand, cl.cgameUserCmdOffHandIndex, cg.weaponForcedSelect);
}
#undef SP_CWS_CG_FIELDS
#undef SP_CWS_CL_FIELDS
