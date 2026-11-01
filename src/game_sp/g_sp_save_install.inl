// Included after the segment-3 codecs. Diagnostic live-owner installation:
// records are decoded into their final addresses, never memcpy-moved after
// registering EntHandle/SentientHandle fields. Ordinary play stays disabled;
// a separate bounded headless probe may test frames after all install checks.

static bool s_saveWantedLinks[MAX_GENTITIES];

static void G_InstallSaveEntities(MemoryFile *file)
{
    int count, index, used = 0;
    MemFile_ReadData(file, 4, (unsigned char *)&count);
    if (count < Com_GetFirstFreeEntityNum() || count > ENTITYNUM_WORLD)
        Com_Error(ERR_DROP, "Live save install: invalid entity count %i", count);
    for (int i = 0; i < MAX_GENTITIES; ++i)
        iassert(!g_entities[i].r.inuse);
    bool seen[MAX_GENTITIES] = {};
    memset(s_saveWantedLinks, 0, sizeof(s_saveWantedLinks));
    level.num_entities = count;
    level.firstFreeEnt = level.lastFreeEnt = nullptr;
    for (;;)
    {
        MemFile_ReadData(file, 4, (unsigned char *)&index);
        if (index == -1)
            break;
        if (index < 0 || index >= ENTITYNUM_NONE || seen[index]
            || (index >= count && index != ENTITYNUM_WORLD))
            Com_Error(ERR_DROP, "Live save install: invalid/duplicate entity %i", index);
        seen[index] = true;
        gentity_s *ent = &g_entities[index];
        scripted_anim_sp_t scripted;
        int sequence;
        const bool hasScripted = G_ReadEntity(ent, &g_entSpExt[index], &scripted, &sequence, file);
        iassert(ent->s.number == index && ent->r.inuse);
        // Old collision links have been removed by G_FreeEntities. A copied
        // linked bit cannot create a new world-sector registration.
        s_saveWantedLinks[index] = ent->r.linked != 0;
        ent->r.linked = false;
        ent->s.eventSequence = 0;
        ent->nextFree = nullptr;
        if (hasScripted)
            *G_SP_AllocScriptedAnim(ent) = scripted;
        G_SP_RestoreAnimCommandSequence(index, sequence);
        ++used;
    }
    for (int i = 0; i < level.maxclients; ++i)
        if (!g_entities[i].r.inuse)
            g_entities[i].client = &level.clients[i];
    // These are real free-list owners; unused slots have no saved live record.
    int freeCount = 0;
    for (int i = Com_GetFirstFreeEntityNum(); i < count; ++i)
        if (!g_entities[i].r.inuse && EntNum_IsNetworked(i))
        {
            gentity_s *ent = &g_entities[i];
            ent->nextFree = nullptr;
            if (level.lastFreeEnt)
                level.lastFreeEnt->nextFree = ent;
            else
                level.firstFreeEnt = ent;
            level.lastFreeEnt = ent;
            ++freeCount;
        }
    SV_LocateGameData(level.gentities, level.num_entities, sizeof(gentity_s), &level.clients->ps, sizeof(gclient_s));
    Com_Printf(15, "loadgame live: installed %i entities, %i free slots; collision not linked yet\n", used, freeCount);
}

static void G_InstallSaveClients(MemoryFile *file)
{
    bool seen[ARRAY_COUNT(g_clients)] = {};
    int index, count = 0;
    for (;;)
    {
        MemFile_ReadData(file, 4, (unsigned char *)&index);
        if (index == -1)
            break;
        if (index < 0 || index >= level.maxclients || index >= ARRAY_COUNT(g_clients) || seen[index])
            Com_Error(ERR_DROP, "Live save install: invalid/duplicate client %i", index);
        seen[index] = true;
        SPClientSaveExt ext;
        gclient_s *client = &level.clients[index];
        G_ReadClient(client, &ext, file, nullptr);
        iassert(client->sess.connected == CON_DISCONNECTED || client->ps.clientNum == index);
        gclientSpExt &live = g_clientSpExt[index];
        iassert(!live.lookatent.number);
        live.sessionPerks = ext.ext.sessionPerks;
        live.downs = ext.ext.downs;
        live.revives = ext.ext.revives;
        // The temporary aggregate's handle registration cannot be copied. Take
        // a new registration at its permanent side-table address, then release
        // the temporary reader's registration through the codec's release path.
        if (ext.ext.lookatent.number)
            live.lookatent.setEnt((gentity_s *)SP_EntityReference(3, ext.ext.lookatent.number));
        SP_ReleaseEntityFields(s_clientExtFields, ARRAY_COUNT(s_clientExtFields), &ext);
        G_SP_PlayerX1State(index) = ext.x1;
        G_SP_PlayerBuiltinState(index) = ext.builtin;
        Com_Printf(15, "Zombies perk install: client %i builtin %x session %x\n", index, ext.builtin.perks, ext.ext.sessionPerks);
        G_SP_RestoreScriptHintString(index, ext.scriptHintString);
        ++count;
    }
    iassert(count == level.maxclients);
    Com_Printf(15, "loadgame live: installed %i clients and SP side tables\n", count);
}

