// Port diagnostic logical state, not the retail save layout. Included by
// destructible.cpp so restored piece arrays use their real hunk owner.
#include <universal/memfile.h>

static int Destructible_ReadSaveInt(MemoryFile *file)
{
    int value = 0;
    MemFile_ReadData(file, 4, (unsigned char *)&value);
    return value;
}

void Destructible_WriteSaveState(MemoryFile *file)
{
    MemFile_WriteInt(file, 0x31545344); // DST1
    if (g_numDisabledLables < 0 || g_numDisabledLables > ARRAY_COUNT(g_disabledLabels))
        Com_Error(ERR_DROP, "Save destructibles: invalid disabled label count");
    MemFile_WriteInt(file, g_numDisabledLables);
    for (int i = 0; i < g_numDisabledLables; ++i)
        MemFile_WriteCString(file, SL_ConvertToString(g_disabledLabels[i], SCRIPTINSTANCE_SERVER));
    unsigned int count = 0;
    int pieces = 0;
    for (int i = 0; i < ARRAY_COUNT(s_destructibles); ++i)
    {
        const Destructible &d = s_destructibles[i];
        if (d.entNum == ENTITYNUM_NONE)
            continue;
        if (d.entNum < 0 || d.entNum >= ENTITYNUM_NONE || !g_entities[d.entNum].r.inuse
            || g_entities[d.entNum].destructible != &d || !d.ddef || !d.ddef->name
            || !d.pieceArray || d.pieceCount < 1 || d.pieceCount > 32
            || d.pieceCount != d.ddef->numPieces || !(d.flags & 2) || d.destructiblePoseID)
            Com_Error(ERR_DROP, "Save destructibles: invalid owner/pieces in slot %i", i);
        MemFile_WriteInt(file, i);
        MemFile_WriteInt(file, d.entNum);
        MemFile_WriteCString(file, (char *)d.ddef->name);
        MemFile_WriteInt(file, d.pieceCount);
        MemFile_WriteInt(file, d.oldestBurnTime);
        MemFile_WriteData(file, sizeof(d.flags), (unsigned char *)&d.flags);
        static_assert(sizeof(DestructibleState) == 28, "Destructible state wire image");
        MemFile_WriteData(file, sizeof(d.states), (unsigned char *)d.states);
        MemFile_WriteInt(file, d.bHasBeenHit);
        for (int j = 0; j < d.pieceCount; ++j)
        {
            const DESTRUCTIBLE_PIECE_INFO &piece = d.pieceArray[j];
            if (piece.xdollHandle || piece.fx || piece.burnData.fx || piece.burnData.sndId)
                Com_Error(ERR_DROP, "Save destructibles: active native debris/effect owner is not supported");
            MemFile_WriteData(file, sizeof(piece.health), (unsigned char *)&piece.health);
            MemFile_WriteInt(file, piece.burnData.burnTime);
        }
        ++count;
        pieces += d.pieceCount;
    }
    if (count != s_numDestructibles)
        Com_Error(ERR_DROP, "Save destructibles: live counter mismatch");
    MemFile_WriteInt(file, -1);
    Com_Printf(15, "Save destructibles: wrote %u owners, %i pieces, %i disabled labels\n", count, pieces, g_numDisabledLables);
}

