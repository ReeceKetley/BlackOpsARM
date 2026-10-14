# Existing parse-tree, actor commands and SP builtin integration

2026-10-13. Genuine Com_Init/Com_Frame remains incomplete. No game assets transferred.

Full Android database target now selects nine whole existing sources: cscr_parsetree,
actor_script_cmd, scr_sp_tables, server SP players/animation/AI/notported tables,
and client SP client/notported tables. Existing common command hash is selected
from its original com_shared owner. Parse nodes retain asserted four-byte sval_u
and existing Narrow32 region handles; debugger allocation retains sizeof its
native prefix. Entity handler table and ordered SP lookups are original owners.
No engine copies or newly generated stubs. The already accepted working Windows
reference's notported tables retain explicit Scr_Error behavior after real builtin
lookups. This does not claim their unsupported builtins were implemented.

A real ARM blocker in cg_scr_sp_client was x86 SSE scalar double-to-int conversion.
Windows retains both original intrinsic calls. Android keeps original float product
and double epsilon, using scalar_conversion.h's std::rint with active FP rounding
mode. Finite signed32 range is checked before casting; invalid results raise
FE_INVALID and return INT32_MIN, matching integer-indefinite. No saturation,
truncation or fake timing value is substituted for valid conversions.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link288.log compiles first owners and hash;
  link289 records x86-intrinsics failure; link290 compiles final native client
  adaptation but full link still fails genuine remaining dependencies.
- parsetree-sp-table-actor-definitions1.log records original debugger allocation/
  nodes, SP methods, entityHandlers and command hash definitions in actual archives.
- takeover_parsetree_actor_script_sp_tables_windows1: ten normal whole translation
  units pass x86/x64 exit0 with final source manifests.
- scalar-conversion-build2/run3: optimized static-C++ native UBSan executable passes
  52 checks on HONOR, including four rounding modes, ties, int32 boundaries,
  infinities/NaN and original epsilon. Initial shared-C++ binary could not launch
  because libc++_shared.so was unavailable at the chosen path; both failed logs
  are retained. The test was rebuilt using static C++ linkage, not engine changes.
- scalar-conversion-cmake-build1/run1: reproducible engine-build CMake target
  bo1_native_scalar_conversion_tests passes the same52 checks with actual build
  flags/UBSan. Transfers were only these small/new test executables.

Fixture proves scalar result/FE_INVALID behavior for tested inputs. It does not
prove all FP exception flags, signaling NaN handling, script parser execution,
Narrow32 publication/lifetime, initialized actor handlers or actual SP builtin
behavior. Full entry cannot link and has not run; input/audio/cinematic, remaining
original engine owners and real recovery services remain work.
