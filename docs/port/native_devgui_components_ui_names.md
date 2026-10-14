# Original debug GUI, weapon components, UI renderer and display names

2026-10-13. Full-engine-link210 compiles selected sources but fails genuine
remaining dependencies. Actual Com_Init/Com_Frame unexecuted.

Selected whole bg_weapons_util, devgui/devgui_util/devgui_input, r_ui3d and
ui_viewer. Shared canonical Com_DisplayName/CS_DisplayName bodies now included
in native boot with the actual clientState_s header; no copied name/state logic.
Original component parsing, debug GUI/viewer and UI rendering algorithms retained.

Neutral GPad_IsActive/GetStick declarations moved to canonical client/cl_gamepad
header; native devgui excludes XInput implementation header. Actual native
controller provider still required, no dummy button/device values supplied.
Native UI shutdown waits through existing actual Sleep boundary.

Native-only UI3D initialized/rendering flags are lock-free atomic<bool> with
size assertion; original field widths preserved and Windows bool fields unchanged.
Existing reads/writes use seq-cst operations. This corrects plain cross-thread
flag access, not certification of every UI field, render target publication,
external synchronization or complete shutdown/resource retirement lifecycle.
Original assertions and initialization/resource calls retained.

Evidence:

-208 preserves native XInput/Sleep errors;209 adapts these and catches wrong
  client-state header.210 uses exact canonical header and reaches real linker gates.
- llvm-nm confirms actual component parser, DevGui_Draw, UI3D per-frame init and
  viewer draw implementations in native database library.
- takeover_devgui_components_ui_windows1 passes eight whole normal Windows TUs
  (six selected plus common and win_gamepad) x86/x64 exit0 with commands/manifests/
  build-result.json.

No runtime probes or device/asset transfers. Names/weapon parsing/debug input/UI
rendering and concurrent UI lifecycle/native pointer/stride paths unexecuted.
Continue real remaining menu/timing/sun/static-trace/server/client/audio/cinematic/
input/network/recovery services from210; run authentic startup once entry links.