void Destructible_ReadSaveState(MemoryFile *file)
{
    if (Destructible_ReadSaveInt(file) != 0x31545344)
        Com_Error(ERR_DROP, "Save destructibles: unsupported section");
    if (s_numDestructibles)
        Com_Error(ERR_DROP, "Save destructibles: old world owners still installed");
    for (const Destructible &d : s_destructibles)
        iassert(d.entNum == ENTITYNUM_NONE);
    const int labels = Destructible_ReadSaveInt(file);
    if (labels < 0 || labels > ARRAY_COUNT(g_disabledLabels))
        Com_Error(ERR_DROP, "Save destructibles: invalid disabled label count");
    g_numDisabledLables = 0;
    for (int i = 0; i < labels; ++i)
    {
        // The original table borrows script/asset string identities. Do not
        // manufacture an unowned string: retained scripts and loaded VM/assets
        // must already supply its lifetime, or this capture is incompatible.
        const char *name = MemFile_ReadCString(file);
        const unsigned int label = SL_FindString(name, SCRIPTINSTANCE_SERVER);
        if (!label)
            Com_Error(ERR_DROP, "Save destructibles: missing borrowed label '%s'", name);
        for (int j = 0; j < i; ++j)
            if (g_disabledLabels[j] == label)
                Com_Error(ERR_DROP, "Save destructibles: duplicate disabled label");
        g_disabledLabels[g_numDisabledLables++] = label;
    }
    bool seen[ARRAY_COUNT(s_destructibles)] = {};
    int pieces = 0;
    for (;;)
    {
        const int index = Destructible_ReadSaveInt(file);
        if (index == -1)
            break;
        if (index < 0 || index >= ARRAY_COUNT(s_destructibles) || seen[index])
            Com_Error(ERR_DROP, "Save destructibles: invalid/duplicate slot %i", index);
        seen[index] = true;
        const int entity = Destructible_ReadSaveInt(file);
        Destructible *d = &s_destructibles[index];
        if (entity < 0 || entity >= ENTITYNUM_NONE || !g_entities[entity].r.inuse
            || g_entities[entity].destructible != d)
            Com_Error(ERR_DROP, "Save destructibles: missing entity back-reference");
        const char *name = MemFile_ReadCString(file);
        DestructibleDef *def = DestructibleDefPrecache(name);
        if (!def || strcmp(def->name, name))
            Com_Error(ERR_DROP, "Save destructibles: definition mismatch");
        const int count = Destructible_ReadSaveInt(file);
        if (count < 1 || count > 32 || count != def->numPieces)
            Com_Error(ERR_DROP, "Save destructibles: piece count mismatch");
        // Publish only with an allocated piece owner so ordinary entity cleanup
        // can unwind a later diagnostic failure.
        if (!g_pieceArrayHunk)
            g_pieceArrayHunk = Hunk_UserCreateFromBuffer(g_pieceArrayBuffer, sizeof(g_pieceArrayBuffer),
                HU_SCHEME_FIRSTFIT, 0, 0, "DesctructiblePieces", 0);
        DESTRUCTIBLE_PIECE_INFO *array = (DESTRUCTIBLE_PIECE_INFO *)Hunk_UserAlloc(
            g_pieceArrayHunk, count * sizeof(DESTRUCTIBLE_PIECE_INFO), 4, 0);
        if (!array)
            Com_Error(ERR_DROP, "Save destructibles: piece allocation failed");
        memset(array, 0, count * sizeof(*array));
        *d = {};
        d->entNum = entity;
        d->ddef = def;
        d->pieceArray = array;
        d->pieceCount = count;
        ++s_numDestructibles;
        d->oldestBurnTime = Destructible_ReadSaveInt(file);
        MemFile_ReadData(file, sizeof(d->flags), (unsigned char *)&d->flags);
        if (!(d->flags & 2))
            Com_Error(ERR_DROP, "Save destructibles: uninitialized piece state");
        MemFile_ReadData(file, sizeof(d->states), (unsigned char *)d->states);
        const int hit = Destructible_ReadSaveInt(file);
        if (hit < 0 || hit > 1)
            Com_Error(ERR_DROP, "Save destructibles: invalid hit state");
        d->bHasBeenHit = hit;
        for (int i = 0; i < count; ++i)
        {
            MemFile_ReadData(file, sizeof(array[i].health), (unsigned char *)&array[i].health);
            array[i].burnData.burnTime = Destructible_ReadSaveInt(file);
        }
        pieces += count;
    }
    for (int i = 0; i < MAX_GENTITIES; ++i)
        if (g_entities[i].r.inuse && g_entities[i].destructible)
        {
            const Destructible *d = g_entities[i].destructible;
            iassert(d->entNum == i && d->pieceArray);
        }
    Com_Printf(15, "loadgame destructibles: installed %u owners, %i pieces, %i disabled labels\n", s_numDestructibles, pieces, labels);
}
