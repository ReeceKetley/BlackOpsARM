// Private diagnostic table format. Keep fresh protocol/game-version/server-ID
// slots 0..3; all gameplay configstrings retain their saved numeric identity.
#include <server_mp/sv_init_mp.h>
#include <xanim/xmodel.h>

extern XModel *cached_models[512]; // canonical game model cache, DB-owned assets
static constexpr int SAVE_CONFIG_FIRST = 4;
static constexpr int SAVE_CONFIG_MAX_BYTES = 0x100000;

static void G_WriteSaveConfigstrings(MemoryFile *file)
{
    MemFile_WriteInt(file, 0x31475343); // CSG1
    MemFile_WriteInt(file, MAX_CONFIGSTRINGS - SAVE_CONFIG_FIRST);
    int total = 0, used = 0;
    for (int i = SAVE_CONFIG_FIRST; i < MAX_CONFIGSTRINGS; ++i)
    {
        const char *text = SL_ConvertToString(SV_GetConfigstringConst(i), SCRIPTINSTANCE_SERVER);
        const size_t length = strlen(text);
        if (length > 65535 || length > (size_t)(SAVE_CONFIG_MAX_BYTES - total))
            Com_Error(ERR_DROP, "Save configstrings: table exceeds diagnostic bounds");
        MemFile_WriteInt(file, (int)length);
        if (length)
        {
            MemFile_WriteData(file, (unsigned int)length, (unsigned char *)text);
            ++used;
        }
        total += (int)length;
    }
    Com_Printf(15, "Save configstrings: wrote %i gameplay slots, %i nonempty, %i text bytes; fresh identity excluded\n",
        MAX_CONFIGSTRINGS - SAVE_CONFIG_FIRST, used, total);
}

static int G_ReadConfigstringInt(MemoryFile *file)
{
    int value = 0;
    MemFile_ReadData(file, 4, (unsigned char *)&value);
    return value;
}

static void G_LoadSaveConfigstrings(MemoryFile *file)
{
    if (G_ReadConfigstringInt(file) != 0x31475343
        || G_ReadConfigstringInt(file) != MAX_CONFIGSTRINGS - SAVE_CONFIG_FIRST)
        Com_Error(ERR_DROP, "Save configstrings: unsupported table format");
    // Com_Error can leave via the engine's non-C++ error boundary. A bounded
    // arena avoids temporary heap owners whose destructors could be skipped.
    static char savedText[SAVE_CONFIG_MAX_BYTES + MAX_CONFIGSTRINGS];
    static int offsets[MAX_CONFIGSTRINGS];
    int cursor = 0;
    int total = 0, changed = 0;
    for (int i = SAVE_CONFIG_FIRST; i < MAX_CONFIGSTRINGS; ++i)
    {
        const int length = G_ReadConfigstringInt(file);
        if (length < 0 || length > 65535 || length > SAVE_CONFIG_MAX_BYTES - total)
            Com_Error(ERR_DROP, "Save configstrings: invalid string length");
        offsets[i] = cursor;
        if (length)
            MemFile_ReadData(file, length, (unsigned char *)savedText + cursor);
        if (memchr(savedText + cursor, 0, length))
            Com_Error(ERR_DROP, "Save configstrings: embedded string terminator");
        savedText[cursor + length] = 0;
        cursor += length + 1;
        total += length;
    }
    // Resolve the entire replacement cache before changing table ownership.
    // G_ModelIndex cannot be used: it chooses a free slot in the current table,
    // whereas saved entity/attachment/snapshot fields need the ORIGINAL slot.
    XModel *models[ARRAY_COUNT(cached_models)] = {};
    static_assert(CS_MODELS_LAST - CS_MODELS + 1 == ARRAY_COUNT(cached_models), "Model cache/table agreement");
    bool gap = false;
    int modelCount = 0;
    for (int i = 0; i < ARRAY_COUNT(models); ++i)
    {
        char *name = savedText + offsets[CS_MODELS + i];
        if (!i && name[0])
            Com_Error(ERR_DROP, "Save configstrings: model zero must be empty");
        if (!i)
            continue;
        if (!name[0])
        {
            gap = true;
            continue;
        }
        if (gap)
            Com_Error(ERR_DROP, "Save configstrings: model table contains a hole");
        models[i] = SV_XModelGet(name);
        // CS_MODELS is lowercased by the canonical configstring setter, while
        // the DB-owned asset retains its precache spelling. Match the same
        // case-insensitive identity as DB_FindXAssetEntry and G_ModelIndex.
        if (!models[i] || !models[i]->name || I_stricmp(models[i]->name, name))
            Com_Error(ERR_DROP, "Save configstrings: missing/mismatched model '%s'", name);
        ++modelCount;
    }
    iassert(level.initializing);
    for (int i = SAVE_CONFIG_FIRST; i < MAX_CONFIGSTRINGS; ++i)
    {
        char *text = savedText + offsets[i];
        if (strcmp(text, SL_ConvertToString(SV_GetConfigstringConst(i), SCRIPTINSTANCE_SERVER)))
        {
            SV_RestoreConfigstring(i, (char *)text);
            ++changed;
        }
    }
    memcpy(cached_models, models, sizeof(models));
    for (int i = 1; i <= modelCount; ++i)
        iassert(G_GetModel(i) == models[i]);
    Com_Printf(15, "loadgame configstrings: restored %i gameplay slots, %i changed, %i model slots resolved at saved indices; fresh identity retained\n",
        MAX_CONFIGSTRINGS - SAVE_CONFIG_FIRST, changed, modelCount);
}
