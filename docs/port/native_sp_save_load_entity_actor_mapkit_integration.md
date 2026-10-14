# Existing SP save/load, entity, actor and mapkit integration

2026-10-13. Genuine Com_Init/Com_Frame remains incomplete; no tablet run/transfers.

Full bo1_native_database selects seven whole existing sources: g_sp_savegame,
g_sp_loadgame, g_scr_sp_entity, g_actor_model_state, g_mapkit, g_sp_level_exit
and actor_sp_ext. Original algorithms and explicit reference unsupported save
sections remain; this does not claim complete save support. Canonical original
SP preparation/reset/level-exit/entity/actor owners are connected.

Android save code omits the Windows intrinsics header; existing native fast-critical
section owner handles the Dvar read boundary. Mapkit uses bounded real snprintf
formatting with explicit overflow error and existing native OpenStdio rather than
secure Windows CRT calls. Windows branches remain unchanged. Mapkit write/close
return handling still follows the original source and requires runtime audit.

The bad-place runtime contains native pointer growth: Android asserts exact48/32
byte badplace/parms sizes rather than imposing the 32-bit44/28 sizes. Original
Windows size checks remain and field-offset/count checks stay active on Android.
This is a runtime-layout check, not permission to change retail wire formats or
proof of derived save-image portability. No writer algorithm or structure was
changed. Native actor field-table offsetof names sentientInfo.inl[n] explicitly;
an added48-entry assertion verifies inline coverage. Windows table stays unchanged.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link278 retains native intrinsic/CRT errors;
  link279 compiles adapters and exposes exact native size/offsetof failures;
  link280 compiles the corrected checks/table and reaches genuine remaining links.
- sp-save-entity-mapkit-definitions1.log confirms actual load preparation/commands,
  entity/actor-model reset, mapkit checks, level exit and actor anim storage owners.
- takeover_sp_save_load_entity_actor_mapkit_windows1: seven whole normal TUs pass
  x86/x64 exit0 after final source changes.

No initialized save/load/entity/actor/mapkit runtime has executed. Reference
SP_SaveNotPorted sections remain explicit. Derived save image layout/ABI/format
and extended actor sentient coverage, mapkit I/O, bounds and owner lifecycles need
execution/caller audit. Actual startup still requires remaining original owners
and real native platform services. No full saving or Com_Init completion claim.
