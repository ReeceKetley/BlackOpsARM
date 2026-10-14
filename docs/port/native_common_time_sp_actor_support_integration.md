# Native common time and existing SP actor support

2026-10-13. Genuine Com_Init/Com_Frame remains incomplete; no asset transfers.

Original Com_GetQTime and Com_RealTime now selected in native boot from com_shared.
Native OS boundary resides in platform/android/calendar_time.h: real std::time,
localtime_r/gmtime_r with caller-owned tm, existing low32 signed time contract,
and rejection of negative timestamps with EINVAL. Engine copies original nine
calendar fields only on conversion success; null output retains original no-op
behavior. Windows original _time64/_localtime64/_gmtime64 path unchanged.

Reference rejection semantics checked against Microsoft primary documentation:
https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/gmtime-gmtime32-gmtime64
https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/localtime-localtime32-localtime64
No widened timestamps or serialized/caller ABI changes. Existing signed32 time
limit remains; this is not a post-2038 redesign. Local time uses process timezone;
APK/device timezone binding remains unverified. Fixture controls TZ explicitly.

Seven existing whole sources connected unchanged: actor_pain, actor_team_move,
bg_actor_prone, g_scr_sp_ai_cmds, scr_sp_debug, cg_sp_nettrace and cg_sp_feeltrace.
Original actor/SP script and optional trace policies retained, no diagnostic state
forced on and no replacement successful callbacks.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link294.log compiles initial eight owners;
  link295 final negative-rejection common path; link296 compiles final platform
  boundary, links actual common-time fixture, still fails real full-engine links.
- common-time-sp-support-definitions1.log records actual common time and actor/
  prone/script-debug/trace archive owners.
- takeover_common_time_sp_support_windows1: eight normal whole TUs x86/x64 exit0;
  takeover_common_time_final_windows1 confirms final common TU bothABIs exit0.
- common-time-build1/run1 and final common-time-run2: actual engine common time
  source linked into bo1_native_common_time_tests, HONOR UBSan passes UTC epoch,
  leap day, INT32_MAX, UTC/local offset, untouched output/EINVAL for invalid input,
  null output, 8192 conversions across four threads, real live clock bounds.
  Final fixture rebuilt after OS code placed in Android boundary. Only changed
  fixture executables (~1.23MiB) pushed; assets reused.

Time fixture does not prove actual initialized script/calendar callers, physical
Android timezone selection, concurrent TZ mutation or clock-failure injection.
New actor/script/trace lifecycles unexecuted; trace code does not imply headless
simulation or frontend startup. Full entry still cannot link; remaining real
engine/native audio/cinematic/input/live/server/recovery dependencies persist.
