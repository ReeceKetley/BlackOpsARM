# Current State
Updated 2026-10-13 during full426 actual entity-link retry. Goal: genuine initialization,
frames, shutdown, headless Zombies simulation, then rendered play.

## Verified behavior and current gate

- Production FULL_DATABASE=ON target bo1_native_engine_entry links. Its startup
  compile commands exclude the historical BO1_STARTUP_SERVICES and
  BO1_STRING_POOL_GATE flags; historical standalone targets retain them.
- Real Com_Init returns and Com_Frame runs. Full411 actual console quit completed
  canonical zone/system teardown, exit0, with clean process marker pid16415.
  This proves the dedicated/headless baseline, not shutdown with a running map.
- Full414/415 load common_zombie and all Kino FastFiles, construct the collision
  world and enter G_InitGame. Full416 passes mantle animation setup and reaches
  real GScr_LoadScript/EmitCall compilation. No game/script frames are proved.
- Full416 EmitShort packed-operand failure is repaired. Full417 emits real
  script bytecode and reaches LinkThread, then stops134 at compiler292 through
  an indirect decoded uint32 operand pointer. Its sentinel read/store need
  fixed4-byte helper access, retaining Narrow32 and variable bookkeeping.
- LinkThread sentinel/read and target patch are repaired; semantic producer/VM
  audits distinguish packed operands from aligned objects. Native/Windows checks
  and focused actual linked-operand test5 pass. Full418 links0.
- Full418 passes linking and reaches GScr_PostLoadScripts/Radiant field
  registration. Terminal134 at149.089s: cscr_variable.cpp7364 stores uint16
  after a variable-length field name at an odd buffer offset. Root70714 terminal.
- Packed field producer and sole reader repaired; native TU/Windows bothABIs
  and focused actual lookup fixture16 pass. Record bytes and ownership unchanged.
- Full419 passes real script field/postload registration and reaches
  G_InitGame1008/G_LoadAnimTreeInstances. Terminal134 at150.396863s:
  XAnimCreateTree572 unsigned allocator callback receives signed
  Hunk_AllocActorXAnimServer(int). Root8542 terminal; that callback repair is now in full420.
- Matching419 SHA256621F36E7E120EA58CA062FE6FA82D49562AFAD41A2F9E03BBF1E3B000A0807EE,
  BuildIde288a7fea0e27379dd2dbb8fdaeb78da58464753.
- Typed tree factory and two actor/three CG callsites frozen. Viewmodel hunk
  alignment covers actual native tree8. Native/Windows affected TUs pass;
  genuine allocator focused tree26 passes. Full420 links0 (369 actions).
- Full420 passes actor trees and reaches G_InitGame1066 dynamic entities.
  Terminal134 at148.988984s: UnitQuatToAxis com_math1496 writes index3 beyond
  float[3] row; actual caller has modelAxis[3][3]. Root53131 terminal.
  Matching420 SHA256C4E5B26623F68D44B985BA69C5E802E28848F35EE3068A838A8FBC1B1C5CC02F,
  BuildId2dbe373923cb9ed7dfd62e5083bdd205430112fc; math repair now in full421.
- Math batch frozen:782 scalar accesses across34 canonical math functions
  use typed rows; root18 inline reads likewise typed. Windows bodies match
  oracle; actual asymmetric numerical/canary HONOR checks pass. Two fixture
  misses (inline reader/inverse44) retained and repaired, tests kept.
  DynEntity_server four bounds bytecopies frozen; collision links unchanged.
- Full421 passes dynamic entities and reaches G_InitGame1077 genuine entity
  spawning/field parsing/Scr_AddString. Terminal134 at149.168552s:
  IncInParam3352 forms eval_stack -1 before its VariableValue[2] array.
  Root4653 terminal; matching421 SHA25693163C094A1F4ABDF769C86DC6A94EAB4CFCE67977039F79194C2EDF7327A7AB,
  BuildIdc9a9e94a99af42bb46b1e74719513ffa101bc264. Next unique422.
