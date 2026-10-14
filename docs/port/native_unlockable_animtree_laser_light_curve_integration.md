# Native item, animation tree, laser, light and curve owners

2026-10-13. Genuine Com_Init/Com_Frame remains incomplete. No tablet run or asset
transfer occurred.

Full bo1_native_database now selects six whole canonical sources:
bg_unlockable_items, cscr_animtree, cg_animscripted_mp, cg_laser,
r_light_load_obj and FxCurve_load_obj. Original item-table lookup, animation-tree
parsing/access, scripted entity linking, laser construction, light and effect-curve
loading algorithms remain intact. No fake result, startup or readiness added.

Android animation-tree compilation includes standard character classification
declarations and the existing legacy CRT formatter. Windows includes/behavior
remain unchanged. The 84-byte TreeNameMap element contains int animIndex and
char strName[80], so its size is not a native-pointer allocation bug; original
allocation retained. No broad structure-size substitutions were made.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link242.log preserves missing character/
  formatter declarations; link243 compiles the correction and fails genuine links.
- native_registry_closure/unlockable-animtree-light-curve-definitions1.log confirms
  actual archive definitions for item lookup, Scr_GetAnims, entity linking, laser,
  light and curve loading.
- takeover_unlockable_animtree_laser_curve_windows1: all six whole normal sources
  pass x86/x64 exit0 after final source changes.

No initialized animation parsing, item lookup, entity linking or GPU/asset loader
runtime has executed. Retained parser bounds, signed-character classification,
curve allocation arithmetic, native layout and ownership still need execution/
caller audit. Actual full entry still needs audio/cinematic/live/profile/game/
client/renderer/network/recovery dependencies. Prior unrelated probes not repeated.