static void G_InstallSaveActors(MemoryFile *file)
{
    int actors = 0, sentients = 0, vehicles = 0;
    for (int i = 0; i < MAX_ACTORS; ++i)
    {
        actor_s *actor = &level.actors[i];
        G_ReadActor(actor, Actor_SP_Ext(actor), file);
        actors += actor->inuse != 0;
    }
    for (int i = 0; i < MAX_SENTIENTS; ++i)
    {
        G_ReadSentient(&level.sentients[i], file);
        sentients += level.sentients[i].inuse != 0;
    }
    for (int i = 0; i < SP_VEHICLE_COUNT; ++i)
    {
        // Unused records contain only the in-use marker. Keep the initialized
        // free-slot sentinel instead of reusing a prior decoded scratch record.
        G_ReadVehicle(&level.vehicles[i], file);
        vehicles += level.vehicles[i].entNum != ENTITYNUM_NONE;
    }
    Com_Printf(15, "loadgame live: installed %i actors, %i sentients, %i vehicles\n", actors, sentients, vehicles);
}

static void G_InstallSaveLevelSystems(MemoryFile *file)
{
    G_LoadCurves(file, cCurveManager::mCurves);
    // Saved command references address old XAnimInfo slots. Stage them until
    // the animation reader allocates and maps their new nodes. When animation
    // verification is disabled, retain the scratch-only behavior.
    const bool animations = G_SP_CheckpointOption("bo1_restore_animation");
    if (!animations)
        G_SP_BackupAnimCommands(true);
    G_SP_ReadAnimCommandRecords(file);
    G_SP_ReadAnimCommandList(file);
    G_SP_ReadAnimCommandHeads(file);
    if (animations)
        G_SP_StageSavedAnimCommandNodes();
    else
        G_SP_BackupAnimCommands(false);
    static unsigned int sounds[32][19];
    G_LoadVehicleInfoSounds(file, sounds);
    for (int i = 0; i < bg_numVehicleInfos; ++i)
        memcpy(bg_vehicleInfos[i].sndIndices, sounds[i], sizeof(sounds[i]));
    for (int i = 0; i < ARRAY_COUNT(turretInfoStore); ++i)
        G_ReadTurret(&level.turrets[i], file);
    for (int i = 0; i < ARRAY_COUNT(g_scr_data.actorCorpseInfo); ++i)
        G_ReadActorCorpse(&g_scr_data.actorCorpseInfo[i], file);
    level.actorCorpseCount = ARRAY_COUNT(g_scr_data.actorCorpseInfo);
    G_ReadThreatBias(&g_threatBias, file);
    g_listenerCount = G_ReadEventListeners(file, g_AIEVlisteners);
    level.pendingTriggerListSize = G_ReadPendingTriggers(file, level.pendingTriggerList);
    int cues[ARRAY_COUNT(level.droppedWeaponCue)];
    G_ReadWeaponCues(file, cues);
    for (int i = 0; i < ARRAY_COUNT(cues); ++i)
        level.droppedWeaponCue[i].setEnt(cues[i] ? &g_entities[cues[i] - 1] : nullptr);
    G_ReadAttractors(file, attrGlob.attractors);
    int zero;
    MemFile_ReadData(file, 4, (unsigned char *)&zero);
    iassert(zero == 0);
    Com_Printf(15, "loadgame live: installed level owners; animation graph/commands still pending\n");
}