- Native VM now owns prefix+two eval slots and prefix+2048 local IDs;
  original capacities/guards/reference flow and timeout clock preserved.
  Main/frame/history storage unchanged; VariableUnion fixed4/Narrow32.
  Native/Windows final TU checks pass; HONOR actual-owner fixture48 passes,
  including real field getters, reference retirement, local capacity and fatal
  overflow/third eval push. Fixture does not prove map/opcode/game frames.
- Full422 passes VM argument boundary and reaches trigger brush linking:
  trigger_use_touch/shared -> SV_SetBrushModel/SV_LinkEntity -> CM_BoxLeafnums97.
  Terminal134 at148.894895s: uint64 bounds load requires8, real float3 is4aligned.
  Root56058 terminal; pidof422 empty. MatchingSHA2564B038ACB71A9D902216C71F14D9A8BF28CE24C115BB689842DB693138289E5C8,
  BuildId5618b88b8b0696069eb138e56ce36a2ed69e84e4; copy repair now in423.
- Collision copy batch frozen: two bounds12-byte copies and exact four-byte
  cluster/tail decision via objectrepr. Native/Windows whole checks pass,
  Windows source byte-identical oracle; actual traversal fixture356 passes.
- Full423 passes trigger link/entity spawn and reaches real Kino level-main
  bytecode: Scr_SP_StartLevelMain -> VM_Execute_0 -> G_f_codespawn.
  Terminal134 at153.298206s: G_CallSpawnEntity808 calls SP_script_origin
  through void(gentity*,unsigned), actual callback void(gentity*,SpawnVar*).
  Root39587 terminal; matching423 BuildIdeda2bab6ff43d51316b7884ffdd68ae0b91fc57b.
- Root typed spawn local/lookup/call fixed, actual nullptr context preserved;
  native/Windows checks and Windows body oracle comparison pass. Four real
  server trace vec3 copies also frozen/pass; nominal contract, no prior
  runtime failure at those sites claimed. Full424 links0, all hashes intact.
- Full424 passes dynamic spawn and reaches actual script BulletTrace through
  dynamic-entity/model collision. Terminal134 at155.010669s: XModelTraceLine225
  loads uint64 from a valid four-byte-aligned float3. Root47212 terminal;
  matching BuildIdec32bba5d0657af813de0a59713d8cb21ac2dce0.
- Four exact12-byte model trace copies and21 inverse-matrix row accesses
  repaired. Windows projection byte-identical oracle; native/Windows checks
  and actual inverse-helper HONOR fixture42 pass. Full425 builds0 (3 actions).
- Full425 passes actual model tracing and reaches real ScrCmd_LinkTo,
  G_EntLinkTo/G_CalcTagAxis/G_CalcTagParentAxis. Terminal134 at155.801021s,
  map40.077894s: g_utils_mp1118 (*parentAxis)[9] exceeds float[3] row;
  actual caller owns float[4][3]. Root54274 terminal; pidof empty.
  SHA256EDA7BD5EAA87DA473849A197094C9F5E40807FDC8BE3D7BF079F082A109F1138,
  BuildId350c641a33949960a00c2e6c9d9a48051e59ca6e. Next unique426.
- Full426 tag/destructible row repair and five vector copies frozen: g_utils
  six sites; destructible three translation stores/nine transpose reads;
  exact copy lengths8/12 preserved. These include nominal related sites, not
  witnessed runtime failures at all sites. Native/Windows affected TUs pass.
- Android-only actual game-init/spawn/frame-work completion logs added. Frame
  work logs first3/every100 real framenum with real time/script tick/serving;
  before ownership release/completion publication, no readiness/time changes.
  Native3TUs and Windows bothABIs pass with established legacy compiler options;
  Windows projection adds no new behavior. Report native_game_completion_markers.md.
- Full426 links0 (9 actions), seven frozen source hashes intact. Current actual
  retry root57315: map40s, set bo1_autoquit1 at240s, backup quit260s, bound300s.
  SHA256EDBCCBA033F05D59B394E95BC2A13C6CA37A6EC19B81816BE2E0CF162FB57DE2,
  BuildId2220133ebe05f79fa25cce39b32dffce829187b0. Next unique427.
  Changed executable only transferred; assets/libraries reused.
