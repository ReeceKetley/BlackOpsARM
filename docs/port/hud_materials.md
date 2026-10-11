# HUD material registration (batch45)

## Cause

The existing `CG_Init` registration block assigned eleven materials to the
wrong `cgMedia` members. For example, `hudDpadLeftHighlight` received the
missing `nightvision_overlay_goggles` material, while its intended
`hud_dpad_outer_frame_highlight_side` material went into `ammoCounterRocket`.
The ammo-counter assignments were shifted too; several were subsequently
overwritten by life-counter and text-decoding registrations.

The action-slot draw routine uses `hudDpadLeftHighlight` for a large rotated
highlight around the selected equipment. Rendering the missing material there
produced the checker square covering Moon's lower-right weapon HUD.

## Change

Assign each material to the corresponding named member, including the previously
unset night-vision, D-pad arrow and D-pad circle members. Preserve the asset
names and draw routines. This is a shared engine correction, independent of
pointer width and the graphics backend.

## Baseline evidence

- `w32_moon_hud45_baseline`: frozen batch42 Win32 executable, system D3D9,
  five Moon weapon cases PASS, zero FAIL, normal exit. Real back-buffer captures
  reproduce the checker square, including `main/screenshots/bo1_45000.jpg`.
- Previous unrestricted x64 system-D3D9 and Vulkan captures reproduce the same
  square. It is therefore not introduced by x64 or DXVK.
- Win32, unrestricted x64, low-address x64 and guard builds compile the corrected
  registration with zero errors and no truncation warnings in this compilation.

## Corrected Win32 result

`w32_moon_hud45_fixed` uses the new Win32 executable and the same five weapon
cases as the frozen baseline. All five pass with zero failures; the process
exits normally after125seconds, with no WER or Application Error event.
Back-buffer screenshots at45000,65000,85000 and105000ms show the corrected HUD.
The first capture shows the dual Wave Guns with the equipment highlight and
ammo display visible; the large checker square is gone.

The tested weapons are `zombie_quantum_bomb`, `microwavegundw_zm`,
`microwavegun_zm`, `microwavegundw_upgraded_zm`, and
`microwavegun_upgraded_zm`. This private-desktop regression uses automated god
mode and muted audio.

## Corrected Vulkan result

`x64hi_dxvk45_moon_hud` uses the corrected unrestricted x64 executable and
the pinned, locally patched DXVK3.1.1 package. The same five weapon cases pass,
with zero failures, normal exit0 and all four SaveGame checks. Explicit Khronos
validation reports no errors, including device teardown; implicit overlay layers
remain disabled in this child process. Sound is enabled and automated god mode
is on. This does not constitute a new user confirmation of audibility.

Actual Vulkan presentation captures at frames3000,4500 and6000 show the HUD
without the checker square. Frame6000 shows the upgraded dual Wave Guns and
their ammo display. Requested frame9000 is not reached before normal completion.
The aggregate evidence is `build/run/hud45_build_results.json`, with executable
hashes, build counts, weapon completion and SaveGame records. FPS shown in
validation captures is not a performance benchmark.

This corrects material registration; the other baseline Moon script error
involving `p_zom_moon_cassimir_plate` remains unresolved.
