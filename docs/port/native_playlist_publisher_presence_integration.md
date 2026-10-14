# Original publisher storage, playlist presence, ticker and server autoplaylist

2026-10-13. Real Com_Init/Com_Frame incomplete; full engine remains unlinked.

Full target selects canonical live_storage_pub.cpp, live_presence_win.cpp,
live_ticker.cpp and server/sv_autoplaylist.cpp. Original playlist flags, matchmaking
presence setters, publisher storage processing, ticker/UI and server auto-playlist
owners now compile. Existing online-service guards and original readiness state
transitions remain unchanged; no forced ready flags or new backend substitutes.

Native adaptations:

- LiveStorage_DecompressBuffer zero-initializes the actual modified z_stream,
  passes sizeof(stream) to the engine's matching modified zlib initialization,
  and rejects initialization failure. Windows retains its original 12-byte clear
  and fixed52 call. Existing Z_STREAM_END assertion, cleanup and return predicate
  remain; no assertion weakening or system-zlib substitution.
- Native online WAD generation-time logging uses caller-owned ctime_r storage
  instead of _ctime64. Conversion failure takes actual ERR_DROP, then abort if
  unexpectedly returned. This is a logging adaptation, not a validated WAD parser.
- Native SV_GroupError uses bounded vsnprintf, va_end before engine logging and
  explicit formatting-failure text. Original literal-safe GROUPS %s logging and
  Windows formatting path remain.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link351.log records the Windows formatter
  dependency; link352 compiles final source adaptations; link353 is latest production
  attempt after fixture configuration, still fails genuine other dependencies.
- playlist-publisher-presence-definitions1.log confirms actual original owners.
- takeover_playlist_presence_publisher_ticker_windows1: four normal whole TUs
  x86/x64 exit0 with final source manifests.
- publisher-decompression-build1.log: focused CMake target uses actual publisher
  and autoplaylist sources with GC/UBSan and matching database modified-zlib archive.
  Fixture logging captures messages; unexpected assertions abort. Production uses
  real engine logging/assertion owners.
- publisher-decompression-push1.log/run1.log: HONOR AKWJ9X5116G04618 ADB-shell UID
  exit0. Binary payload round trip and output canaries pass. Child processes hit
  retained assertions/SIGABRT on truncated compressed input and insufficient output.
  Actual group varargs preserve literal percent text and bound long output to127
  bytes. Only changed fixture binary transferred; staged assets reused.

No real publisher/backend fetch, playlist presence login, ticker render, server
auto-rotation, WAD generation-time caller or initialized error recovery executed.
The WAD container still needs received-length tracking and a bounds/endian audit
of header counts, table/name/offset/compressed-size ranges, decompression result
handling and text termination before certifying online data processing. Its
original completed-attempt flag is not proof that a valid playlist was loaded.
No whole WAD or backend readiness claim follows from the decompressor fixture.
Native audio/cinematic/renderer/Steam/debug/Monkey and other actual dependencies
remain unresolved; complete Com_Init return and frame execution are unproven.
