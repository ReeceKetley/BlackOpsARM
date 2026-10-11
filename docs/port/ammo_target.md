# Ammo purchase measurement targets

Batch51 fixes three legacy test-plan entries: `ammo_pm63`, `ammo_rottweil`
and `ammo_m14`. Their `weapon` fields now identify the purchased gun.
This changes test harness bookkeeping; the scripts still perform purchases
and determine weapon/ammo results.

## Observed failure

`w32_water50_final_gate` finishes31PASS/1FAIL and exits0 after an actual
PM63 upgrade. It remains a failed gate. Its inserted PM63 ammo step begins
with the Death Machine raised and eventually makes109 use attempts before
timing out at423450ms. The actual refill occurs at384300ms: reserve10->80.

The old entries omitted `step.weapon`. Production `StartStep` and
`CheckUseSuccess` therefore measured the raised gun. The initial Death
Machine had376 rounds, so a real PM63 refill to100 could never exceed the
captured baseline. The other gun could also change during the step.

## Focused verification

```powershell
tools/port/check_ammo_target.ps1
tools/port/check_ammo_target.ps1 -NegativeControl
```

These tests extract the production step table, `StartStep` and
`CheckUseSuccess`. Both Win32 and x64 pass12 scenarios: purchased gun,
loaded Death Machine or empty other gun raised. Payment alone, another gun's
ammo increase and unpaid target increase reject; a paid target refill passes.
Pinned1fca062 fails baseline selection on both hosts.

Fresh five-step game checks on Win32/high (`qr wall_rottweil fire3 wall_m14
ammo_rottweil`) each finish5PASS/0FAIL/exit0. These demonstrate target-gun
measurement across a weapon change, but this route replaces the shotgun
with the M14, then purchases it again. They are not proof of a retained
shotgun refill with Death Machine active; that exact selection condition is
covered by the focused production-function test above.

All five builds compile0errors/no truncation warnings in recompiled files.
Final original routes and Vulkan validation are recorded below when complete.

## Final batch50/51 gameplay verification

Fresh corrected original routes Win32/high/low/guard finish24/25/25/25PASS,
all0FAIL/exit0 with all four SaveGame checks. These executables contain the
complete cache/wave/wind fixes and corrected ammo-test targets.
`x64hi_dxvk51_pane2_final` finishes33PASS/0FAIL/exit0, actual
glass2 shattering and Pack-a-Punch upgrade, then30seconds further play.
Sound is enabled; automation uses god mode. Pinned patched local DXVK,
system Vulkan/NVIDIA and explicit validation layers are recorded in the
module manifests. Presented-frame captures are retained. There are zero
Vulkan errors through teardown. All these test processes have ended.

The earlier Win32 water50 run remains31PASS/1FAIL/exit0, rejected for its
ammo-target measurement failure. `water50_results.json` retains all six
batch50 observations. `ammo51_results.json` retains both focused game runs
and all five corrected final gates; `water50_focused_results.json` retains
the retail wave/cache/skin and old-code control evidence. Focused ammo
measurement cases and their pinned controls are in `test_ammo_target*.log`.
These normal gameplay routes still do not certify full water-cell rendering.
WARN0010 remains open for scheduler/TrackView/material integration.
