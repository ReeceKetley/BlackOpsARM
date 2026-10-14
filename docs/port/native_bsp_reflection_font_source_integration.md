# Original BSP, reflection, Dvar and text/render source integration

2026-10-13. Latest full-engine-link166 compiles newly selected sources and fails
linking genuine remaining dependencies. Actual Com_Init/Com_Frame unexecuted.

Seven complete canonical sources selected: r_bsp_load_obj, com_bsp_load_obj,
dvar_cmds, r_reflection_probe, r_font, rb_shade and com_stringtable_obj. Full
Dvar commands replace startup's partial source selection; full reflection probe
replaces entry_state's partial selection, preserving one owner per source.
Original BSP material/version/sun-light/shadow helpers, reflection generation,
Dvar info strings, font measurements, tessellation globals/overflow and raw
string-table hash/loader implementations now supply their retained symbols.
No new diagnostic or successful placeholder supplied.

Android Com_ShutdownWorld uses __atomic_thread_fence(__ATOMIC_SEQ_CST) for the
original ordering barrier formerly expressed by Windows InterlockedExchange on
a local variable. The world in-use state update remains original. Windows is
unchanged. This preserves the full ordering barrier; it does not establish safe
concurrent world teardown or certify other accesses to the world flag.

Evidence:

- full-engine-link164 preserves missing Windows InterlockedExchange compilation
  failure. Native barrier adapted;165 reaches genuine linker dependencies.
-166 compiles unchanged original font/shade/string-table sources and reaches
  linking. No new source-level adaptations required for those three.
- takeover_bsp_dvars_reflections_windows1: four whole normal Windows TUs pass
  x86/x64 exit0; commands/source manifests/build-result.json retained.
- takeover_font_shade_stringtable_windows1: three whole normal Windows TUs pass
  x86/x64 exit0 with the same evidence format.

No new runtime probe or device/asset transfer. BSP loading, reflection rendering,
native font/tessellation execution, Dvar command execution and raw table loading
remain unverified. Wider original decompiled pointer/stride/math assumptions
still require runtime-driven audit; compiling full sources is not blanket ABI
or rendering correctness evidence.

Remaining link166 gates include audio/cinematic, active client query, vertex
helpers/render targets/draw surface owners, real server state/trace, game/script,
input/events/network and recovery. The expanded worker-context probe from the
previous milestone also remains unlinked; prior passing queue proof has narrower
scope. Continue authentic source/service integration, then execute actual entry.
