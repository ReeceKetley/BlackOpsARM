# Full430 restart mode audit

The apparent switch to `tdm` is not a restart failure. The completed full430 run
loads the same `*_zombie_theater_tdm.csv` and optional
`maps/gametypes/tdm.gsc` on its first map as on its second map. Both genuine game
initialization and server spawn complete, followed by 300 completed server/script
frames on each initialization. The runner exits0 after canonical mapped quit,
at 309.8142776 seconds. Sources remain frozen; this audit changes no engine code.

The mode selection owners match the read-only BlackOps64 reference:

- `SV_Map_f` does not force a Zombies gametype string. It applies the original
  SP cheat policy and calls `SV_SpawnServer`.
- `SV_MapRestart(0)` calls the same `SV_SetGametype` and then `SV_SpawnServer`
  for the current map. `SV_SetGametype` uses the inherited `g_gametype` string
  with default `tdm`; there is no separate `zclassic` map-command policy.
- `Com_LoadLevelFastFiles` derives `zombiemode` from the actual map name on each
  load, including restart. `Com_IsSPLevel` and `G_SP_IsSPLevel` select the SP
  Zombies code from that mode, independently of the inherited MP gametype string.
- Real `GScr_LoadScripts` calls `GScr_SP_LoadScripts` on SP levels. Real
  `G_InitGame` invokes `Scr_SP_StartLevelMain`, rather than the MP gametype main,
  pregame or startup-gametype execution path.

The noisy CSV lookup comes from inherited `CCS_ShouldLoadConstConfigStrings`,
which returns true in both branches and is also unchanged in BlackOps64. It
constructs its filename from the inherited `g_gametype` string. Separately,
`GScr_SP_LoadCallbacks` tries the optional `maps/gametypes/<ui_gametype>::init`
callback only when that UI dvar is nonempty, with existence enforcement disabled.
That function is the same in the Windows reference. Seeing its missing `tdm`
script diagnostic does not mean MP game logic replaced the Zombies level main.
The actual source that assigned `ui_gametype=tdm` was not captured in this run;
the current/default dvar values are not evidence of arbitrary native corruption.

Full430 evidence:

- First missing CSV/script records: log lines 16920/17073. First game-init/spawn
  completion: 20175/23121. Frames 100/200/300 reach level time 15000.
- Second missing CSV/script records: 23430/23587. Second game-init/spawn
  completion: 26696/29647 at level time 15000, with frame/script counters reset 0.
  Resumed frames 100/200/300 reach level time 30000 and serving 400/500/600.
- `full-engine430-honor-restart-baseline1.log` and `.log.json` retain the actual
  command schedule and exit0. `full430-mode-audit.json` records newline-normalized
  source-range hashes for six actual policy owners, all identical to BlackOps64.
  All evidence is under `build-android-audit/native_registry_closure`.

There is no evidence requiring a forced gametype, Dvar re-registration change,
per-asset repair or assertion bypass. Keep the original mode policy while root
continues the measured frame-performance and real APK/rendered-provider work.
