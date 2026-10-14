# Original player statistics and storage integration

2026-10-13. Full Com_Init/Com_Frame remain unexecuted and the full link fails.

The full database target now selects the existing canonical live_storage.cpp,
live_storage_win.cpp and live_stats.cpp. Actual stats-buffer selection, flags,
first-run state, basic-training queries, DDL/statistics and file-operation bodies
replace omitted source owners. This does not prove initialized player buffers,
DDL, persistent profile loading, file operations or online services. Existing
BO1_LIVE_STUBS conditionals retain reference policy; no new feature disabling,
successful backend replacement or fabricated fetch/synchronization was added.

Native adaptations:

- Windows _time64 calls use the existing real Android WallTime64 boundary.
  LiveStorage_GetUTC retains the engine's signed offset and 64-bit result.
  Reference server-completion callback remains under its existing backend guard.
- SystemTimeToInt uses time/gmtime_r and the original packed UTC year/month/day
  format. Clock/conversion failure takes actual Com_Error(ERR_FATAL), followed
  by abort if that unexpectedly returns. It never substitutes a ready/date value.
- Weekly/monthly server timestamps are existing signed32 values. They use the
  previously tested BreakDownTime32 provider with distinct caller-owned current,
  weekly and monthly tm objects. No shared CRT/POSIX static-buffer alias remains
  in the native comparison. Negative timestamps are rejected before signed32 elapsed-time subtraction;
  invalid timestamp conversion takes actual ERR_DROP
  then abort if it returns. Original Windows _gmtime64 calls remain unchanged.
- Native milestone-news concatenation uses canonical I_strncat with the actual
  statsName[64] destination. The decompiled Windows sequence relies on an adjacent
  newRank stack slot and 32-bit pointer subtraction; that sequence is retained
  only on Windows. Native follows engine bounded string policy.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link333.log accidentally built the default
  aggregate including older unlinked fixtures; no fixture was run. Subsequent
  builds explicitly target bo1_native_engine_entry.
- link334.log records missing Windows time APIs; link335.log compiles all three
  final source owners. link336.log records the production link attempt after
  adding the focused fixture. link337.log compiles the final timestamp guard and
  still fails real unrelated dependencies.
- native_registry_closure/live-storage-stats-definitions1.log confirms actual
  archive owners including stats buffers, UTC/date, first-run state, prestige
  writing and basic-training state. Symbol ownership is not runtime readiness.
- takeover_live_storage_stats_windows1 and final
  takeover_live_storage_stats_final_windows1 normal whole three TUs pass Windows x86
  and x64, with final source manifests and exit0.
- native_registry_closure/live-storage-clock-build1.log builds the focused CMake
  bo1_native_live_storage_clock_tests target with actual two canonical storage
  sources, function/data GC and UBSan. An isolated aborting Com_Error fixture
  boundary prevents pulling the whole recovery graph; production keeps actual
  Com_Error. Positive checks must never enter that boundary.
- live-storage-clock-push1.log/run1.log: HONOR AKWJ9X5116G04618, ADB-shell UID,
  exit0. Actual SystemTimeToInt matches real UTC date with TZ UTC0/EST5/JST-9;
  actual LiveStorage_GetUTC remains between real wall-time bounds at offsets
  zero/INT_MAX/INT_MIN and does not change the unsynchronized state. Only this
  changed fixture binary was transferred; existing game assets were reused.

The fixture does not execute weekly/monthly rollover, milestone publication,
negative-timestamp error recovery, stats-file import, backend tasks or APK-UID
storage. Those initialized consumers and retained decompiled native arithmetic,
buffer sizes, object lifetimes and protocol ownership still need runtime audit.
