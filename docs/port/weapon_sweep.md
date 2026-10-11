# Zombies weapon sweep - Windows x64

Generated from saved catalogs and runtime logs. A case counts as accepted only when it passed in a completed run with exit 0 and no emergency crash.

Coverage: Win32 **120/120**, high-address x64 **120/120**.

Plan: 120 cases, 119 distinct asset names, including 52 upgraded cases. Nacht's different Thundergun definition is a separate case.

Catalogs: 20/20; checked metadata differences: 0; Pack-a-Punch mapping differences: 0.

## Scope and limits

- Ten installed round-based Zombies maps. Shared definitions are exercised once per architecture on Pentagon; map-exclusive or numerically different definitions are exercised on their respective map. Every map has a separate Win32/x64 catalog comparison.
- Real rendered client, sound enabled, automated user commands, god mode. Each case lasts 18 game seconds and checks activation plus ADS, reload and switching where applicable. Dual weapons and alternate fire are separate cases where registered.
- Upgraded weapons are granted directly with retail gold camo. This does not test the Pack-a-Punch machine for every base weapon. The earlier Pentagon route tested the machine separately.
- Actor damage/death counters are observations during a case, not guaranteed weapon-specific attribution. These checks do not prove every special effect, revive effect, collision or multiplayer interaction correct. No valid visual screenshots were collected.
- Dead Ops Arcade (zombietron) is a separate mode without Pack-a-Punch. It reaches an unsupported server GetDvarVector function on both architectures; gameplay is not covered. Its native flame-table loader failure was repaired.
- Scripted items, perk bottles, non-player assets and internal projectile/left-hand components are excluded from independent cases; parent weapons exercise the components.

## Cases

