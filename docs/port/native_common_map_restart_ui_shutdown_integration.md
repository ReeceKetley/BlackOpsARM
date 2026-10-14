# Original common map, restart, UI asset and shutdown integration

2026-10-13. Genuine Com_Init/Com_Frame remains incomplete. No device or asset transfers.

Selected canonical common.cpp's existing UI asset startup/callback, UI/map/common
FastFile loading, shared-level dependency lookup, frontend load/unload/after-error
policy, UI asset reload, restart, shutdown/internal shutdown, animation-tree and
weapon-memory lifecycle bodies. Twenty public native archive owners confirmed.
Loading flags, zone order, original dedicated/client conditions, UI error policy,
allocator ownership and cleanup calls retained. No duplicated engine implementation,
independent simplified startup, forced ready state or assertion bypass.

Full CMake target supplies BO1_NATIVE_COMMON_ASSET_LIFECYCLE only to its existing
startup-services common owner. This selection excludes the three inherited
unavailable endpoints Com_InitUIAndCommonXAssets, Com_LoadFrontEnd and
Com_LoadUiFastFile only in FULL_DATABASE=ON, where real bodies replace them.
OFF remains diagnostic; not rebuilt/run this scope. Source headers are canonical.

Android animation-tree allocation/free now use properly typed forwarding adapters
rather than calling differently typed functions through casts. Allocation validates
unsigned size <= INT_MAX before forwarding to original CG_AllocAnimTree(int);
invalid size reports actual ERR_DROP then aborts if error unexpectedly returns.
Free forwards original pointer/size/script instance to MT_Free. No new allocator,
copy, fake free, region bypass or lifetime change. Windows original casts retained.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link297..302 logs show genuine header closure
  errors and final compilation of selected common bodies.
- link303 verifies full-only CMake selection; link304 compiles final typed callbacks
  and still fails real remaining full-engine links.
- common-map-restart-ui-shutdown-definitions1.log confirms twenty actual archive
  owners, before typed forwarding change (same public names retained).
- takeover_common_map_restart_ui_shutdown_windows1/final_windows1/callbacks_windows1
  each compile one normal whole common TU for x86/x64 exit0 with source manifests;
  callbacks_windows1 is final source evidence. Earlier checks retained because
  subsequent guard/callback edits required renewed source comparison.

No actual map/UI assets, XAnim allocation/free, weapon-memory teardown, shutdown,
restart or frontend runtime executed. Preserved reference SP frontend unsupported
save-buffer/zone behavior remains as documented in original source. Callback ABI
is corrected but allocation ownership/range failure path needs initialized execution.
Map dependency parsing/zone bounds and recovery ordering are runtime gates.
Expanded full engine still cannot link; remaining real engine/native dependencies
include audio/cinematic, input, networking, live/profile and recovery services.
No Com_Init return, Com_Frame or game-ready claim.
