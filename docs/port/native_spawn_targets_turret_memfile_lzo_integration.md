# Existing spawn, targets, turret, commands and memory-file integration

2026-10-13. Genuine Com_Init/Com_Frame remains incomplete; no tablet run/transfers.

Full bo1_native_database now selects seven whole existing sources:
g_spawnsystem_mp, g_targets, turret, g_svcmds, r_draw_cmdbuf, memfile and the
matching minilzo.cpp. Original spawn/target/turret/server-command/draw-reader/
memory-file/codec algorithms remain. Native spawn strings use the existing CRT
comparison adapter. No independent codec, dummy initialization or format changes.

Memory-file's Windows reference already uses native lzo_uint variables instead of
passing four-byte length slots to codec APIs; retained unchanged. Android now checks
actual lzo_init()==LZO_E_OK before setting the existing initialized flag. Failure
uses actual Com_Error(ERR_FATAL), aborting if it unexpectedly returns. Original
Windows initialization expression remains. Native initialization concurrency,
compression/decompression envelope bounds and codec alignment are not certified.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link274.log preserves missing native string
  comparison declarations; link275 compiles six owners and exposes genuine LZO
  dependencies; link276 compiles matching codec and resolves its named links;
  link277 compiles final initialization check and fails genuine other dependencies.
- spawn-target-turret-memfile-definitions1.log confirms actual spawn/quaternion,
  target/turret/server-command/draw-reader/memory-file/codec archive definitions.
- takeover_spawn_targets_turret_memfile_lzo_windows1: seven normal whole TUs pass
  x86/x64. takeover_memfile_lzo_init_final_windows1 repeats only modified memfile;
  both ABIs pass exit0.

No initialized spawn/target/turret/command/memory-file or LZO runtime occurred.
Codec output/native ABI/source bounds and owner lifecycle remain focused execution
gates; original unchecked decompressor uses require caller audit before untrusted
save input execution. Full entry still lacks omitted original owners and genuine
Android services. Prior unrelated probes not repeated; no successful startup claim.
