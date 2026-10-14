# Native game profiler diagnostics

2026-10-13. Full Com_Init remains incomplete. No new diagnostic endpoints or
fake samples were introduced; original game profiler bodies remain selected.

Native profiling_diagnostics now measures mapped virtual space through the real
/proc/self/statm page count and actual _SC_PAGESIZE, including reserved mappings.
It validates outputs, IO/parsing and multiplication bounds. Errors return false
with errno; the enabled game profiler reports an error and aborts rather than
printing fabricated memory data. Windows GlobalMemoryStatusEx behavior remains.

ELF dladdr resolves real dynamic symbol names, or a basename/native-width module
offset when a local symbol is unavailable. It validates caller storage/capacity
and reports truncation failure. Builtin labels retain the original index fallback
on failed lookup. Android does not load dbghelp.dll or pretend to resolve PDBs.
Full native PC storage and report locals use uintptr_t; low32 is used only to
select the original hash bucket, never as the address passed to symbol lookup.

HONOR UBSan profiling-diagnostics-run1 passes real VA measurement before/during/
after reserving and releasing an untouched 128 MiB PROT_NONE mapping: 2238369792,
2372587520,2238369792 bytes. Exported fixture resolves its actual ELF symbol;
unexported local fixture resolves real module+offset. Invalid outputs/addresses,
zero capacity and short-buffer adjacent guard checks pass. Test source is
tools/port/tests/native_profiling_diagnostics_tests.cpp. Build1 links sanitizer
runtime statically and exports fixture symbols explicitly. Only this test binary
was transferred; existing game assets were reused. Actual game report output has
not run, and stripped static function names are not claimed available.

The normal whole g_main_mp source passes Windows x86/x64 compile comparison in
takeover_game_diagnostics_windows1 before final PC typedef/casts. Final Windows
PC type remains unsigned int and arithmetic retains its original width; final
type changes affect Android address storage. No gameplay comparison was run.

Full-engine-link107/108 now fail compilation only at retained server sampler
Windows APIs: CloseHandle/OpenThread flags, SuspendThread/GetThreadContext/context,
ResumeThread, Sleep and CreateThread. This is not a native profiler runtime gate.
Next implement real target-thread PC sampling plus sampler lifecycle and safe
publication/collection. Do not carry volatile-only cross-thread profiler/script
reads into ARM64; capture PC at native width and preserve reporting categories.
Then resume original engine link closure and production Com_Init execution.
