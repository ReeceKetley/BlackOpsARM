# Original raw-file, color, outdoor, pixel-cost, LOD and view helpers

2026-10-13 correction: the initial link198/199 claim was wrong. These sources
were accidentally added to the occlusion test target instead of the full library;
those builds did not compile them for the engine. Corrected wiring in link200
actually compiles all seven into bo1_native_database and reaches genuine remaining
link failures. Actual Com_Init/Com_Frame/rendering remain unexecuted.

Selected whole fx_convert, com_loadutils, r_outdoor, r_pixelcost_load_obj,
r_world_lod, r_extracam and rb_fog. Existing effect/color conversion, raw text
allocation/loading, outdoor image generation, pixel-cost color map, world LOD,
extra-camera policy and frame fog bodies now retained. All compile ARM unchanged;
no source algorithm, assertion, live state or readiness behavior replaced.

Evidence:200 compile log names all seven bo1_native_database objects; llvm-nm
confirms actual color/pixelcost/outdoor definitions, no longer unresolved in201.
takeover_raw_color_outdoor_lod_windows1 passes seven whole
normal Windows sources on x86/x64 exit0 with commands/manifests/build-result.json.
No additional runtime probe or device/asset transfer required for this selection.

Effect conversion sizes/pointers, actual raw-file ownership, image/device output,
world/view/LOD state and fog execution remain unverified. Compile evidence does
not certify original decompiled pointer/stride assumptions or prove actual GPU/
Com_Init behavior. Continue genuine physics/path/convex-hull, server/game/client,
audio/cinematic, renderer/input/network/recovery owners retained by199. Existing
native services and fixtures retain their documented narrower scope.