| Map | Weapon asset | Win32 | High-address x64 |
| --- | --- | --- | --- |
| zombie_pentagon | `ak47_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `m1911_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `frag_grenade_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `claymore_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `m1911_upgraded_zm` | PASS (`w32_weapsweep39_cleanup`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `python_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `python_upgraded_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `cz75_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `cz75_upgraded_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `m14_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `m14_upgraded_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `m16_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `m16_gl_upgraded_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `gl_m16_upgraded_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `g11_lps_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `g11_lps_upgraded_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `famas_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `famas_upgraded_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `ak74u_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `ak74u_upgraded_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `mp5k_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `mp5k_upgraded_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `mpl_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `mpl_upgraded_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `pm63_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `pm63_upgraded_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `spectre_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `spectre_upgraded_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `cz75dw_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `cz75dw_upgraded_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `ithaca_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `ithaca_upgraded_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `rottweil72_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `rottweil72_upgraded_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `spas_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `spas_upgraded_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `hs10_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `hs10_upgraded_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `aug_acog_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `aug_acog_mk_upgraded_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `mk_aug_upgraded_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `galil_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `galil_upgraded_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `commando_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `commando_upgraded_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `fnfal_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `fnfal_upgraded_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `dragunov_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `dragunov_upgraded_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `l96a1_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `l96a1_upgraded_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `rpk_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `rpk_upgraded_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `hk21_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `hk21_upgraded_zm` | PASS (`w32_weapsweep39_cleanup`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `m72_law_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `m72_law_upgraded_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `china_lake_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `china_lake_upgraded_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `zombie_cymbal_monkey` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `ray_gun_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `ray_gun_upgraded_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `freezegun_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `freezegun_upgraded_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `crossbow_explosive_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `crossbow_explosive_upgraded_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `knife_ballistic_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `knife_ballistic_upgraded_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `knife_ballistic_bowie_zm` | PASS (`w32_weapsweep40_ballistic_bowie`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `knife_ballistic_bowie_upgraded_zm` | PASS (`w32_weapsweep40_ballistic_bowie`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `minigun_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `knife_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_pentagon | `bowie_knife_zm` | PASS (`w32_weapsweep38_zombie_pentagon`) | PASS (`x64hi_weapsweep40_zombie_pentagon`) |
| zombie_theater | `mp40_zm` | PASS (`w32_weapsweep34_zombie_theater`) | PASS (`x64hi_weapsweep34_zombie_theater`) |
| zombie_theater | `mp40_upgraded_zm` | PASS (`w32_weapsweep34_zombie_theater`) | PASS (`x64hi_weapsweep34_zombie_theater`) |
| zombie_theater | `thundergun_zm` | PASS (`w32_weapsweep34_zombie_theater`) | PASS (`x64hi_weapsweep34_zombie_theater`) |
| zombie_theater | `thundergun_upgraded_zm` | PASS (`w32_weapsweep34_zombie_theater`) | PASS (`x64hi_weapsweep34_zombie_theater`) |
| zombie_cosmodrome | `zombie_black_hole_bomb` | PASS (`w32_weapsweep34_zombie_cosmodrome`) | PASS (`x64hi_weapsweep34_zombie_cosmodrome`) |
| zombie_cosmodrome | `zombie_nesting_dolls` | PASS (`w32_weapsweep34_zombie_cosmodrome`) | PASS (`x64hi_weapsweep34_zombie_cosmodrome`) |
| zombie_cosmodrome | `knife_ballistic_sickle_zm` | PASS (`w32_weapsweep34_zombie_cosmodrome`) | PASS (`x64hi_weapsweep34_zombie_cosmodrome`) |
| zombie_cosmodrome | `knife_ballistic_sickle_upgraded_zm` | PASS (`w32_weapsweep34_zombie_cosmodrome`) | PASS (`x64hi_weapsweep34_zombie_cosmodrome`) |
| zombie_cosmodrome | `sickle_knife_zm` | PASS (`w32_weapsweep34_zombie_cosmodrome`) | PASS (`x64hi_weapsweep34_zombie_cosmodrome`) |
| zombie_coast | `sticky_grenade_zm` | PASS (`w32_weapsweep37_zombie_coast_sticky`) | PASS (`x64hi_weapsweep36_zombie_coast`) |
| zombie_coast | `humangun_zm` | PASS (`w32_weapsweep34_zombie_coast`) | PASS (`x64hi_weapsweep36_zombie_coast`) |
| zombie_coast | `humangun_upgraded_zm` | PASS (`w32_weapsweep34_zombie_coast`) | PASS (`x64hi_weapsweep36_zombie_coast`) |
| zombie_coast | `sniper_explosive_zm` | PASS (`w32_weapsweep34_zombie_coast`) | PASS (`x64hi_weapsweep36_zombie_coast`) |
| zombie_coast | `sniper_explosive_upgraded_zm` | PASS (`w32_weapsweep34_zombie_coast`) | PASS (`x64hi_weapsweep36_zombie_coast`) |
| zombie_coast | `tesla_gun_zm` | PASS (`w32_weapsweep34_zombie_coast`) | PASS (`x64hi_weapsweep36_zombie_coast`) |
| zombie_temple | `shrink_ray_zm` | PASS (`w32_weapsweep37_zombie_temple`) | PASS (`x64hi_weapsweep40_zombie_temple`) |
| zombie_temple | `shrink_ray_upgraded_zm` | PASS (`w32_weapsweep37_zombie_temple`) | PASS (`x64hi_weapsweep40_zombie_temple`) |
| zombie_moon | `zombie_quantum_bomb` | PASS (`w32_weapsweep34_zombie_moon`) | PASS (`x64hi_weapsweep34_zombie_moon`) |
| zombie_moon | `microwavegundw_zm` | PASS (`w32_weapsweep34_zombie_moon`) | PASS (`x64hi_weapsweep34_zombie_moon`) |
| zombie_moon | `microwavegun_zm` | PASS (`w32_weapsweep34_zombie_moon`) | PASS (`x64hi_weapsweep34_zombie_moon`) |
| zombie_moon | `microwavegundw_upgraded_zm` | PASS (`w32_weapsweep34_zombie_moon`) | PASS (`x64hi_weapsweep34_zombie_moon`) |
| zombie_moon | `microwavegun_upgraded_zm` | PASS (`w32_weapsweep34_zombie_moon`) | PASS (`x64hi_weapsweep34_zombie_moon`) |
| zombie_cod5_prototype | `zombie_m1carbine` | PASS (`w32_weapsweep34_zombie_cod5_prototype`) | PASS (`x64hi_weapsweep34_zombie_cod5_prototype`) |
| zombie_cod5_prototype | `zombie_thompson` | PASS (`w32_weapsweep34_zombie_cod5_prototype`) | PASS (`x64hi_weapsweep34_zombie_cod5_prototype`) |
| zombie_cod5_prototype | `zombie_kar98k` | PASS (`w32_weapsweep34_zombie_cod5_prototype`) | PASS (`x64hi_weapsweep34_zombie_cod5_prototype`) |
| zombie_cod5_prototype | `zombie_doublebarrel` | PASS (`w32_weapsweep34_zombie_cod5_prototype`) | PASS (`x64hi_weapsweep34_zombie_cod5_prototype`) |
| zombie_cod5_prototype | `zombie_doublebarrel_sawed` | PASS (`w32_weapsweep34_zombie_cod5_prototype`) | PASS (`x64hi_weapsweep34_zombie_cod5_prototype`) |
| zombie_cod5_prototype | `zombie_shotgun` | PASS (`w32_weapsweep34_zombie_cod5_prototype`) | PASS (`x64hi_weapsweep34_zombie_cod5_prototype`) |
| zombie_cod5_prototype | `zombie_bar` | PASS (`w32_weapsweep34_zombie_cod5_prototype`) | PASS (`x64hi_weapsweep34_zombie_cod5_prototype`) |
| zombie_cod5_prototype | `thundergun_zm` | PASS (`w32_weapsweep34_zombie_cod5_prototype`) | PASS (`x64hi_weapsweep34_zombie_cod5_prototype`) |
| zombie_cod5_asylum | `zombie_gewehr43` | PASS (`w32_weapsweep34_zombie_cod5_asylum`) | PASS (`x64hi_weapsweep34_zombie_cod5_asylum`) |
| zombie_cod5_asylum | `zombie_m1garand` | PASS (`w32_weapsweep34_zombie_cod5_asylum`) | PASS (`x64hi_weapsweep34_zombie_cod5_asylum`) |
| zombie_cod5_asylum | `zombie_bar_bipod` | PASS (`w32_weapsweep34_zombie_cod5_asylum`) | PASS (`x64hi_weapsweep34_zombie_cod5_asylum`) |
| zombie_cod5_asylum | `zombie_stg44` | PASS (`w32_weapsweep34_zombie_cod5_asylum`) | PASS (`x64hi_weapsweep34_zombie_cod5_asylum`) |
| zombie_cod5_sumpf | `zombie_type99_rifle` | PASS (`w32_weapsweep34_zombie_cod5_sumpf`) | PASS (`x64hi_weapsweep34_zombie_cod5_sumpf`) |
| zombie_cod5_sumpf | `zombie_type100_smg` | PASS (`w32_weapsweep34_zombie_cod5_sumpf`) | PASS (`x64hi_weapsweep34_zombie_cod5_sumpf`) |
| zombie_cod5_factory | `zombie_kar98k_upgraded` | PASS (`w32_weapsweep34_zombie_cod5_factory`) | PASS (`x64hi_weapsweep36_zombie_cod5_factory`) |
| zombie_cod5_factory | `zombie_m1carbine_upgraded` | PASS (`w32_weapsweep34_zombie_cod5_factory`) | PASS (`x64hi_weapsweep36_zombie_cod5_factory`) |
| zombie_cod5_factory | `zombie_gewehr43_upgraded` | PASS (`w32_weapsweep34_zombie_cod5_factory`) | PASS (`x64hi_weapsweep36_zombie_cod5_factory`) |
| zombie_cod5_factory | `zombie_stg44_upgraded` | PASS (`w32_weapsweep34_zombie_cod5_factory`) | PASS (`x64hi_weapsweep36_zombie_cod5_factory`) |
| zombie_cod5_factory | `zombie_thompson_upgraded` | PASS (`w32_weapsweep34_zombie_cod5_factory`) | PASS (`x64hi_weapsweep36_zombie_cod5_factory`) |
| zombie_cod5_factory | `zombie_type100_smg_upgraded` | PASS (`w32_weapsweep34_zombie_cod5_factory`) | PASS (`x64hi_weapsweep36_zombie_cod5_factory`) |
| zombie_cod5_factory | `zombie_doublebarrel_upgraded` | PASS (`w32_weapsweep34_zombie_cod5_factory`) | PASS (`x64hi_weapsweep36_zombie_cod5_factory`) |
| zombie_cod5_factory | `zombie_shotgun_upgraded` | PASS (`w32_weapsweep34_zombie_cod5_factory`) | PASS (`x64hi_weapsweep36_zombie_cod5_factory`) |
| zombie_cod5_factory | `zombie_fg42` | PASS (`w32_weapsweep34_zombie_cod5_factory`) | PASS (`x64hi_weapsweep36_zombie_cod5_factory`) |
| zombie_cod5_factory | `zombie_fg42_upgraded` | PASS (`w32_weapsweep34_zombie_cod5_factory`) | PASS (`x64hi_weapsweep36_zombie_cod5_factory`) |
| zombie_cod5_factory | `tesla_gun_upgraded_zm` | PASS (`w32_weapsweep34_zombie_cod5_factory`) | PASS (`x64hi_weapsweep36_zombie_cod5_factory`) |

## Retained failed cases

- `w32_weapsweep34_zombie_coast`: `sticky_grenade_zm`.

## Incomplete or crashed sweep runs

- `w32_weapsweep34_zombie_pentagon`: exit `-1`, completed=False, crash=False.
- `w32_weapsweep34_zombie_temple`: exit `-1`, completed=False, crash=True.
- `x64hi_weapsweep34_zombie_coast`: exit `-1073740972`, completed=False, crash=True.
- `x64hi_weapsweep34_zombie_cod5_factory`: exit `-1`, completed=False, crash=False.
- `x64hi_weapsweep34_zombie_pentagon`: exit `-1`, completed=False, crash=True.
- `x64hi_weapsweep38_zombie_pentagon`: exit `-1`, completed=False, crash=True.
