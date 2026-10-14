# Original leaderboards, friends UI, news and recent-player owners

2026-10-13. Full executable remains unlinked; Com_Init/Com_Frame incomplete.

Full target selects canonical live_leaderboard.cpp, ui/ui_friends.cpp,
live_news.cpp and live_meetplayer.cpp. Original leaderboard initialization/query/
selection, friends Dvar/update, news and recent-player lifecycles are compiled,
with existing online/backend feature guards unchanged. Native leaderboard string
comparison uses canonical I_stricmp; Windows retains _stricmp.

Native recent-player adaptations:

- Android file_time.h reads actual CLOCK_REALTIME and converts Unix seconds plus
  nanoseconds to Windows 1601-epoch100ns ticks. Output is the original two uint32
  fields, unchanged8-byte FILETIME. Null/range/OS failures return false before
  publication; caller takes actual ERR_FATAL then abort if it returns.
- Canonical time functions use existing Bo1MeetPlayerTime alias. Windows alias
  remains _FILETIME; native alias remains scoped compatibility FILETIME.
- AddPlayerAtSlot uses actual record UID/gamertag/time instead of executable
  addresses and decompiled2416/48-byte pointer calculations. Existing high-slot/
  empty UID/name checks and session-request callback retained; native rejects
  negative slots/wrong controller through assertion and failure, not array access.
- Last-met elapsed calculation composes typed uint64 fields, uses integer100ns
  division, returns zero for a future timestamp/backward wall clock and saturates
  at UINT32_MAX for unrepresentably large elapsed seconds. No pointer aliasing,
  double conversion or narrow wrapping. Windows original calculation unchanged.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link354.log records FILETIME/CRT dependency
  errors; link355 compiles final adaptations; link356 is latest production attempt
  after fixture configuration and still fails genuine remaining dependencies.
- leaderboard-friends-news-recent-definitions1.log confirms actual owners.
- takeover_leaderboard_friends_news_recent_windows1: four normal whole TUs
  Windows x86/x64 exit0, final source manifests retained.
- recent-player-time-build1.log: focused CMake target uses actual canonical recent
  source with UBSan/GC and the actual inline Android clock provider. Fixture string
  copy/session-request/log/error boundaries isolate unavailable initialized engine
  consumers; no production provider or callback is replaced.
- recent-player-time-push1.log/run1.log: HONOR AKWJ9X5116G04618 ADB-shell UID exit0.
  Null-clock failure, actual realtime tick bounds, XuidInfo48/MetPlayerInfo2416-byte
  assertions, typed slot49/high UID/name boundary/untouched earlier slots, session
  invocation, five-second elapsed, future timestamp and uint32 saturation pass.
  String boundary uses isolated fixture provider, not proof of engine string or
  session/backend initialization. Only changed binary transferred, assets reused.

No real friend enumeration/Steam backend, leaderboard upload/download, news/ticker
render, recent-player file read/write, clock failure recovery or concurrent timestamp
publication executed. OS clock conversion is verified for real current time;
extreme clock range and OS failure paths remain source-audited only. Retained
fixed-format player-count/index bounds and other decompiled consumer arithmetic
need initialized runtime audit. Full link still requires genuine friends/backend,
renderer/audio/cinematic/debug/Monkey/platform and other dependencies. No complete
Com_Init return, initialized UI service or frame execution claim.
