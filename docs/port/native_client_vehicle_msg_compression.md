# Original client world/vehicle/UI integration and real MSG compression

2026-10-13. Full-engine-link183 compiles new sources and fails remaining genuine
dependencies. Actual Com_Init/Com_Frame unexecuted.

Selected complete cg_world, cg_draw_debug, cg_vehicles_mp, g_debug, g_scr_vehicle
and ui_emblem. These supply actual area-entity query, client/background vehicle
information, debug-star and overlays, vehicle/script operations and emblem UI.
Only native cg_draw_debug formatter include needed; original logic/assertions
retained. No fake vehicle/world/emblem/debug state provided.

The retained graph required deflate. Added original modified-zlib deflate.c and
trees.c beside existing matching inflate/inffast/inftrees/adler32/crc32/zutil in
the full target. No system zlib substituted. Actual MSG_CompressWithZLib and
MSG_DecompressWithZLib previously passed fixed52-byte Win32 stream size. Android
now passes sizeof(stream), and initializes zalloc/zfree/opaque using typed null
assignments, including the wider opaque pointer missed by original16-byte clear.
Existing compression parameters remain level9/raw window-13/memlevel1/strategy0;
original error/cleanup/return behavior preserved. Windows calls unchanged.

Evidence:

-180 native debug formatter error corrected;181 compiles selected six and reaches
  real linker dependencies including deflate symbols.182 includes actual codec
  sources;183 includes native MSG stream-width fix and reaches genuine links.
- takeover_client_world_vehicle_emblem_windows1: six whole normal TUs pass x86/
  x64 exit0 with commands/manifests/build-result.json.
- takeover_msg_zlib_windows1: whole normal MSG TU passes both ABIs exit0.
- msg-zlib-probe1/build.log builds actual production MSG source and all eight
  matching modified-zlib sources for Android28 ARM64 with UBSan minimal runtime,
  real signed-char/MSVC compatibility/native headers and function-section GC.
- msg-zlib-probe1/run1.log on HONOR passes4096-byte original-wrapper exact round
  trip, actual raw deflate output, insufficient compression output failure,
  insufficient decode output failure and truncated input rejection. Both encoder
  and decoder run actual sources. No exhaustive corpus, Windows bytecode parity,
  network/save/demo execution or initialized engine state claim.

Only this new test binary transferred; retail assets reused. Selected world,
vehicle/script/debug/emblem native pointer/stride paths remain unexecuted and
require runtime-driven audit. Native runtime z_stream widths do not change the
raw compressed format or fixed32 game metadata. Other retained code may still
contain Win32 stream-size assumptions and must be audited when integrated.
Continue actual audio/cinematic/weapon/model/client/game/UI/script/renderer/
platform owners shown by link183, then run authentic production startup.
