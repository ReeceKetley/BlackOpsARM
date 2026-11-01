// Own saved settings across the temporary fresh CGame/reliable-command epoch.
// Values are logical text/type records; no native dvar/string pointers persist.
#include <vector>
#include <qcommon/threads.h>
struct SPPublicSavedDvar
{
    std::string name, text;
    dvarType_t type;
};
static std::vector<SPPublicSavedDvar> s_publicSavedDvars;
static bool s_publicSavedDvarsReady;

static void SP_ClearPublicSavedDvars()
{
    s_publicSavedDvarsReady = false;
    s_publicSavedDvars.clear();
}

static void SP_StagePublicSavedDvar(const char *name, const char *text)
{
    if (!s_continuousZombieResume || !I_stricmp(name, "sv_restoreDvars")) return;
    const auto *dvar = Dvar_FindVar(name);
    if (!dvar || s_publicSavedDvars.size() >= 4096)
        Com_Error(ERR_DROP, "Saved dvar checkpoint lost its registered owner or exceeded the native pool");
    for (const auto &saved : s_publicSavedDvars)
        if (!I_stricmp(saved.name.c_str(), name))
            Com_Error(ERR_DROP, "Saved dvar checkpoint contains a duplicate name");
    s_publicSavedDvars.push_back({name, text, dvar->type});
}

static void SP_RestorePublicSavedDvars(const char *boundary)
{
    if (!s_continuousZombieResume) return;
    if (!s_publicSavedDvarsReady || !Sys_IsMainThread())
        Com_Error(ERR_DROP, "Saved dvar checkpoint has no completed main-thread stage");
    // Validate all identities/domains first. Do not replace the original
    // loadedFromSaveGame backup: normal error recovery still owns that value.
    for (const auto &saved : s_publicSavedDvars)
    {
        const auto *dvar = Dvar_FindVar(saved.name.c_str());
        if (!dvar || dvar->type != saved.type)
            Com_Error(ERR_DROP, "Saved dvar checkpoint changed owner/type for %s", saved.name.c_str());
        DvarValue value{};
        Dvar_StringToValue(&value, dvar->type, dvar->domain, saved.text.c_str());
        if (!Dvar_ValueInDomain(dvar->type, value, dvar->domain))
            Com_Error(ERR_DROP, "Saved dvar checkpoint changed domain for %s", saved.name.c_str());
    }
    int changed = 0;
    for (const auto &saved : s_publicSavedDvars)
    {
        auto *dvar = Dvar_FindMalleableVar(saved.name.c_str());
        DvarValue value{};
        Dvar_StringToValue(&value, dvar->type, dvar->domain, saved.text.c_str());
        if (!Dvar_ValuesEqual(dvar->type, dvar->current, value))
        {
            Dvar_SetVariant(dvar, value, DVAR_SOURCE_INTERNAL);
            if (!Dvar_ValuesEqual(dvar->type, dvar->current, value))
                Com_Error(ERR_DROP, "Saved dvar checkpoint failed live readback for %s", saved.name.c_str());
            Com_Printf(15, "Zombies resume dvars: restored %s at %s\n", saved.name.c_str(), boundary);
            ++changed;
        }
    }
    Com_Printf(15, "Zombies resume dvars: %i saved owners verified / %i temporary overrides retired at %s\n",
        static_cast<int>(s_publicSavedDvars.size()), changed, boundary);
}