static void G_CheckInstalledSaveOwners()
{
    int entities = 0, actors = 0, sentients = 0;
    for (int i = 0; i < MAX_GENTITIES; ++i)
    {
        const gentity_s *ent = &g_entities[i];
        if (!ent->r.inuse)
            continue;
        ++entities;
        iassert(ent->s.number == i);
        if (ent->client)
            iassert(i < level.maxclients && ent->client == &level.clients[i]);
        if (ent->actor)
        {
            iassert(ent->actor->inuse && ent->actor->ent == ent);
            ++actors;
        }
        if (ent->sentient)
        {
            iassert(ent->sentient->inuse && ent->sentient->ent == ent);
            ++sentients;
        }
    }
    Com_Printf(15, "loadgame live: owner back-references verified entities %i actors %i sentients %i player health %i points %i weapon %u; NO gameplay resumed\n",
        entities, actors, sentients, g_entities[0].health, level.clients[0].sess.cs.score.score, level.clients[0].ps.weapon);
}

static void G_RebuildSaveDObj(int index, unsigned char *visited, int *models)
{
    if (visited[index] == 2)
        return;
    if (visited[index] == 1)
        Com_Error(ERR_DROP, "Live save install: cyclic tag parent at entity %i", index);
    visited[index] = 1;
    gentity_s *ent = &g_entities[index];
    if (ent->tagInfo)
    {
        gentity_s *parent = ent->tagInfo->parent;
        if (!parent || !parent->r.inuse)
            Com_Error(ERR_DROP, "Live save install: missing tag parent at entity %i", index);
        G_RebuildSaveDObj(parent->s.number, visited, models);
    }
    // Use the normal model/attachment builder after all actor owners exist.
    // Actor tree banks still have their initial pose; animation nodes are a
    // separate missing save section, so this cannot enable gameplay yet.
    G_DObjUpdate(ent);
    if (!ent->client && ent->model)
    {
        iassert(Com_GetServerDObj(index));
        ++*models;
    }
    visited[index] = 2;
}

static void G_RebuildSaveWorldBindings()
{
    unsigned char visited[MAX_GENTITIES] = {};
    int models = 0, links = 0, collisionOwners = 0;
    for (int i = 0; i < MAX_GENTITIES; ++i)
        if (g_entities[i].r.inuse)
            G_RebuildSaveDObj(i, visited, &models);
    for (int i = 0; i < MAX_GENTITIES; ++i)
        if (s_saveWantedLinks[i])
        {
            SV_LinkEntity(&g_entities[i]);
            ++links;
        }
    // Query the actual collision sector lists, rather than trusting r.linked.
    // Zero-content entities intentionally have no collision registration.
    int hits[MAX_GENTITIES];
    for (int i = 0; i < MAX_GENTITIES; ++i)
    {
        gentity_s *ent = &g_entities[i];
        if (!s_saveWantedLinks[i] || !ent->r.contents)
            continue;
        const int count = CM_AreaEntities(ent->r.absmin, ent->r.absmax, hits, ARRAY_COUNT(hits), ent->r.contents);
        bool found = false;
        for (int j = 0; j < count; ++j)
            found |= hits[j] == i;
        if (!found)
            Com_Error(ERR_DROP, "Live save install: entity %i missing from collision sectors", i);
        ++collisionOwners;
    }
    Com_Printf(15, "loadgame live: rebuilt %i server model objects, %i saved links, %i collision owners verified; client synchronization pending\n",
        models, links, collisionOwners);
}

static void G_InstallSaveWorld(SPSaveGame *save)
{
    SP_SaveMemory_MoveToSegment(save, 3, true);
    MemoryFile *file = &save->memFile[0];
    G_LoadPathNodes(file);
    G_InstallSaveEntities(file);
    G_ReadBadPlaces(file);
    G_InstallSaveClients(file);
    G_InstallSaveActors(file);
    G_InstallSaveLevelSystems(file);
    G_CheckInstalledSaveOwners();
    if (G_SP_CheckpointOption("bo1_restore_animation"))
        G_LoadSaveAnimations(save);
    if (G_SP_CheckpointOption("bo1_restore_animation"))
        G_CheckRestoredAnimations(save, "before model rebuild");
    G_RebuildSaveWorldBindings();
    if (G_SP_CheckpointOption("bo1_restore_animation"))
        G_CheckRestoredAnimations(save, "after model rebuild");
    if (G_SP_CheckpointOption("bo1_restore_input"))
        G_RebaseInstalledSaveInput();
    if (G_SP_CheckpointOption("bo1_restore_handshake"))
        G_BeginSavedClientHandshake();
}
