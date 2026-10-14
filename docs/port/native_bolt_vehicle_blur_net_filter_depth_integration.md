# Native bolt, vehicle, blur, network display, filter and depth owners

2026-10-13. Genuine Com_Init/Com_Frame remains incomplete. No tablet run/transfers.

Full bo1_native_database selects eight whole canonical sources: cg_bolt,
cg_vehicle, cg_scr_sp_blur, cg_draw_net_mp, rb_imagefilter, r_cmdbuf,
rb_depthprepass and cl_medal. Original bolt/vehicle behavior, SP blur script
functions, network display, image filtering, context/depth setup and medal lookups
are retained. No fake device/readiness or independent implementation added.

Android vehicle state/cache allocate sizeof(cgVehicle_s)/sizeof(vehicle_cache_t)
instead of84/1928 bytes. Corresponding MT_Free calls in canonical CG_ShutdownEntity
use those same sizes, preserving script instance and allocator type. Native pointer
growth must not cause zeroing/writes past the block or mismatched free classes.
Windows allocations/frees and serialized fields remain unchanged. No initialized
vehicle/pool alignment/retirement result is claimed by this compile correction.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link260.log compiles eight original sources;
  link261 compiles final native allocation/free sizes and fails genuine links.
- native_registry_closure/bolt-vehicle-filter-depth-definitions1.log verifies
  actual archive bolt, vehicle controller, lagometer, Gaussian filter, context,
  depth prepass and medal owners. SP blur source compiles; it does not supply all
  missing SP blur/time-scale extension functions.
- takeover_bolt_vehicle_blur_net_filter_depth_windows1: eight whole normal TUs
  pass x86/x64. takeover_vehicle_native_alloc_free_windows1 repeats only final
  modified vehicle and snapshot sources; both ABIs pass exit0.

No initialized bolt/vehicle/blur/display/filter/depth/medal runtime has executed.
Allocator capacity/alignment, vehicle/cache construction and cleanup, native
offsets, render resource lifetime/recovery remain focused runtime/caller gates.
Full entry still needs real audio/cinematic/live/profile/game/client/renderer/
network/recovery dependencies. Prior unrelated probes not repeated; no startup
or gameplay completion claimed.
