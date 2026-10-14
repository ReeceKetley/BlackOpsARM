# Original server config/game and client message/input/HUD integration

2026-10-13. Full-engine-link218 compiles selected sources but fails remaining
genuine dependencies. Actual Com_Init/Com_Frame/server/network execution incomplete.

Selected whole sv_init_mp, sv_game, cl_parse_mp, cl_input_mp, cg_animtree_mp and
g_hudelem. Original config strings/server initialization/game dispatch, client
message parsing/packet construction, animation-tree ownership and HUD updates now
retained. Client input uses existing neutral gamepad API on Android rather than
XInput implementation header; actual native controller provider still required.

FULL inherited SV_Init/SV_GameCommand diagnostics excluded only where real source
owners replace them. OFF diagnostic selection unchanged. No initialized server,
config/network values or successful fake game command supplied.

Native server game-ownership counter is fixed32 lock-free atomic<unsigned int>.
Clear/increment/decrement/get/assertion reads now use atomic seq-cst operations;
original assertion and count semantics retained. Windows original volatile/
Interlocked branch unchanged. This fixes mixed atomic updates/plain reads, not
proof of actual server-thread spawn, TLS, event or full ownership lifecycle.

Evidence:

-216 preserves native XInput include error;217 uses neutral API and reaches
  inherited diagnostic duplicates;218 corrects exports and reaches genuine links.
- takeover_server_config_client_message_windows1 passes six whole normal TUs
  x86/x64 exit0 with commands/manifests/build-result.json.
- server-ownership-build1/run1 compiles actual production sv_init source/functions
  for API28 ARM64 with UBSan minimal runtime and function-section GC. HONOR proves
  clear/get and two cycles of four concurrent producers each adding256, followed
  by four each decrementing256, exact1024/zero counts. Assertion fixture fails
  terminally if production invariant fires; no artificial game/thread state used
  to claim initialization. Test exercises counter API only, not engine ownership.

Only new test binary transferred; assets reused. Actual server/config/client
messages/packet/input/animation/HUD and native pointer/stride/lifetime paths remain
unexecuted. Continue genuine physics preset/safe-mode/perk/config/live/profile/
server-spawn/game/audio/cinematic/input/network/recovery owners retained by218,
then run authentic production startup once linked.
