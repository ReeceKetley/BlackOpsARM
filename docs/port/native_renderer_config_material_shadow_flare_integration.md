# Original renderer configuration, materials, shadows and flare integration

2026-10-13. Genuine Com_Init/Com_Frame remains incomplete. No device/asset transfers.

Eight existing whole sources selected unchanged: r_draw_material,
r_char_tech_util, r_draw_shadowablelight, r_draw_sunshadow, rb_spotshadow,
rb_sunshadow, rb_superflare, ui_screenshot. Material time/constants, techniques,
shadow drawing and flare visibility retain original device/query/worker behavior.

Existing r_init owner now selects seven original configuration helpers through
BO1_NATIVE_RENDER_CONFIG in the full database target: R_MakeDedicated, SetGfxConfig,
R_AllocateMinimalResources, R_SetIsMultiplayer, R_TrackStatistics,
R_ConfigureRenderer and R_StereoActivated. Bodies unchanged. Config validation and
sizeof(gfxCfg) copy retained; actual dedicated path registers dvars when needed,
sets/commits r_loadForRenderer=false, then calls real DB_LoadGraphicsAssetsForPC.
No fake renderer readiness, suppressed DB loading, stereo override or successful
query substitute. Device/window init, recovery and full registration remain work.

UI screenshot source retains reference BO1_SCREENSHOT conditional policy. This
selection does not enable or implement screenshot capture/decompression on native;
when that feature is enabled, native image allocation/stride/ownership needs audit.
Do not confuse an existing conditional shutdown owner with screenshot readiness.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link309.log compiles new source/config
  selection; target definition change rebuilds the database sources (579 build
  steps). This is source compilation, not rerunning prior runtime fixtures/assets.
- link310 compiles charred technique dependency; full executable still fails real
  remaining links.
- renderer-config-material-shadow-flare-definitions1.log records actual seven
  configuration helpers and selected material/shadow/flare/conditional screenshot
  owners in database archive.
- takeover_renderer_config_material_shadow_flare_screenshot_windows1: eight normal
  whole TUs pass x86/x64 exit0 with final source manifests.
- takeover_charred_material_tech_windows1: remaining normal whole TU passes both
  Windows ABIs exit0 with source manifests. No prior runtime probes repeated.

No original dedicated graphics-asset lifecycle, renderer commands, device/query,
shadows/flare/GPU or screenshot runtime executed. Native render state/ring ownership,
color-byte stores, draw-list bounds, initialized thread/device prerequisites and
recovery remain caller/runtime gates. Seven selected helpers do not implement
full r_init device/window behavior. Actual full Com_Init/Com_Frame remains unlinked;
remaining engine/native audio/cinematic/input/live/profile/network/recovery owners
must still be connected and implemented without changing pinned Vulkan requirements.
