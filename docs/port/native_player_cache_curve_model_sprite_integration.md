# Existing player cache, curve, model and sprite owners

2026-10-13. Genuine Com_Init/Com_Frame remains incomplete. No device/asset transfers.

Seven whole existing sources connected: live_pcache/live_pcache_profile,
EffectsCore/FxCurve, r_staticmodelcache_load_obj, r_draw_model_util, r_sky_load_obj,
r_sprite. Existing curve loader was already selected; this scope selects actual
iterator create/release owner. Original profile/cache lookup/locking, curve,
model/sun/sprite/vertex-buffer algorithms retained, no fabricated profile/GPU state.

Native adaptations:

- Cache component pools use sizeof(PCachePublicProfile/PCachePlayerEmblem) rather
  than Windows 288/432 strides, preserving original 256/32 capacities, offsets,
  callbacks and TTL. Emblem pointers grow on LP64; strides/clears must match arrays.
- PCache_Time uses real 64-bit wall time through Android calendar_time.h; Windows
  retains _time64. Existing native FCS locking/load helpers retained unchanged.
- Static-model temporary pointer array uses sizeof(*drawInstArray), validates
  count against INT_MAX allocation contract before multiplication, then invokes
  original hunk allocator/free. Range failure actual ERR_DROP, abort if it returns.
- Original x86 _BitScanReverse maps to std::countl_zero only after nonzero input;
  same ceil-log2/minimum4 branch, counts and allocator policy. Windows intrinsic
  and original allocation unchanged.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link311 compiles initial selections;
  link312 final scan expression; link313 final OS clock boundary, still fails
  real remaining full-engine links.
- player-cache-curve-model-sprite-definitions1.log confirms actual cache/curve/
  model-resource/stream/sun/sprite owners (before clock-boundary forwarding change;
  public definitions unchanged).
- takeover_player_cache_curve_model_sprite_windows1: seven normal whole TUs pass
  x86/x64 exit0. takeover_player_cache_native_clock_final_windows1 confirms final
  cache source bothABIs after moving OS clock call to Android boundary.

No initialized cache/profile/DDL/batch fetch, component release/locking lifecycle,
curve asset, model-resource assignment/overflow rejection, native quad/vertex
buffer/draw or sun loader runtime executed. Existing FCS and common32 clock
fixture evidence does not verify these cache lifecycles or cache64 API. Original
sprite decompiled packed numeric conversions and model-resource arithmetic/bounds,
asset/renderer readiness, callback lifetime and thread ownership need runtime
verification. No new fixture mirrors memcpy/sizeof or repeats earlier checks.
Full executable still unlinked; real audio/cinematic/input/live/profile/network/
recovery dependencies remain. No startup completion or percent estimate derived
from source counts; link progress is not Com_Init execution progress.