- Returned game initialization/meaningful game frames, recovery, positive Activity/input/audio/GPU and
  rendered play remain unverified. Use production logs for each actual boundary.

## Latest integrated batch

- XAnim preserves signed/unsigned allocator signatures through canonical
  creation and hunk registration. Actual native sizes/alignment and debug
  allocation failures checked; real allocator/retirement fixture50 passes.
- Script animation allocators propagate by typed argument through four owners;
  CG has four exact allocation adapters and one genuine MT free adapter.
  BG forwards its unsigned provider directly and keeps parser rows in the real
  map animation arena through Load return/SP actor lookup/PostLoad.
- Shared platform/bo1_bytecode_io.h copies scalar1/2/4 operands. Compiler emits,
  backpatches and sorts packed records without changing bytes/lengths/handles;
  two local-variable shifts use actual typed rows. Actual compiler fixture22
  passes. VM readers retain fixed4 Narrow32/13+5 archive format; native unsigned
  cursor is four bytes and retained vectors use real owned aligned storage.
  Actual odd-offset VM scalar-reader fixture passes, not a whole-VM proof.
- Four animation-link and two debugger operands use exact packed widths.
  Native whole TUs, normal Windows x86/x64 and focused tablet checks pass.
  These checks do not prove initialized debugger or retail VM execution.
- All source batches frozen; actual map retry live as described above. Root owns
  CMake, BG/common/main/
  CL/GScr, shared bytecode header, ledger and production runs. Reuse agents
  for distinct concrete blockers as observed; coordinate shared header freeze.

## Next work

1. Observe current root57315 to terminal; fix next real script/game blocker.
   Retain real algorithms/owners; transfers are changed binaries only.
2. After simulation starts, prove meaningful real script/server frames and
   initialized teardown. Complete actual SDLActivity/shared engine/input/audio/
   rendering and application-accessible asset import before APK packaging.
3. Preserve actual algorithms, fixed32 disk/VM slots and native runtime pointer
   widths. No dummy success, fake readiness, assertion weakening or global packing.
   Keep tests proportional to new ABI/ownership/synchronization risks.

## Evidence and environment

- Main chronological report: docs/port/native_initialization_return_shutdown.md.
  Latest reports: native_xanim_allocator_contract_integration.md,
  native_anim_unsigned_allocator_integration.md, native_cgame_anim_callbacks.md,
  native_bg_animation_allocator_lifetime.md, native_compiler_bytecode_integration.md,
  native_vm_bytecode_access_integration.md, native_script_packed_consumers.md,
  native_script_field_buffer_integration.md.
  All under docs/port; older reports retain their recorded scope.
- Build build-android-audit/native_engine_entry1; explicit target
  bo1_native_engine_entry. Logs under build-android-audit/native_registry_closure,
  latest full-engine-link426.log and full-engine426-honor-zombies1.log.
  CMake D:/Android/Sdk/cmake/3.22.1/bin/cmake.exe; NDK30.0.16248370/API28.
- HONOR AKWJ9X5116G04618; ADB D:/Android/Sdk/platform-tools/adb.exe.
  Binaries /data/local/tmp/blackopsarm-native-engine-entry47-1. Matching
  SDL2/DXVK/shader/Steam/UBSan libraries are already staged; reuse them.
- 266 assets/~11.2GiB SHA verified /data/local/tmp/blackopsarm-game-data;
  user root /data/local/tmp/blackopsarm-user-data. Transfer changed binaries or
  genuinely missing/changed assets only. No per-file testing/retransfer.
- TASK-0030 application-UID accessible storage/import remains required before
  APK packaging. No immediate relocation of the native ADB data is needed.
- BlackOpsARM android-arm64; BlackOps64 is the validated read-only oracle;
  DXVK-Native1.9.2 and retail assets protected. No commit/push authorized.

## History

Prior current state is preserved verbatim, with SHA/size manifest, in
archive/animation-bytecode-handoff-20261013/STATE-through416.md.
Earlier handoffs, ledger records and failed logs remain evidence; history does
not override the current runtime gate or explicit user direction.
