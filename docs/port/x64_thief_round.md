# Pentagon thief round verification (2026-10-10)

## Scope

Headless Windows x64 gameplay; rendering and Android are deferred. Tests use the existing fighting test client, god mode, and injected points. The retail dev-menu `thief_round` command starts the round after normal purchases/elevator traversal and power activation. The thief is spawned, animated, and controlled by the stock game scripts.

Plan: `qr wall_rottweil fire3 ammo_rottweil door_conf debris_elev2 floor2 debris_stair debris_elev1 battery floor3 power devgui_thief wait110`.

Common flags: `bo1_headless 1`, `bo1_zombies 1`, `developer_script 1`, `bo1_testclient 1`, `bo1_testclient_fight 1`, `bo1_testclient_god 1`, `bo1_testclient_score 10000`, `bo1_testclient_quitdone 5000`, `bo1_autoquit 480000`, `QuitOnError 1`, `logfile 2`, `devmap zombie_pentagon`. Processes were launched hidden.

## Steal, kill and recovery

| Event (level time in ms) | Guard x64 | Win32 |
|---|---:|---:|
| Thief round starts | 97900 | 98900 |
| Rottweil removed by retail steal | 103200 | 104200 |
| Thief enters exiting state | 104650 | 105650 |
| Retail script returns Rottweil | 104700 | 105700 |
| Thief finishes, last_thief_down = 1 | 105650 | 106650 |
| Thief round ends | 110900 | 111900 |
| Following round starts | 123400 | 124400 |
| Clean exit (code 0) | 226850 | 227850 |

`bo1_testclient_thiefkill 1` applies labelled player lethal damage only once the thief has stolen and entered `exiting`. This tests `thief_zombie_die` and `thief_return_loot`; it does not establish that ordinary gunfire can kill the thief reliably or validate balance. The steal itself occurs normally. The stolen-weapon animation item is removed, the Rottweil returns to the inventory, and normal ammo/powerup processing follows.

Both runs logged 14 requested step PASS results, zero FAIL, zero headless assertions/crashes, and zero script runtime errors. The harness inserts a wall AK74u rearm step after the thief; that inserted purchase does not produce an extra `end PASS` record.

Evidence:

- `build/run/x64_thief_recover/thief_verified.log`
- `build/run/w32_thief_recover/thief_verified.log`

## Escape without loot recovery

Plain x64: `bo1_testclient_thiefkill 0`, `bo1_testclient_holdfire 90000`. The bot stops firing before the thief spawns. The thief started at 96900 (round 4, health 2400), stole the Rottweil at 104800, entered `exiting` at 107050, and escaped at 145800 with health still 2400. `last_thief_down` became 1, and the player's inventory still lacked the Rottweil at the escape event. Final cleanup/exit results follow below.

Escape run completed: the round after the escape ended at 151900 (end_of_round, round 4), round 5 started at 164400, and `bo1_autoquit` fired at level time 484350. The log then shows every fastfile unloaded and its PMem freed, with no `Com_Error`, assertion, crash or script runtime error. Evidence: `build/run/x64_thief_escape/main/console_mp.log`.

## Changes

No engine or harness source changes were needed for the tested thief paths. Runtime output and reference comparison support the existing x64 fixes. This is headless functional coverage; visual animation quality, audio, and player-operated movement are outside this test.

## Perks, Pack-a-Punch and mystery box (2026-10-10)

Headless, god mode, same plan on plain x64 (`build/run/x64_cover1`) and Win32 (`w32_cover1`): `qr wall_rottweil fire3 ammo_rottweil door_conf debris_elev2 floor2 debris_stair debris_elev1 battery floor3 power jugg speedcola doubletap box pap`. Steps 0-17 (Quick Revive, Rottweil, door, debris, elevators, battery, power, Juggernog 250 health, Speed Cola, Double Tap) PASS on both with the same entity numbers and costs. `defcon` x4, `portal_pack` and `pap` PASS on both; Pack-a-Punch produced `ak74u_upgraded_zm` (x64) and `m1911_upgraded_zm` (Win32). Both exit 0.

The `box` step timed out on both builds at the same level time (277150): the harness stalled short of the chest in a room behind Pentagon's west debris and lab doors, so that is a plan-order limit, not an x64 difference. With `debris_west door_lab1 door_lab2 door_lab3` bought first (`x64_box`, `w32_box`, `bo1_testclient_score 20000`), `box`, `box_leave` and `box` again PASS on both builds (x64 rolled spas_zm, Win32 hk21_zm then china_lake_zm; the roll is random). No script runtime errors, assertions or crashes in any of the four logs.


## Non-god soak: death, game over, map restart, and an LZO bug (2026-10-10)

Without god mode and with only a pistol (`x64_mortal`, `w32_mortal`, 900 s) both builds reach round 4-5, the bot dies, `mus_zombie_game_over` plays, `SV_MapRestart(1)` runs and round 1 starts again; this repeated three times on each build with a clean shutdown and no script errors. Single-player has no revive partner, so revive-by-teammate is not covered.

The startup level-start save has a read-back check (`SaveGame check:` in `g_sp_savegame.cpp`). On Win32 it logs `identical` (11573 bytes, 281 dvars, 18 hud elems). On x64 it logged `DIFFERENT` (272 bytes, 1 dvar), `Could not load weapon` with a garbage name, and a nonsense segment-4 header. Cause: `memfile.cpp` passed 4-byte `int`/slot addresses to miniLZO as `lzo_uint*`, but `lzo_uint` is `size_t` (8 bytes) on x64. The compressor overwrote the first 4 bytes of each compressed block with the high half of the length, and the decompressor overran its `int len`. `lzo_init` also passed Win32 `sizeof` literals. Fix: real `lzo_uint` locals copied into the 4-byte slots, and `lzo_init()` (sizeof-based). After the fix x64 prints the same four `SaveGame check` lines as Win32.

Regression after the LZO fix (all rebuilt): plain x64 24 PASS / 0 FAIL, Win32 23 PASS / 0 FAIL, guard x64 26 PASS / 0 FAIL (first guard run hit its 300 s limit mid-route because the guard heap is slower and a `portal_pack` recovery step ran before the defcon puzzle had opened the hideaway; with a 700 s limit it completed `pap` -> `m14_upgraded_zm`). Route: `qr wall_rottweil fire3 ammo_rottweil door_conf debris_elev2 floor2 debris_stair debris_elev1 battery floor3 power jugg speedcola doubletap pap` (the defcon/portal recovery steps are inserted by the harness). All three print identical `SaveGame check` lines and exit 0 with zero script errors, assertions or crashes. Runs: `build/run/{x64_lzo2,w32_lzo2,x64guard_lzo}`.

## 30-minute god-mode soak (2026-10-10)

`x64_long2` and `w32_long2`: the 15-step route (`qr` ... `doubletap`), then fighting with `bo1_testclient_dryammo 20000` and `bo1_testclient_resupply 1`, `bo1_autoquit 1800000`. Plain x64 started round 21 (level time 1797400) and Win32 round 22 (1771400); both reached the autoquit and exited 0 with zero script errors, assertions or crashes. Dog rounds (about 45 vs 47 dog log lines) and natural thief steals (3 vs 4 `zombie_gunstolen` gives) occurred on both. Round counts differ by one because the harness and AI timing are not deterministic.

An earlier pair (`x64_long`, `w32_long`, without resupply) stopped progressing on x64 at round 9: a natural thief took the bot's last pistol and the harness bot never re-armed, so zombies could not be killed. That is a harness limit (Win32 hit it less often only because it still held another gun), not an x64 difference.
