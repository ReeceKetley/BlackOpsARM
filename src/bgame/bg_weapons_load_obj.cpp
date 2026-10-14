#include <universal/csp_field_offset.h>
#if defined(__ANDROID__)
#include <platform/android/legacy_crt.h>
#endif
#include "bg_weapons_load_obj.h"
#include "bg_weapons.h"
#include "bg_animation.h"
#include <universal/com_loadutils.h>
#include <qcommon/common.h>
#include <universal/q_parse.h>
#include <universal/com_memory.h>
#include <universal/com_files.h>
#include "bg_weapons_attachment.h"
#include "bg_weapons_util.h"
#include <clientscript/cscr_stringlist.h>
#include <universal/surfaceflags.h>
#include <game/g_weapon_load_obj.h>
#include "bg_weapons_def.h"
#include <database/db_registry.h>

const char *szWeapOverlayReticleNames[2] =
{ "none", "crosshair" };

const char *penetrateTypeNames[4] =
{ "none", "small", "medium", "large" };

const char *impactTypeNames_0[16] =
{
  "none",
  "bullet_small",
  "bullet_large",
  "bullet_ap",
  "bullet_xtreme",
  "shotgun",
  "grenade_bounce",
  "grenade_explode",
  "rifle_grenade",
  "rocket_explode",
  "rocket_explode_xtreme",
  "projectile_dud",
  "mortar_shell",
  "tank_shell",
  "bolt",
  "blade"
};

const char *szWeapStanceNames[3] =
{ "stand", "duck", "prone" };

const char *szProjectileExplosionNames[10] =
{
  "grenade",
  "rocket",
  "flashbang",
  "none",
  "dud",
  "smoke",
  "heavy explosive",
  "fire",
  "napalmblob",
  "bolt"
};

const char *offhandClassNames[5] =
{ "None", "Frag Grenade", "Smoke Grenade", "Flash Grenade", "Gear" };

const char *offhandSlotNames[5] =
{ "None", "Lethal grenade", "Tactical grenade", "Equipment", "Specific use" };

const char *activeReticleNames[3] =
{ "None", "Pip-On-A-Stick", "Bouncing diamond" };

const char *guidedMissileNames[7] =
{
  "None",
  "Sidewinder",
  "Hellfire",
  "Javelin",
  "Ballistic",
  "WireGuided",
  "TVGuided"
};

const char *stickinessNames[6] =
{
  "Don't stick",
  "Stick to all",
  "Stick to all, except ai and clients",
  "Stick to ground",
  "Stick to ground, maintain yaw",
  "Stick to flesh"
};

const char *rotateTypeNames[3] =
{
  "Rotate both axis, grenade style",
  "Rotate one axis, blade style",
  "Rotate like a cylinder"
};

const char *overlayInterfaceNames[3] =
{ "None", "Javelin", "Turret Scope" };

const char *szWeapFireTypeNames[7] =
{
  "Full Auto",
  "Single Shot",
  "2-Round Burst",
  "3-Round Burst",
  "4-Round Burst",
  "Stacked Fire",
  "Minigun"
};

const char *szWeapClipTypeNames[6] =
{ "bottom", "top", "left", "dp28", "ptrs", "lmg" };

const char *ammoCounterClipNames[7] =
{
  "None",
  "Magazine",
  "ShortMagazine",
  "Shotgun",
  "Rocket",
  "Beltfed",
  "AltWeapon"
};

const char *weapIconRatioNames[3] =
{ "1:1", "2:1", "4:1" };

WeaponFullDef bg_defaultWeaponFullDefs;
SurfaceTypeSoundList surfaceTypeSoundLists[16];

char *s_weaponGripAnimSubstrings[64];

char *g_playerAnimTypeNames[32];
unsigned int g_playerAnimTypeNamesCount;

int g_playerAnimTypeIndex_revivee;
int g_playerAnimTypeIndex_sniper;
int g_playerAnimTypeIndex_sniper_rearclip;
int g_playerAnimTypeIndex_briefcase;

int surfaceTypeSoundListCount;
int s_numWeaponGripAnimSubstrings;

unsigned int bg_nextParseFlameTableIndex;

flameTable *bg_flameTables[8];

cspField_t weaponDefFields[748] =
{
  { "displayName", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.szDisplayName, 12), 0 },
  { "AIOverlayDescription", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.szOverlayName, 228), 0 },
  { "modeName", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.szModeName, 240), 0 },
  { "playerAnimType", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.playerAnimType, 252), 24 },
  { "gunModel", CSP_FIELD_OFFSET(WeaponFullDef, gunXModel[0], 2284), 10 },
  { "gunModel2", CSP_FIELD_OFFSET(WeaponFullDef, gunXModel[1], 2288), 10 },
  { "gunModel3", CSP_FIELD_OFFSET(WeaponFullDef, gunXModel[2], 2292), 10 },
  { "gunModel4", CSP_FIELD_OFFSET(WeaponFullDef, gunXModel[3], 2296), 10 },
  { "gunModel5", CSP_FIELD_OFFSET(WeaponFullDef, gunXModel[4], 2300), 10 },
  { "gunModel6", CSP_FIELD_OFFSET(WeaponFullDef, gunXModel[5], 2304), 10 },
  { "gunModel7", CSP_FIELD_OFFSET(WeaponFullDef, gunXModel[6], 2308), 10 },
  { "gunModel8", CSP_FIELD_OFFSET(WeaponFullDef, gunXModel[7], 2312), 10 },
  { "gunModel9", CSP_FIELD_OFFSET(WeaponFullDef, gunXModel[8], 2316), 10 },
  { "gunModel10", CSP_FIELD_OFFSET(WeaponFullDef, gunXModel[9], 2320), 10 },
  { "gunModel11", CSP_FIELD_OFFSET(WeaponFullDef, gunXModel[10], 2324), 10 },
  { "gunModel12", CSP_FIELD_OFFSET(WeaponFullDef, gunXModel[11], 2328), 10 },
  { "gunModel13", CSP_FIELD_OFFSET(WeaponFullDef, gunXModel[12], 2332), 10 },
  { "gunModel14", CSP_FIELD_OFFSET(WeaponFullDef, gunXModel[13], 2336), 10 },
  { "gunModel15", CSP_FIELD_OFFSET(WeaponFullDef, gunXModel[14], 2340), 10 },
  { "gunModel16", CSP_FIELD_OFFSET(WeaponFullDef, gunXModel[15], 2344), 10 },
  { "handModel", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.handXModel, 236), 10 },
  { "hideTags", CSP_FIELD_OFFSET(WeaponFullDef, hideTags[0], 2612), 40 },
  { "notetrackSoundMap", CSP_FIELD_OFFSET(WeaponFullDef, notetrackSoundMapKeys[0], 2676), 42 },
  { "idleAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[1], 2352), 0 },
  { "idleAnimLeft", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[60], 2588), 0 },
  { "emptyIdleAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[2], 2356), 0 },
  { "emptyIdleAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[61], 2592), 0 },
  { "fireAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[3], 2360), 0 },
  { "fireAnimLeft", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[58], 2580), 0 },
  { "holdFireAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[4], 2364), 0 },
  { "lastShotAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[5], 2368), 0 },
  { "lastShotAnimLeft", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[59], 2584), 0 },
  { "detonateAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[39], 2504), 0 },
  { "rechamberAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[6], 2372), 0 },
  { "meleeAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[7], 2376), 0 },
  { "meleeChargeAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[8], 2380), 0 },
  { "reloadAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[9], 2384), 0 },
  { "reloadAnimRight", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[10], 2388), 0 },
  { "reloadAnimLeft", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[63], 2600), 0 },
  { "reloadEmptyAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[11], 2392), 0 },
  { "reloadEmptyAnimLeft", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[62], 2596), 0 },
  { "reloadStartAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[12], 2396), 0 },
  { "reloadEndAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[13], 2400), 0 },
  { "reloadQuickAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[14], 2404), 0 },
  { "reloadQuickEmptyAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[15], 2408), 0 },
  { "raiseAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[16], 2412), 0 },
  { "dropAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[18], 2420), 0 },
  { "firstRaiseAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[17], 2416), 0 },
  { "altRaiseAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[19], 2424), 0 },
  { "altDropAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[20], 2428), 0 },
  { "quickRaiseAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[21], 2432), 0 },
  { "quickDropAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[22], 2436), 0 },
  { "emptyRaiseAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[23], 2440), 0 },
  { "emptyDropAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[24], 2444), 0 },
  { "sprintInAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[25], 2448), 0 },
  { "sprintLoopAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[26], 2452), 0 },
  { "sprintOutAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[27], 2456), 0 },
  { "sprintInEmptyAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[28], 2460), 0 },
  { "sprintLoopEmptyAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[29], 2464), 0 },
  { "sprintOutEmptyAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[30], 2468), 0 },
  { "lowReadyInAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[31], 2472), 0 },
  { "lowReadyLoopAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[32], 2476), 0 },
  { "lowReadyOutAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[33], 2480), 0 },
  { "contFireInAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[34], 2484), 0 },
  { "contFireLoopAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[35], 2488), 0 },
  { "contFireOutAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[36], 2492), 0 },
  { "deployAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[37], 2496), 0 },
  { "breakdownAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[38], 2500), 0 },
  { "nightVisionWearAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[40], 2508), 0 },
  { "nightVisionRemoveAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[41], 2512), 0 },
  { "adsFireAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[42], 2516), 0 },
  { "adsLastShotAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[43], 2520), 0 },
  { "adsRechamberAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[44], 2524), 0 },
  { "adsUpAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[64], 2604), 0 },
  { "adsDownAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[65], 2608), 0 },
  { "deployAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[37], 2496), 0 },
  { "breakdownAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[38], 2500), 0 },
  { "dtp_in", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[45], 2528), 0 },
  { "dtp_loop", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[46], 2532), 0 },
  { "dtp_out", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[47], 2536), 0 },
  { "dtp_empty_in", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[48], 2540), 0 },
  { "dtp_empty_loop", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[49], 2544), 0 },
  { "dtp_empty_out", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[50], 2548), 0 },
  { "slide_in", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[51], 2552), 0 },
  { "mantleAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[52], 2556), 0 },
  { "sprintCameraAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[53], 2560), 0 },
  { "dtpInCameraAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[54], 2564), 0 },
  { "dtpLoopCameraAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[55], 2568), 0 },
  { "dtpOutCameraAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[56], 2572), 0 },
  { "mantleCameraAnim", CSP_FIELD_OFFSET(WeaponFullDef, szXAnims[57], 2576), 0 },
  { "script", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.szScript, 2176), 0 },
  { "weaponType", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.weapType, 256), 15 },
  { "weaponClass", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.weapClass, 260), 16 },
  { "penetrateType", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.penetrateType, 264), 18 },
  { "impactType", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.impactType, 268), 19 },
  { "inventoryType", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.inventoryType, 272), 31 },
  { "fireType", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fireType, 276), 32 },
  { "clipType", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.clipType, 280), 33 },
  { "offhandClass", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.offhandClass, 332), 22 },
  { "offhandSlot", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.offhandSlot, 336), 23 },
  { "viewFlashEffect", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.viewFlashEffect, 344), 9 },
  { "worldFlashEffect", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.worldFlashEffect, 348), 9 },
  { "pickupSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.pickupSound, 352), 0 },
  { "pickupSoundPlayer", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.pickupSoundPlayer, 356), 0 },
  { "ammoPickupSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.ammoPickupSound, 360), 0 },
  { "ammoPickupSoundPlayer", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.ammoPickupSoundPlayer, 364), 0 },
  { "projectileSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.projectileSound, 368), 0 },
  { "pullbackSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.pullbackSound, 372), 0 },
  { "pullbackSoundPlayer", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.pullbackSoundPlayer, 376), 0 },
  { "fireSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fireSound, 380), 0 },
  { "crackSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.crackSound, 428), 0 },
  { "whizbySound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.whizbySound, 432), 0 },
  { "fireSoundPlayer", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fireSoundPlayer, 384), 0 },
  { "loopFireSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fireLoopSound, 388), 0 },
  { "loopFireSoundPlayer", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fireLoopSoundPlayer, 392), 0 },
  { "loopFireEndSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fireLoopEndSound, 396), 0 },
  { "loopFireEndSoundPlayer", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fireLoopEndSoundPlayer, 400), 0 },
  { "stopFireSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fireStopSound, 404), 0 },
  { "stopFireSoundPlayer", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fireStopSoundPlayer, 408), 0 },
  { "lastShotSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fireLastSound, 412), 0 },
  { "lastShotSoundPlayer", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fireLastSoundPlayer, 416), 0 },
  { "emptyFireSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.emptyFireSound, 420), 0 },
  { "emptyFireSoundPlayer", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.emptyFireSoundPlayer, 424), 0 },
  { "meleeSwipeSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.meleeSwipeSound, 436), 0 },
  { "meleeSwipeSoundPlayer", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.meleeSwipeSoundPlayer, 440), 0 },
  { "meleeHitSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.meleeHitSound, 444), 0 },
  { "meleeMissSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.meleeMissSound, 448), 0 },
  { "rechamberSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.rechamberSound, 452), 0 },
  { "rechamberSoundPlayer", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.rechamberSoundPlayer, 456), 0 },
  { "reloadSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.reloadSound, 460), 0 },
  { "reloadSoundPlayer", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.reloadSoundPlayer, 464), 0 },
  { "reloadEmptySound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.reloadEmptySound, 468), 0 },
  { "reloadEmptySoundPlayer", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.reloadEmptySoundPlayer, 472), 0 },
  { "reloadStartSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.reloadStartSound, 476), 0 },
  { "reloadStartSoundPlayer", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.reloadStartSoundPlayer, 480), 0 },
  { "reloadEndSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.reloadEndSound, 484), 0 },
  { "reloadEndSoundPlayer", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.reloadEndSoundPlayer, 488), 0 },
  { "rotateLoopSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.rotateLoopSound, 492), 0 },
  { "rotateLoopSoundPlayer", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.rotateLoopSoundPlayer, 496), 0 },
  { "deploySound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.deploySound, 500), 0 },
  { "deploySoundPlayer", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.deploySoundPlayer, 504), 0 },
  { "finishDeploySound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.finishDeploySound, 508), 0 },
  { "finishDeploySoundPlayer", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.finishDeploySoundPlayer, 512), 0 },
  { "breakdownSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.breakdownSound, 516), 0 },
  { "breakdownSoundPlayer", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.breakdownSoundPlayer, 520), 0 },
  { "finishBreakdownSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.finishBreakdownSound, 524), 0 },
  { "finishBreakdownSoundPlayer", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.finishBreakdownSoundPlayer, 528), 0 },
  { "detonateSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.detonateSound, 532), 0 },
  { "detonateSoundPlayer", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.detonateSoundPlayer, 536), 0 },
  { "nightVisionWearSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.nightVisionWearSound, 540), 0 },
  { "nightVisionWearSoundPlayer", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.nightVisionWearSoundPlayer, 544), 0 },
  { "nightVisionRemoveSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.nightVisionRemoveSound, 548), 0 },
  { "nightVisionRemoveSoundPlayer", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.nightVisionRemoveSoundPlayer, 552), 0 },
  { "raiseSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.raiseSound, 564), 0 },
  { "raiseSoundPlayer", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.raiseSoundPlayer, 568), 0 },
  { "firstRaiseSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.firstRaiseSound, 572), 0 },
  { "firstRaiseSoundPlayer", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.firstRaiseSoundPlayer, 576), 0 },
  { "altSwitchSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.altSwitchSound, 556), 0 },
  { "altSwitchSoundPlayer", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.altSwitchSoundPlayer, 560), 0 },
  { "putawaySound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.putawaySound, 580), 0 },
  { "putawaySoundPlayer", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.putawaySoundPlayer, 584), 0 },
  { "overheatSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.overheatSound, 588), 0 },
  { "overheatSoundPlayer", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.overheatSoundPlayer, 592), 0 },
  { "adsZoomSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.adsZoomSound, 596), 0 },
  { "bounceSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bounceSound, 600), 27 },
  { "standMountedWeapdef", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.standMountedWeapdef, 604), 0 },
  { "crouchMountedWeapdef", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.crouchMountedWeapdef, 608), 0 },
  { "proneMountedWeapdef", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.proneMountedWeapdef, 612), 0 },
  { "viewShellEjectEffect", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.viewShellEjectEffect, 628), 9 },
  { "worldShellEjectEffect", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.worldShellEjectEffect, 632), 9 },
  { "viewLastShotEjectEffect", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.viewLastShotEjectEffect, 636), 9 },
  { "worldLastShotEjectEffect", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.worldLastShotEjectEffect, 640), 9 },
  { "reticleCenter", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.reticleCenter, 644), 11 },
  { "reticleSide", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.reticleSide, 648), 11 },
  { "reticleCenterSize", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iReticleCenterSize, 652), 4 },
  { "reticleSideSize", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iReticleSideSize, 656), 4 },
  { "reticleMinOfs", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iReticleMinOfs, 660), 4 },
  { "activeReticleType", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.activeReticleType, 664), 25 },
  { "standMoveF", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vStandMove[0], 668), 7 },
  { "standMoveR", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vStandMove[1], 672), 7 },
  { "standMoveU", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vStandMove[2], 676), 7 },
  { "standRotP", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vStandRot[0], 680), 7 },
  { "standRotY", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vStandRot[1], 684), 7 },
  { "standRotR", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vStandRot[2], 688), 7 },
  { "duckedOfsF", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vDuckedOfs[0], 692), 7 },
  { "duckedOfsR", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vDuckedOfs[1], 696), 7 },
  { "duckedOfsU", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vDuckedOfs[2], 700), 7 },
  { "duckedMoveF", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vDuckedMove[0], 704), 7 },
  { "duckedMoveR", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vDuckedMove[1], 708), 7 },
  { "duckedMoveU", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vDuckedMove[2], 712), 7 },
  { "duckedSprintOfsF", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vDuckedSprintOfs[0], 716), 7 },
  { "duckedSprintOfsR", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vDuckedSprintOfs[1], 720), 7 },
  { "duckedSprintOfsU", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vDuckedSprintOfs[2], 724), 7 },
  { "duckedSprintRotP", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vDuckedSprintRot[0], 728), 7 },
  { "duckedSprintRotY", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vDuckedSprintRot[1], 732), 7 },
  { "duckedSprintRotR", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vDuckedSprintRot[2], 736), 7 },
  { "duckedSprintBobH", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vDuckedSprintBob[0], 740), 7 },
  { "duckedSprintBobV", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vDuckedSprintBob[1], 744), 7 },
  { "duckedSprintScale", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fDuckedSprintCycleScale, 748), 7 },
  { "sprintOfsF", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vSprintOfs[0], 752), 7 },
  { "sprintOfsR", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vSprintOfs[1], 756), 7 },
  { "sprintOfsU", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vSprintOfs[2], 760), 7 },
  { "sprintRotP", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vSprintRot[0], 764), 7 },
  { "sprintRotY", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vSprintRot[1], 768), 7 },
  { "sprintRotR", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vSprintRot[2], 772), 7 },
  { "sprintBobH", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vSprintBob[0], 776), 7 },
  { "sprintBobV", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vSprintBob[1], 780), 7 },
  { "sprintScale", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fSprintCycleScale, 784), 7 },
  { "lowReadyOfsF", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vLowReadyOfs[0], 788), 7 },
  { "lowReadyOfsR", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vLowReadyOfs[1], 792), 7 },
  { "lowReadyOfsU", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vLowReadyOfs[2], 796), 7 },
  { "lowReadyRotP", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vLowReadyRot[0], 800), 7 },
  { "lowReadyRotY", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vLowReadyRot[1], 804), 7 },
  { "lowReadyRotR", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vLowReadyRot[2], 808), 7 },
  { "dtpOfsF", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vDtpOfs[0], 812), 7 },
  { "dtpOfsR", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vDtpOfs[1], 816), 7 },
  { "dtpOfsU", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vDtpOfs[2], 820), 7 },
  { "dtpRotP", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vDtpRot[0], 824), 7 },
  { "dtpRotY", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vDtpRot[1], 828), 7 },
  { "dtpRotR", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vDtpRot[2], 832), 7 },
  { "dtpBobH", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vDtpBob[0], 836), 7 },
  { "dtpBobV", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vDtpBob[1], 840), 7 },
  { "dtpScale", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fDtpCycleScale, 844), 7 },
  { "mantleOfsF", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vMantleOfs[0], 848), 7 },
  { "mantleOfsR", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vMantleOfs[1], 852), 7 },
  { "mantleOfsU", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vMantleOfs[2], 856), 7 },
  { "mantleRotP", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vMantleRot[0], 860), 7 },
  { "mantleRotY", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vMantleRot[1], 864), 7 },
  { "mantleRotR", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vMantleRot[2], 868), 7 },
  { "slideOfsF", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vSlideOfs[0], 872), 7 },
  { "slideOfsR", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vSlideOfs[1], 876), 7 },
  { "slideOfsU", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vSlideOfs[2], 880), 7 },
  { "slideRotP", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vSlideRot[0], 884), 7 },
  { "slideRotY", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vSlideRot[1], 888), 7 },
  { "slideRotR", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vSlideRot[2], 892), 7 },
  { "duckedRotP", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vDuckedRot[0], 896), 7 },
  { "duckedRotY", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vDuckedRot[1], 900), 7 },
  { "duckedRotR", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vDuckedRot[2], 904), 7 },
  { "proneOfsF", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vProneOfs[0], 908), 7 },
  { "proneOfsR", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vProneOfs[1], 912), 7 },
  { "proneOfsU", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vProneOfs[2], 916), 7 },
  { "proneMoveF", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vProneMove[0], 920), 7 },
  { "proneMoveR", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vProneMove[1], 924), 7 },
  { "proneMoveU", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vProneMove[2], 928), 7 },
  { "proneRotP", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vProneRot[0], 932), 7 },
  { "proneRotY", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vProneRot[1], 936), 7 },
  { "proneRotR", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vProneRot[2], 940), 7 },
  { "strafeMoveF", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vStrafeMove[0], 944), 7 },
  { "strafeMoveR", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vStrafeMove[1], 948), 7 },
  { "strafeMoveU", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vStrafeMove[2], 952), 7 },
  { "strafeRotP", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vStrafeRot[0], 956), 7 },
  { "strafeRotY", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vStrafeRot[1], 960), 7 },
  { "strafeRotR", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vStrafeRot[2], 964), 7 },
  { "posMoveRate", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fPosMoveRate, 968), 7 },
  { "posProneMoveRate", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fPosProneMoveRate, 972), 7 },
  { "standMoveMinSpeed", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fStandMoveMinSpeed, 976), 7 },
  { "duckedMoveMinSpeed", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fDuckedMoveMinSpeed, 980), 7 },
  { "proneMoveMinSpeed", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fProneMoveMinSpeed, 984), 7 },
  { "posRotRate", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fPosRotRate, 988), 7 },
  { "posProneRotRate", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fPosProneRotRate, 992), 7 },
  { "standRotMinSpeed", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fStandRotMinSpeed, 996), 7 },
  { "duckedRotMinSpeed", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fDuckedRotMinSpeed, 1000), 7 },
  { "proneRotMinSpeed", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fProneRotMinSpeed, 1004), 7 },
  { "worldModel", CSP_FIELD_OFFSET(WeaponFullDef, worldModel[0], 2756), 10 },
  { "worldModel2", CSP_FIELD_OFFSET(WeaponFullDef, worldModel[1], 2760), 10 },
  { "worldModel3", CSP_FIELD_OFFSET(WeaponFullDef, worldModel[2], 2764), 10 },
  { "worldModel4", CSP_FIELD_OFFSET(WeaponFullDef, worldModel[3], 2768), 10 },
  { "worldModel5", CSP_FIELD_OFFSET(WeaponFullDef, worldModel[4], 2772), 10 },
  { "worldModel6", CSP_FIELD_OFFSET(WeaponFullDef, worldModel[5], 2776), 10 },
  { "worldModel7", CSP_FIELD_OFFSET(WeaponFullDef, worldModel[6], 2780), 10 },
  { "worldModel8", CSP_FIELD_OFFSET(WeaponFullDef, worldModel[7], 2784), 10 },
  { "worldModel9", CSP_FIELD_OFFSET(WeaponFullDef, worldModel[8], 2788), 10 },
  { "worldModel10", CSP_FIELD_OFFSET(WeaponFullDef, worldModel[9], 2792), 10 },
  { "worldModel11", CSP_FIELD_OFFSET(WeaponFullDef, worldModel[10], 2796), 10 },
  { "worldModel12", CSP_FIELD_OFFSET(WeaponFullDef, worldModel[11], 2800), 10 },
  { "worldModel13", CSP_FIELD_OFFSET(WeaponFullDef, worldModel[12], 2804), 10 },
  { "worldModel14", CSP_FIELD_OFFSET(WeaponFullDef, worldModel[13], 2808), 10 },
  { "worldModel15", CSP_FIELD_OFFSET(WeaponFullDef, worldModel[14], 2812), 10 },
  { "worldModel16", CSP_FIELD_OFFSET(WeaponFullDef, worldModel[15], 2816), 10 },
  { "worldClipModel", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.worldClipModel, 1012), 10 },
  { "rocketModel", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.rocketModel, 1016), 10 },
  { "mountedModel", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.mountedModel, 1020), 10 },
  { "AdditionalMeleeModel", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.additionalMeleeModel, 1024), 10 },
  { "hudIcon", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.hudIcon, 1028), 11 },
  { "hudIconRatio", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.hudIconRatio, 1032), 35 },
  { "indicatorIcon", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.indicatorIcon, 1036), 11 },
  { "indicatorIconRatio", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.indicatorIconRatio, 1040), 39 },
  { "ammoCounterIcon", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.ammoCounterIcon, 1044), 11 },
  { "ammoCounterIconRatio", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.ammoCounterIconRatio, 1048), 36 },
  { "ammoCounterClip", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.ammoCounterClip, 1052), 34 },
  { "startAmmo", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iStartAmmo, 1056), 4 },
  { "ammoName", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.szAmmoName, 64), 0 },
  { "clipName", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.szClipName, 72), 0 },
  { "maxAmmo", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iMaxAmmo, 1064), 4 },
  { "clipSize", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.iClipSize, 32), 4 },
  { "shotCount", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.shotCount, 1068), 4 },
  { "sharedAmmoCapName", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.szSharedAmmoCapName, 1072), 0 },
  { "sharedAmmoCap", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iSharedAmmoCap, 1080), 4 },
  { "unlimitedAmmo", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.unlimitedAmmo, 1084), 5 },
  { "ammoCountClipRelative", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.ammoCountClipRelative, 1085), 5 },
  { "sharedAmmo", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.sharedAmmo, 1584), 5 },
  { "jamFireTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iJamFireTime, 292), 8 },
  { "overheatWeapon", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.overheatWeapon, 308), 4 },
  { "overheatRate", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.overheatRate, 312), 7 },
  { "cooldownRate", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.cooldownRate, 316), 7 },
  { "overheatEndVal", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.overheatEndVal, 320), 7 },
  { "coolWhileFiring", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.coolWhileFiring, 324), 4 },
  { "fuelTankWeapon", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fuelTankWeapon, 325), 4 },
  { "tankLifeTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iTankLifeTime, 328), 8 },
  { "damage", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.damage, 1088), 4 },
  { "damageDuration", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.damageDuration, 1092), 7 },
  { "damageInterval", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.damageInterval, 1096), 7 },
  { "playerDamage", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.playerDamage, 1100), 4 },
  { "meleeDamage", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iMeleeDamage, 1104), 4 },
  { "minDamage", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.minDamage, 2180), 4 },
  { "minPlayerDamage", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.minPlayerDamage, 2184), 4 },
  { "maxDamageRange", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fMaxDamageRange, 2188), 7 },
  { "minDamageRange", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fMinDamageRange, 2192), 7 },
  { "destabilizationRateTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.destabilizationRateTime, 2196), 7 },
  { "destabilizationCurvatureMax", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.destabilizationCurvatureMax, 2200), 7 },
  { "destabilizeDistance", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.destabilizeDistance, 2204), 4 },
  { "fireDelay", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iFireDelay, 1116), 8 },
  { "meleeDelay", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iMeleeDelay, 1120), 8 },
  { "meleeChargeDelay", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.meleeChargeDelay, 1124), 8 },
  { "spinUpTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iSpinUpTime, 1132), 8 },
  { "spinDownTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iSpinDownTime, 1136), 8 },
  { "spinRate", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.spinRate, 1140), 7 },
  { "spinLoopSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.spinLoopSound, 1144), 0 },
  { "spinLoopSoundPlayer", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.spinLoopSoundPlayer, 1148), 0 },
  { "startSpinSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.startSpinSound, 1152), 0 },
  { "startSpinSoundPlayer", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.startSpinSoundPlayer, 1156), 0 },
  { "stopSpinSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.stopSpinSound, 1160), 0 },
  { "stopSpinSoundPlayer", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.stopSpinSoundPlayer, 1164), 0 },
  { "fireTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iFireTime, 1168), 8 },
  { "lastFireTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iLastFireTime, 1172), 8 },
  { "rechamberTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iRechamberTime, 1176), 8 },
  { "rechamberBoltTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iRechamberBoltTime, 1180), 8 },
  { "holdFireTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iHoldFireTime, 1184), 8 },
  { "detonateTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iDetonateTime, 1188), 8 },
  { "detonateDelay", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iDetonateDelay, 1128), 8 },
  { "meleeTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iMeleeTime, 1192), 8 },
  { "meleeChargeTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.meleeChargeTime, 1196), 8 },
  { "reloadTime", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.iReloadTime, 36), 8 },
  { "reloadShowRocketTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.reloadShowRocketTime, 1208), 8 },
  { "reloadEmptyTime", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.iReloadEmptyTime, 40), 8 },
  { "reloadAddTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iReloadAddTime, 1216), 8 },
  { "reloadEmptyAddTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iReloadEmptyAddTime, 1220), 8 },
  { "reloadQuickAddTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iReloadQuickAddTime, 1224), 8 },
  { "reloadQuickEmptyAddTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iReloadQuickEmptyAddTime, 1228), 8 },
  { "reloadStartTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iReloadStartTime, 1232), 8 },
  { "reloadStartAddTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iReloadStartAddTime, 1236), 8 },
  { "reloadEndTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iReloadEndTime, 1240), 8 },
  { "reloadQuickTime", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.iReloadQuickTime, 44), 8 },
  { "reloadQuickEmptyTime", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.iReloadQuickEmptyTime, 48), 8 },
  { "dropTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iDropTime, 1244), 8 },
  { "raiseTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iRaiseTime, 1248), 8 },
  { "altDropTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iAltDropTime, 1252), 8 },
  { "altRaiseTime", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.iAltRaiseTime, 60), 8 },
  { "quickDropTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.quickDropTime, 1256), 8 },
  { "quickRaiseTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.quickRaiseTime, 1260), 8 },
  { "firstRaiseTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iFirstRaiseTime, 1264), 8 },
  { "emptyRaiseTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iEmptyRaiseTime, 1268), 8 },
  { "emptyDropTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iEmptyDropTime, 1272), 8 },
  { "sprintInTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.sprintInTime, 1276), 8 },
  { "sprintLoopTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.sprintLoopTime, 1280), 8 },
  { "sprintOutTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.sprintOutTime, 1284), 8 },
  { "lowReadyInTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.lowReadyInTime, 1288), 8 },
  { "lowReadyLoopTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.lowReadyLoopTime, 1292), 8 },
  { "lowReadyOutTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.lowReadyOutTime, 1296), 8 },
  { "contFireInTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.contFireInTime, 1300), 8 },
  { "contFireLoopTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.contFireLoopTime, 1304), 8 },
  { "contFireOutTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.contFireOutTime, 1308), 8 },
  { "dtpInTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.dtpInTime, 1312), 8 },
  { "dtpLoopTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.dtpLoopTime, 1316), 8 },
  { "dtpOutTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.dtpOutTime, 1320), 8 },
  { "slideInTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.slideInTime, 1324), 8 },
  { "deployTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.deployTime, 1328), 8 },
  { "breakdownTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.breakdownTime, 1332), 8 },
  { "tracerFrequency", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.tracerFrequency, 296), 4 },
  { "tracerWidth", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.tracerWidth, 300), 7 },
  { "tracerLength", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.tracerLength, 304), 7 },
  { "nightVisionWearTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.nightVisionWearTime, 1336), 8 },
  { "nightVisionWearTimeFadeOutEnd", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.nightVisionWearTimeFadeOutEnd, 1340), 8 },
  { "nightVisionWearTimePowerUp", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.nightVisionWearTimePowerUp, 1344), 8 },
  { "nightVisionRemoveTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.nightVisionRemoveTime, 1348), 8 },
  { "nightVisionRemoveTimePowerDown", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.nightVisionRemoveTimePowerDown, 1352), 8 },
  { "nightVisionRemoveTimeFadeInStart", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.nightVisionRemoveTimeFadeInStart, 1356), 8 },
  { "fuseTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fuseTime, 1360), 8 },
  { "aifuseTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.aiFuseTime, 1364), 8 },
  { "lockOnRadius", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.lockOnRadius, 1368), 4 },
  { "lockOnSpeed", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.lockOnSpeed, 1372), 4 },
  { "requireLockonToFire", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.requireLockonToFire, 1376), 5 },
  { "noAdsWhenMagEmpty", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.noAdsWhenMagEmpty, 1377), 5 },
  { "avoidDropCleanup", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.avoidDropCleanup, 1378), 5 },
  { "stackFire", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.stackFire, 1380), 4 },
  { "stackFireSpread", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.stackFireSpread, 1384), 7 },
  { "stackFireAccuracyDecay", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.stackFireAccuracyDecay, 1388), 7 },
  { "stackSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.stackSound, 1392), 0 },
  { "autoAimRange", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.autoAimRange, 1396), 7 },
  { "aimAssistRange", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.aimAssistRange, 1400), 7 },
  { "aimAssistRangeAds", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.fAimAssistRangeAds, 80), 7 },
  { "mountableWeapon", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.mountableWeapon, 1404), 5 },
  { "aimPadding", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.aimPadding, 1408), 7 },
  { "enemyCrosshairRange", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.enemyCrosshairRange, 1412), 7 },
  { "crosshairColorChange", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.crosshairColorChange, 1416), 5 },
  { "moveSpeedScale", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.moveSpeedScale, 1420), 7 },
  { "adsMoveSpeedScale", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.adsMoveSpeedScale, 1424), 7 },
  { "sprintDurationScale", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.sprintDurationScale, 1428), 7 },
  { "idleCrouchFactor", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fIdleCrouchFactor, 1524), 7 },
  { "idleProneFactor", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fIdleProneFactor, 1528), 7 },
  { "gunMaxPitch", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fGunMaxPitch, 1532), 7 },
  { "gunMaxYaw", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fGunMaxYaw, 1536), 7 },
  { "swayMaxAngle", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.swayMaxAngle, 1540), 7 },
  { "swayLerpSpeed", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.swayLerpSpeed, 1544), 7 },
  { "swayPitchScale", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.swayPitchScale, 1548), 7 },
  { "swayYawScale", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.swayYawScale, 1552), 7 },
  { "swayHorizScale", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.swayHorizScale, 1556), 7 },
  { "swayVertScale", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.swayVertScale, 1560), 7 },
  { "swayShellShockScale", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.swayShellShockScale, 1564), 7 },
  { "adsSwayMaxAngle", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.adsSwayMaxAngle, 1568), 7 },
  { "adsSwayLerpSpeed", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.adsSwayLerpSpeed, 1572), 7 },
  { "adsSwayPitchScale", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.adsSwayPitchScale, 1576), 7 },
  { "adsSwayYawScale", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.adsSwayYawScale, 1580), 7 },
  { "adsSwayHorizScale", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.fAdsSwayHorizScale, 84), 7 },
  { "adsSwayVertScale", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.fAdsSwayVertScale, 88), 7 },
  { "meleeChargeRange", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.meleeChargeRange, 1836), 7 },
  { "rifleBullet", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bRifleBullet, 1585), 5 },
  { "armorPiercing", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.armorPiercing, 1586), 5 },
  { "boltAction", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bBoltAction, 1587), 5 },
  { "useAltTagFlash", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bUseAltTagFlash, 1588), 5 },
  { "useAntiLagRewind", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bUseAntiLagRewind, 1589), 5 },
  { "isCarriedKillstreakWeapon", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bIsCarriedKillstreakWeapon, 1590), 5 },
  { "aimDownSight", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.aimDownSight, 1591), 5 },
  { "rechamberWhileAds", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bRechamberWhileAds, 1592), 5 },
  { "reloadWhileAds", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bReloadWhileAds, 1593), 5 },
  { "adsViewErrorMin", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.adsViewErrorMin, 1596), 7 },
  { "adsViewErrorMax", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.adsViewErrorMax, 1600), 7 },
  { "clipOnly", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bClipOnly, 1605), 5 },
  { "canUseInVehicle", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bCanUseInVehicle, 1606), 5 },
  { "noDropsOrRaises", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bNoDropsOrRaises, 1607), 5 },
  { "cookOffHold", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bCookOffHold, 1604), 5 },
  { "adsFire", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.adsFireOnly, 1608), 5 },
  { "cancelAutoHolsterWhenEmpty", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.cancelAutoHolsterWhenEmpty, 1609), 5 },
  { "suppressAmmoReserveDisplay", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.suppressAmmoReserveDisplay, 1610), 5 },
  { "laserSightDuringNightvision", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.laserSightDuringNightvision, 1611), 5 },
  { "bayonet", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bHasBayonet, 1613), 5 },
  { "dualWield", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bDualWield, 1614), 5 },
  { "hideThirdPerson", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bHideThirdPerson, 1612), 5 },
  { "explodeOnGround", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bExplodeOnGround, 1615), 5 },
  { "throwBack", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bThrowBack, 1616), 5 },
  { "retrievable", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bRetrievable, 1617), 5 },
  { "dieOnRespawn", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bDieOnRespawn, 1618), 5 },
  { "noThirdPersonDropsOrRaises", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bNoThirdPersonDropsOrRaises, 1619), 5 },
  { "continuousFire", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bContinuousFire, 1620), 5 },
  { "fullMetalJacket", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.bFullMetalJacket, 134), 5 },
  { "hollowPoint", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.bHollowPoint, 135), 5 },
  { "useAsMelee", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bUseAsMelee, 1840), 5 },
  { "rapidFire", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.bRapidFire, 136), 5 },
  { "noPing", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bNoPing, 1621), 5 },
  { "forceBounce", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bForceBounce, 1622), 5 },
  { "useDroppedModelAsStowed", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bUseDroppedModelAsStowed, 1623), 5 },
  { "noQuickDropWhenEmpty", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bNoQuickDropWhenEmpty, 1624), 5 },
  { "keepCrosshairWhenADS", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bKeepCrosshairWhenADS, 1625), 5 },
  { "useOnlyAltWeaoponHideTagsInAltMode", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bUseOnlyAltWeaoponHideTagsInAltMode, 1626), 5 },
  { "killIcon", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.killIcon, 1628), 11 },
  { "killIconRatio", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.killIconRatio, 1632), 37 },
  { "flipKillIcon", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.flipKillIcon, 1636), 5 },
  { "dpadIcon", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.dpadIcon, 148), 11 },
  { "dpadIconRatio", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.dpadIconRatio, 152), 38 },
  { "noPartialReload", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bNoPartialReload, 1637), 5 },
  { "segmentedReload", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bSegmentedReload, 1638), 5 },
  { "noADSAutoReload", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bNoADSAutoReload, 1639), 5 },
  { "reloadAmmoAdd", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iReloadAmmoAdd, 1640), 4 },
  { "reloadStartAdd", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iReloadStartAdd, 1644), 4 },
  { "altWeapon", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.szAltWeaponName, 20), 0 },
  { "DualWieldWeapon", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.szDualWieldWeaponName, 1652), 0 },
  { "grenadeWeapon", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.szSpawnedGrenadeWeaponName, 1648), 0 },
  { "dropAmmoMin", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iDropAmmoMin, 1660), 4 },
  { "dropAmmoMax", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iDropAmmoMax, 1664), 4 },
  { "dropClipAmmoMin", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iDropClipAmmoMin, 1668), 4 },
  { "dropClipAmmoMax", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iDropClipAmmoMax, 1672), 4 },
  { "blocksProne", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.blocksProne, 1676), 5 },
  { "silenced", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.bSilenced, 132), 5 },
  { "dualMag", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.bDualMag, 133), 5 },
  { "isRollingGrenade", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.isRollingGrenade, 1680), 6 },
  { "showIndicator", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bShowIndicator, 1677), 5 },
  { "explosionRadius", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iExplosionRadius, 1684), 4 },
  { "explosionRadiusMin", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iExplosionRadiusMin, 1688), 4 },
  { "indicatorRadius", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iIndicatorRadius, 1692), 4 },
  { "explosionInnerDamage", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iExplosionInnerDamage, 1696), 4 },
  { "explosionOuterDamage", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iExplosionOuterDamage, 1700), 4 },
  { "damageConeAngle", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.damageConeAngle, 1704), 7 },
  { "projectileSpeed", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iProjectileSpeed, 1708), 4 },
  { "projectileSpeedRelativeUp", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iProjectileSpeedRelativeUp, 1716), 4 },
  { "projectileSpeedUp", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iProjectileSpeedUp, 1712), 4 },
  { "projectileSpeedForward", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iProjectileSpeedForward, 1720), 4 },
  { "projectileActivateDist", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iProjectileActivateDist, 1724), 4 },
  { "projectileLifetime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.projLifetime, 1728), 7 },
  { "timeToAccelerate", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.timeToAccelerate, 1732), 7 },
  { "projectileCurvature", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.projectileCurvature, 1736), 7 },
  { "projectileModel", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.projectileModel, 1740), 10 },
  { "projExplosionType", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.projExplosion, 1744), 21 },
  { "projExplosionEffect", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.projExplosionEffect, 1748), 9 },
  { "projExplosionEffectForceNormalUp", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.projExplosionEffectForceNormalUp, 1752), 5 },
  { "projExplosionEffect2", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.projExplosionEffect2, 1756), 9 },
  { "projExplosionEffect2ForceNormalUp", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.projExplosionEffect2ForceNormalUp, 1760), 5 },
  { "projExplosionEffect3", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.projExplosionEffect3, 1764), 9 },
  { "projExplosionEffect3ForceNormalUp", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.projExplosionEffect3ForceNormalUp, 1768), 5 },
  { "projExplosionEffect4", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.projExplosionEffect4, 1772), 9 },
  { "projExplosionEffect4ForceNormalUp", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.projExplosionEffect4ForceNormalUp, 1776), 5 },
  { "projExplosionEffect5", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.projExplosionEffect5, 1780), 9 },
  { "projExplosionEffect5ForceNormalUp", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.projExplosionEffect5ForceNormalUp, 1784), 5 },
  { "projExplosionSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.projExplosionSound, 1792), 0 },
  { "projDudEffect", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.projDudEffect, 1788), 9 },
  { "projDudSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.projDudSound, 1796), 0 },
  { "projImpactExplode", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bProjImpactExplode, 1808), 5 },
  { "bulletImpactExplode", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bBulletImpactExplode, 1809), 5 },
  { "mortarShellSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.mortarShellSound, 1800), 0 },
  { "tankShellSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.tankShellSound, 1804), 0 },
  { "stickiness", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.stickiness, 1812), 28 },
  { "rotateType", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.rotateType, 1816), 29 },
  { "hasDetonator", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.hasDetonator, 1821), 5 },
  { "plantable", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.plantable, 1820), 5 },
  { "timedDetonation", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.timedDetonation, 1822), 5 },
  { "noCrumpleMissile", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bNoCrumpleMissile, 1823), 5 },
  { "rotate", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.rotate, 1824), 5 },
  { "keepRolling", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bKeepRolling, 1825), 5 },
  { "holdButtonToThrow", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.holdButtonToThrow, 1826), 5 },
  { "offhandHoldIsCancelable", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.offhandHoldIsCancelable, 1827), 5 },
  { "freezeMovementWhenFiring", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.freezeMovementWhenFiring, 1828), 5 },
  { "lowAmmoWarningThreshold", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.lowAmmoWarningThreshold, 1832), 7 },
  { "explosionTag", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.explosionTag, 1112), 41 },
  { "isCameraSensor", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.isCameraSensor, 1841), 5 },
  { "isAcousticSensor", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.isAcousticSensor, 1842), 5 },
  { "parallelDefaultBounce", CSP_FIELD_OFFSET(WeaponFullDef, parallelBounce[0], 2820), 7 },
  { "parallelAsphaltBounce", CSP_FIELD_OFFSET(WeaponFullDef, parallelBounce[22], 2908), 7 },
  { "parallelBarkBounce", CSP_FIELD_OFFSET(WeaponFullDef, parallelBounce[1], 2824), 7 },
  { "parallelBrickBounce", CSP_FIELD_OFFSET(WeaponFullDef, parallelBounce[2], 2828), 7 },
  { "parallelCarpetBounce", CSP_FIELD_OFFSET(WeaponFullDef, parallelBounce[3], 2832), 7 },
  { "parallelCeramicBounce", CSP_FIELD_OFFSET(WeaponFullDef, parallelBounce[23], 2912), 7 },
  { "parallelClothBounce", CSP_FIELD_OFFSET(WeaponFullDef, parallelBounce[4], 2836), 7 },
  { "parallelConcreteBounce", CSP_FIELD_OFFSET(WeaponFullDef, parallelBounce[5], 2840), 7 },
  { "parallelCushionBounce", CSP_FIELD_OFFSET(WeaponFullDef, parallelBounce[26], 2924), 7 },
  { "parallelDirtBounce", CSP_FIELD_OFFSET(WeaponFullDef, parallelBounce[6], 2844), 7 },
  { "parallelFleshBounce", CSP_FIELD_OFFSET(WeaponFullDef, parallelBounce[7], 2848), 7 },
  { "parallelFoliageBounce", CSP_FIELD_OFFSET(WeaponFullDef, parallelBounce[8], 2852), 7 },
  { "parallelFruitBounce", CSP_FIELD_OFFSET(WeaponFullDef, parallelBounce[27], 2928), 7 },
  { "parallelGlassBounce", CSP_FIELD_OFFSET(WeaponFullDef, parallelBounce[9], 2856), 7 },
  { "parallelGrassBounce", CSP_FIELD_OFFSET(WeaponFullDef, parallelBounce[10], 2860), 7 },
  { "parallelGravelBounce", CSP_FIELD_OFFSET(WeaponFullDef, parallelBounce[11], 2864), 7 },
  { "parallelIceBounce", CSP_FIELD_OFFSET(WeaponFullDef, parallelBounce[12], 2868), 7 },
  { "parallelMetalBounce", CSP_FIELD_OFFSET(WeaponFullDef, parallelBounce[13], 2872), 7 },
  { "parallelMudBounce", CSP_FIELD_OFFSET(WeaponFullDef, parallelBounce[14], 2876), 7 },
  { "parallelPaintedMetalBounce", CSP_FIELD_OFFSET(WeaponFullDef, parallelBounce[28], 2932), 7 },
  { "parallelPaperBounce", CSP_FIELD_OFFSET(WeaponFullDef, parallelBounce[15], 2880), 7 },
  { "parallelPlasterBounce", CSP_FIELD_OFFSET(WeaponFullDef, parallelBounce[16], 2884), 7 },
  { "parallelPlasticBounce", CSP_FIELD_OFFSET(WeaponFullDef, parallelBounce[24], 2916), 7 },
  { "parallelRockBounce", CSP_FIELD_OFFSET(WeaponFullDef, parallelBounce[17], 2888), 7 },
  { "parallelRubberBounce", CSP_FIELD_OFFSET(WeaponFullDef, parallelBounce[25], 2920), 7 },
  { "parallelSandBounce", CSP_FIELD_OFFSET(WeaponFullDef, parallelBounce[18], 2892), 7 },
  { "parallelSnowBounce", CSP_FIELD_OFFSET(WeaponFullDef, parallelBounce[19], 2896), 7 },
  { "parallelWaterBounce", CSP_FIELD_OFFSET(WeaponFullDef, parallelBounce[20], 2900), 7 },
  { "parallelWoodBounce", CSP_FIELD_OFFSET(WeaponFullDef, parallelBounce[21], 2904), 7 },
  { "perpendicularDefaultBounce", CSP_FIELD_OFFSET(WeaponFullDef, perpendicularBounce[0], 2944), 7 },
  { "perpendicularAsphaltBounce", CSP_FIELD_OFFSET(WeaponFullDef, perpendicularBounce[22], 3032), 7 },
  { "perpendicularBarkBounce", CSP_FIELD_OFFSET(WeaponFullDef, perpendicularBounce[1], 2948), 7 },
  { "perpendicularBrickBounce", CSP_FIELD_OFFSET(WeaponFullDef, perpendicularBounce[2], 2952), 7 },
  { "perpendicularCarpetBounce", CSP_FIELD_OFFSET(WeaponFullDef, perpendicularBounce[3], 2956), 7 },
  { "perpendicularCeramicBounce", CSP_FIELD_OFFSET(WeaponFullDef, perpendicularBounce[23], 3036), 7 },
  { "perpendicularClothBounce", CSP_FIELD_OFFSET(WeaponFullDef, perpendicularBounce[4], 2960), 7 },
  { "perpendicularConcreteBounce", CSP_FIELD_OFFSET(WeaponFullDef, perpendicularBounce[5], 2964), 7 },
  { "perpendicularCushionBounce", CSP_FIELD_OFFSET(WeaponFullDef, perpendicularBounce[26], 3048), 7 },
  { "perpendicularDirtBounce", CSP_FIELD_OFFSET(WeaponFullDef, perpendicularBounce[6], 2968), 7 },
  { "perpendicularFleshBounce", CSP_FIELD_OFFSET(WeaponFullDef, perpendicularBounce[7], 2972), 7 },
  { "perpendicularFoliageBounce", CSP_FIELD_OFFSET(WeaponFullDef, perpendicularBounce[8], 2976), 7 },
  { "perpendicularFruitBounce", CSP_FIELD_OFFSET(WeaponFullDef, perpendicularBounce[27], 3052), 7 },
  { "perpendicularGlassBounce", CSP_FIELD_OFFSET(WeaponFullDef, perpendicularBounce[9], 2980), 7 },
  { "perpendicularGrassBounce", CSP_FIELD_OFFSET(WeaponFullDef, perpendicularBounce[10], 2984), 7 },
  { "perpendicularGravelBounce", CSP_FIELD_OFFSET(WeaponFullDef, perpendicularBounce[11], 2988), 7 },
  { "perpendicularIceBounce", CSP_FIELD_OFFSET(WeaponFullDef, perpendicularBounce[12], 2992), 7 },
  { "perpendicularMetalBounce", CSP_FIELD_OFFSET(WeaponFullDef, perpendicularBounce[13], 2996), 7 },
  { "perpendicularMudBounce", CSP_FIELD_OFFSET(WeaponFullDef, perpendicularBounce[14], 3000), 7 },
  { "perpendicularPaintedMetalBounce", CSP_FIELD_OFFSET(WeaponFullDef, perpendicularBounce[28], 3056), 7 },
  { "perpendicularPaperBounce", CSP_FIELD_OFFSET(WeaponFullDef, perpendicularBounce[15], 3004), 7 },
  { "perpendicularPlasterBounce", CSP_FIELD_OFFSET(WeaponFullDef, perpendicularBounce[16], 3008), 7 },
  { "perpendicularPlasticBounce", CSP_FIELD_OFFSET(WeaponFullDef, perpendicularBounce[24], 3040), 7 },
  { "perpendicularRockBounce", CSP_FIELD_OFFSET(WeaponFullDef, perpendicularBounce[17], 3012), 7 },
  { "perpendicularRubberBounce", CSP_FIELD_OFFSET(WeaponFullDef, perpendicularBounce[25], 3044), 7 },
  { "perpendicularSandBounce", CSP_FIELD_OFFSET(WeaponFullDef, perpendicularBounce[18], 3016), 7 },
  { "perpendicularSnowBounce", CSP_FIELD_OFFSET(WeaponFullDef, perpendicularBounce[19], 3020), 7 },
  { "perpendicularWaterBounce", CSP_FIELD_OFFSET(WeaponFullDef, perpendicularBounce[20], 3024), 7 },
  { "perpendicularWoodBounce", CSP_FIELD_OFFSET(WeaponFullDef, perpendicularBounce[21], 3028), 7 },
  { "projTrailEffect", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.projTrailEffect, 1852), 9 },
  { "projectileRed", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vProjectileColor[0], 1856), 7 },
  { "projectileGreen", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vProjectileColor[1], 1860), 7 },
  { "projectileBlue", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vProjectileColor[2], 1864), 7 },
  { "guidedMissileType", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.guidedMissileType, 1868), 26 },
  { "maxSteeringAccel", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.maxSteeringAccel, 1872), 7 },
  { "projIgnitionDelay", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.projIgnitionDelay, 1876), 4 },
  { "projIgnitionEffect", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.projIgnitionEffect, 1880), 9 },
  { "projIgnitionSound", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.projIgnitionSound, 1884), 0 },
  { "tagFx_preparationEffect", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.tagFx_preparationEffect, 2268), 9 },
  { "tagFlash_preparationEffect", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.tagFlash_preparationEffect, 2272), 9 },
  { "adsTransInTime", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.iAdsTransInTime, 52), 8 },
  { "adsTransOutTime", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.iAdsTransOutTime, 56), 8 },
  { "adsIdleAmount", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fAdsIdleAmount, 1508), 7 },
  { "adsIdleSpeed", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.adsIdleSpeed, 1516), 7 },
  { "adsZoomFov1", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.fAdsZoomFov1, 100), 7 },
  { "adsZoomFov2", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.fAdsZoomFov2, 104), 7 },
  { "adsZoomFov3", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.fAdsZoomFov3, 108), 7 },
  { "adsZoomInFrac", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.fAdsZoomInFrac, 112), 7 },
  { "adsZoomOutFrac", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.fAdsZoomOutFrac, 116), 7 },
  { "adsOverlayShader", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.overlayMaterial, 140), 12 },
  { "adsOverlayShaderLowRes", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.overlayMaterialLowRes, 144), 12 },
  { "adsOverlayReticle", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.overlayReticle, 1432), 17 },
  { "adsOverlayInterface", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.overlayInterface, 1436), 30 },
  { "adsOverlayWidth", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.overlayWidth, 1440), 7 },
  { "adsOverlayHeight", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.overlayHeight, 1444), 7 },
  { "adsOverlayAlphaScale", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.fOverlayAlphaScale, 120), 7 },
  { "adsBobFactor", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fAdsBobFactor, 1448), 7 },
  { "adsViewBobMult", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fAdsViewBobMult, 1452), 7 },
  { "adsAimPitch", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fAdsAimPitch, 1888), 7 },
  { "adsCrosshairInFrac", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fAdsCrosshairInFrac, 1892), 7 },
  { "adsCrosshairOutFrac", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fAdsCrosshairOutFrac, 1896), 7 },
  { "adsReloadTransTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.iPositionReloadTransTime, 2080), 8 },
  { "adsGunKickReducedKickBullets", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.adsGunKickReducedKickBullets, 1900), 4 },
  { "adsGunKickReducedKickPercent", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.adsGunKickReducedKickPercent, 1904), 7 },
  { "adsGunKickPitchMin", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fAdsGunKickPitchMin, 1908), 7 },
  { "adsGunKickPitchMax", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fAdsGunKickPitchMax, 1912), 7 },
  { "adsGunKickYawMin", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fAdsGunKickYawMin, 1916), 7 },
  { "adsGunKickYawMax", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fAdsGunKickYawMax, 1920), 7 },
  { "adsGunKickAccel", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fAdsGunKickAccel, 1924), 7 },
  { "adsGunKickSpeedMax", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fAdsGunKickSpeedMax, 1928), 7 },
  { "adsGunKickSpeedDecay", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fAdsGunKickSpeedDecay, 1932), 7 },
  { "adsGunKickStaticDecay", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fAdsGunKickStaticDecay, 1936), 7 },
  { "adsViewKickPitchMin", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fAdsViewKickPitchMin, 1940), 7 },
  { "adsViewKickPitchMax", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fAdsViewKickPitchMax, 1944), 7 },
  { "adsViewKickYawMin", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fAdsViewKickYawMin, 1948), 7 },
  { "adsViewKickYawMax", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fAdsViewKickYawMax, 1952), 7 },
  { "adsViewKickCenterSpeed", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.fAdsViewKickCenterSpeed, 92), 7 },
  { "adsSpread", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fAdsSpread, 1964), 7 },
  { "guidedMissileType", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.guidedMissileType, 1868), 26 },
  { "hipSpreadStandMin", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fHipSpreadStandMin, 1456), 7 },
  { "hipSpreadDuckedMin", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fHipSpreadDuckedMin, 1460), 7 },
  { "hipSpreadProneMin", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fHipSpreadProneMin, 1464), 7 },
  { "hipSpreadMax", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.hipSpreadStandMax, 1468), 7 },
  { "hipSpreadDuckedMax", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.hipSpreadDuckedMax, 1472), 7 },
  { "hipSpreadProneMax", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.hipSpreadProneMax, 1476), 7 },
  { "hipSpreadDecayRate", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fHipSpreadDecayRate, 1480), 7 },
  { "hipSpreadFireAdd", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fHipSpreadFireAdd, 1484), 7 },
  { "hipSpreadTurnAdd", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fHipSpreadTurnAdd, 1488), 7 },
  { "hipSpreadMoveAdd", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fHipSpreadMoveAdd, 1492), 7 },
  { "hipSpreadDuckedDecay", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fHipSpreadDuckedDecay, 1496), 7 },
  { "hipSpreadProneDecay", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fHipSpreadProneDecay, 1500), 7 },
  { "hipReticleSidePos", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fHipReticleSidePos, 1504), 7 },
  { "hipIdleAmount", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fHipIdleAmount, 1512), 7 },
  { "hipIdleSpeed", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.hipIdleSpeed, 1520), 7 },
  { "hipGunKickReducedKickBullets", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.hipGunKickReducedKickBullets, 1968), 4 },
  { "hipGunKickReducedKickPercent", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.hipGunKickReducedKickPercent, 1972), 7 },
  { "hipGunKickPitchMin", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fHipGunKickPitchMin, 1976), 7 },
  { "hipGunKickPitchMax", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fHipGunKickPitchMax, 1980), 7 },
  { "hipGunKickYawMin", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fHipGunKickYawMin, 1984), 7 },
  { "hipGunKickYawMax", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fHipGunKickYawMax, 1988), 7 },
  { "hipGunKickAccel", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fHipGunKickAccel, 1992), 7 },
  { "hipGunKickSpeedMax", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fHipGunKickSpeedMax, 1996), 7 },
  { "hipGunKickSpeedDecay", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fHipGunKickSpeedDecay, 2000), 7 },
  { "hipGunKickStaticDecay", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fHipGunKickStaticDecay, 2004), 7 },
  { "hipViewKickPitchMin", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fHipViewKickPitchMin, 2008), 7 },
  { "hipViewKickPitchMax", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fHipViewKickPitchMax, 2012), 7 },
  { "hipViewKickYawMin", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fHipViewKickYawMin, 2016), 7 },
  { "hipViewKickYawMax", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fHipViewKickYawMax, 2020), 7 },
  { "hipViewKickCenterSpeed", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.fHipViewKickCenterSpeed, 96), 7 },
  { "leftArc", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.leftArc, 2084), 7 },
  { "rightArc", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.rightArc, 2088), 7 },
  { "topArc", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.topArc, 2092), 7 },
  { "bottomArc", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.bottomArc, 2096), 7 },
  { "accuracy", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.accuracy, 2100), 7 },
  { "aiSpread", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.aiSpread, 2104), 7 },
  { "playerSpread", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.playerSpread, 2108), 7 },
  { "maxVertTurnSpeed", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.maxTurnSpeed[0], 2120), 7 },
  { "maxHorTurnSpeed", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.maxTurnSpeed[1], 2124), 7 },
  { "minVertTurnSpeed", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.minTurnSpeed[0], 2112), 7 },
  { "minHorTurnSpeed", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.minTurnSpeed[1], 2116), 7 },
  { "pitchConvergenceTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.pitchConvergenceTime, 2128), 7 },
  { "yawConvergenceTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.yawConvergenceTime, 2132), 7 },
  { "suppressionTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.suppressTime, 2136), 7 },
  { "maxRange", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.maxRange, 2140), 7 },
  { "animHorRotateInc", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fAnimHorRotateInc, 2144), 7 },
  { "playerPositionDist", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fPlayerPositionDist, 2148), 7 },
  { "stance", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.stance, 340), 20 },
  { "useHintString", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.szUseHintString, 2152), 0 },
  { "dropHintString", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.dropHintString, 2156), 0 },
  { "horizViewJitter", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.horizViewJitter, 2168), 7 },
  { "vertViewJitter", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.vertViewJitter, 2172), 7 },
  { "fightDist", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fightDist, 2032), 7 },
  { "maxDist", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.maxDist, 2036), 7 },
  { "aiVsAiAccuracyGraph", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.accuracyGraphName[0], 2040), 0 },
  { "aiVsPlayerAccuracyGraph", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.accuracyGraphName[1], 2044), 0 },
  { "locNone", CSP_FIELD_OFFSET(WeaponFullDef, locationDamageMultipliers[0], 3068), 7 },
  { "locHelmet", CSP_FIELD_OFFSET(WeaponFullDef, locationDamageMultipliers[1], 3072), 7 },
  { "locHead", CSP_FIELD_OFFSET(WeaponFullDef, locationDamageMultipliers[2], 3076), 7 },
  { "locNeck", CSP_FIELD_OFFSET(WeaponFullDef, locationDamageMultipliers[3], 3080), 7 },
  { "locTorsoUpper", CSP_FIELD_OFFSET(WeaponFullDef, locationDamageMultipliers[4], 3084), 7 },
  { "locTorsoLower", CSP_FIELD_OFFSET(WeaponFullDef, locationDamageMultipliers[5], 3088), 7 },
  { "locRightArmUpper", CSP_FIELD_OFFSET(WeaponFullDef, locationDamageMultipliers[6], 3092), 7 },
  { "locRightArmLower", CSP_FIELD_OFFSET(WeaponFullDef, locationDamageMultipliers[8], 3100), 7 },
  { "locRightHand", CSP_FIELD_OFFSET(WeaponFullDef, locationDamageMultipliers[10], 3108), 7 },
  { "locLeftArmUpper", CSP_FIELD_OFFSET(WeaponFullDef, locationDamageMultipliers[7], 3096), 7 },
  { "locLeftArmLower", CSP_FIELD_OFFSET(WeaponFullDef, locationDamageMultipliers[9], 3104), 7 },
  { "locLeftHand", CSP_FIELD_OFFSET(WeaponFullDef, locationDamageMultipliers[11], 3112), 7 },
  { "locRightLegUpper", CSP_FIELD_OFFSET(WeaponFullDef, locationDamageMultipliers[12], 3116), 7 },
  { "locRightLegLower", CSP_FIELD_OFFSET(WeaponFullDef, locationDamageMultipliers[14], 3124), 7 },
  { "locRightFoot", CSP_FIELD_OFFSET(WeaponFullDef, locationDamageMultipliers[16], 3132), 7 },
  { "locLeftLegUpper", CSP_FIELD_OFFSET(WeaponFullDef, locationDamageMultipliers[13], 3120), 7 },
  { "locLeftLegLower", CSP_FIELD_OFFSET(WeaponFullDef, locationDamageMultipliers[15], 3128), 7 },
  { "locLeftFoot", CSP_FIELD_OFFSET(WeaponFullDef, locationDamageMultipliers[17], 3136), 7 },
  { "locGun", CSP_FIELD_OFFSET(WeaponFullDef, locationDamageMultipliers[18], 3140), 7 },
  { "fireRumble", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.fireRumble, 2212), 0 },
  { "meleeImpactRumble", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.meleeImpactRumble, 2216), 0 },
  { "reloadRumble", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.reloadRumble, 2220), 0 },
  { "adsDofStart", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.adsDofStart, 2224), 7 },
  { "adsDofEnd", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.adsDofEnd, 2228), 7 },
  { "scanSpeed", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.scanSpeed, 2240), 7 },
  { "scanAccel", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.scanAccel, 2244), 7 },
  { "scanPauseTime", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.scanPauseTime, 2248), 8 },
  { "flameTableFirstPerson", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.flameTableFirstPerson, 2252), 0 },
  { "flameTableThirdPerson", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.flameTableThirdPerson, 2256), 0 },
  { "ikLeftHandOffsetF", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.ikLeftHandOffset[0], 156), 7 },
  { "ikLeftHandOffsetR", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.ikLeftHandOffset[1], 160), 7 },
  { "ikLeftHandOffsetU", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.ikLeftHandOffset[2], 164), 7 },
  { "ikLeftHandRotationP", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.ikLeftHandRotation[0], 168), 7 },
  { "ikLeftHandRotationY", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.ikLeftHandRotation[1], 172), 7 },
  { "ikLeftHandRotationR", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.ikLeftHandRotation[2], 176), 7 },
  { "ikLeftHandProneOffsetF", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.ikLeftHandProneOffset[0], 180), 7 },
  { "ikLeftHandProneOffsetR", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.ikLeftHandProneOffset[1], 184), 7 },
  { "ikLeftHandProneOffsetU", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.ikLeftHandProneOffset[2], 188), 7 },
  { "ikLeftHandProneRotationP", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.ikLeftHandProneRotation[0], 192), 7 },
  { "ikLeftHandProneRotationY", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.ikLeftHandProneRotation[1], 196), 7 },
  { "ikLeftHandProneRotationR", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.ikLeftHandProneRotation[2], 200), 7 },
  { "ikLeftHandUiViewerOffsetF", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.ikLeftHandUiViewerOffset[0], 204), 7 },
  { "ikLeftHandUiViewerOffsetR", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.ikLeftHandUiViewerOffset[1], 208), 7 },
  { "ikLeftHandUiViewerOffsetU", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.ikLeftHandUiViewerOffset[2], 212), 7 },
  { "ikLeftHandUiViewerRotationP", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.ikLeftHandUiViewerRotation[0], 216), 7 },
  { "ikLeftHandUiViewerRotationY", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.ikLeftHandUiViewerRotation[1], 220), 7 },
  { "ikLeftHandUiViewerRotationR", CSP_FIELD_OFFSET(WeaponFullDef, weapVariantDef.ikLeftHandUiViewerRotation[2], 224), 7 },
  { "parentWeaponName", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.parentWeaponName, 288), 0 },
  { "doGibbing", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.doGibbing, 2276), 5 },
  { "maxGibDistance", CSP_FIELD_OFFSET(WeaponFullDef, weapDef.maxGibDistance, 2280), 7 }
};



const char *szWeapTypeNames[8] =
{
  "bullet",
  "grenade",
  "projectile",
  "binoculars",
  "gas",
  "bomb",
  "mine",
  "melee"
};

const char *szWeapClassNames[13] =
{
  "rifle",
  "mg",
  "smg",
  "spread",
  "pistol",
  "grenade",
  "rocketlauncher",
  "turret",
  "non-player",
  "gas",
  "item",
  "melee",
  "Killstreak Alt Stored Weapon"
};

const char *szWeapInventoryTypeNames[5] =
{ "primary", "offhand", "item", "altmode", "melee" };





char *__cdecl BG_GetPlayerAnimTypeName(int index)
{
    return g_playerAnimTypeNames[index];
}

int __cdecl BG_GetPlayerAnimTypeIndex(int typeEnum)
{
    int result; // eax

    switch ( typeEnum )
    {
        case 0:
            result = g_playerAnimTypeIndex_revivee;
            break;
        case 1:
            result = g_playerAnimTypeIndex_sniper;
            break;
        case 2:
            result = g_playerAnimTypeIndex_sniper_rearclip;
            break;
        case 3:
            result = g_playerAnimTypeIndex_briefcase;
            break;
        default:
            result = -1;
            break;
    }
    return result;
}

const char *__cdecl BG_GetWeaponTypeName(weapType_t type)
{
    if ( (unsigned int)type >= WEAPTYPE_NUM
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\bgame\\bg_weapons_load_obj.cpp",
                    1465,
                    0,
                    "type doesn't index ARRAY_COUNT( szWeapTypeNames )\n\t%i not in [0, %i)",
                    type,
                    8) )
    {
        __debugbreak();
    }
    return szWeapTypeNames[type];
}

const char *__cdecl BG_GetWeaponClassName(weapClass_t type)
{
    if ( (unsigned int)type >= WEAPCLASS_NUM
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\bgame\\bg_weapons_load_obj.cpp",
                    1473,
                    0,
                    "type doesn't index ARRAY_COUNT( szWeapClassNames )\n\t%i not in [0, %i)",
                    type,
                    13) )
    {
        __debugbreak();
    }
    return szWeapClassNames[type];
}

const char *__cdecl BG_GetWeaponInventoryTypeName(weapInventoryType_t type)
{
    if ( (unsigned int)type >= WEAPINVENTORYCOUNT
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\bgame\\bg_weapons_load_obj.cpp",
                    1481,
                    0,
                    "type doesn't index ARRAY_COUNT( szWeapInventoryTypeNames )\n\t%i not in [0, %i)",
                    type,
                    5) )
    {
        __debugbreak();
    }
    return szWeapInventoryTypeNames[type];
}

void __cdecl BG_LoadWeaponStrings()
{
    unsigned int i; // [esp+0h] [ebp-4h]

    for ( i = 0; i < g_playerAnimTypeNamesCount; ++i )
        BG_InitWeaponString(i, g_playerAnimTypeNames[i]);
}

void __cdecl BG_LoadPlayerAnimTypes()
{
    char v0; // [esp+53h] [ebp-29h]
    char *v1; // [esp+58h] [ebp-24h]
    const char *v2; // [esp+5Ch] [ebp-20h]
    char *buf; // [esp+70h] [ebp-Ch]
    const char *text_p; // [esp+74h] [ebp-8h] BYREF
    const char *token; // [esp+78h] [ebp-4h]

    g_playerAnimTypeNamesCount = 0;
    g_playerAnimTypeIndex_revivee = -1;
    buf = Com_LoadRawTextFile("mp/playeranimtypes.txt");
    if ( !buf )
        Com_Error(ERR_DROP, "Couldn't load file '%s'", "mp/playeranimtypes.txt");
    text_p = buf;
    Com_BeginParseSession("BG_AnimParseAnimScript");
    while ( 1 )
    {
        token = (const char *)Com_Parse(&text_p);
        if ( !token || !*token )
            break;
        if ( g_playerAnimTypeNamesCount >= 0x20 )
            Com_Error(ERR_DROP, "Player anim type array size exceeded");
        g_playerAnimTypeNames[g_playerAnimTypeNamesCount] = (char *)Hunk_Alloc(
                                                                                                                                    strlen(token) + 1,
                                                                                                                                    "BG_LoadPlayerAnimTypes",
                                                                                                                                    10);
        v2 = token;
        v1 = g_playerAnimTypeNames[g_playerAnimTypeNamesCount];
        do
        {
            v0 = *v2;
            *v1++ = *v2++;
        }
        while ( v0 );
        if ( !strcmp(token, "revivee") )
        {
            g_playerAnimTypeIndex_revivee = g_playerAnimTypeNamesCount;
        }
        else if ( !strcmp(token, "sniper") )
        {
            g_playerAnimTypeIndex_sniper = g_playerAnimTypeNamesCount;
        }
        else if ( !strcmp(token, "briefcase") )
        {
            g_playerAnimTypeIndex_briefcase = g_playerAnimTypeNamesCount;
        }
        else if ( !strcmp(token, "rearclipsniper") )
        {
            g_playerAnimTypeIndex_sniper_rearclip = g_playerAnimTypeNamesCount;
        }
        ++g_playerAnimTypeNamesCount;
    }
    Com_EndParseSession();
    Com_UnloadRawTextFile(buf);
}

void __cdecl BG_ClearWeaponDefInternal()
{
    surfaceTypeSoundListCount = 0;
}

void __cdecl SetConfigString(char **ppszConfigString, const char *pszKeyValue)
{
    char v2; // [esp+3h] [ebp-21h]
    char *v3; // [esp+8h] [ebp-1Ch]
    const char *v4; // [esp+Ch] [ebp-18h]
    char *buf; // [esp+20h] [ebp-4h]

    if ( *pszKeyValue )
    {
        buf = (char *)Hunk_AllocLowAlign(strlen(pszKeyValue) + 1, 1, "SetConfigString", 10);
        v4 = pszKeyValue;
        v3 = buf;
        do
        {
            v2 = *v4;
            *v3++ = *v4++;
        }
        while ( v2 );
        *ppszConfigString = buf;
    }
    else
    {
        *ppszConfigString = (char *)"";
    }
}

flameTable *__cdecl BG_LoadFlameTableInternal(const char *folder, char *name)
{
    char buffer[10244]; // [esp+14h] [ebp-2858h] BYREF
    int f; // [esp+2818h] [ebp-54h] BYREF
    int len; // [esp+281Ch] [ebp-50h]
    signed int v6; // [esp+2820h] [ebp-4Ch]
    char dest[64]; // [esp+2824h] [ebp-48h] BYREF
    flameTable *fTable; // [esp+2868h] [ebp-4h]

    len = strlen("FLAMETABLEFILE");
    fTable = (flameTable *)Hunk_AllocLow(sizeof(flameTable), "BG_LoadFlameTableInternal", 10);
    InitFlameTable(fTable);
    Com_sprintf(dest, 0x40u, "weapons/%s/%s", folder, name);
    v6 = FS_FOpenFileByMode(dest, &f, FS_READ);
    if ( v6 >= 0 )
    {
        FS_Read((unsigned __int8 *)buffer, len, f);
        buffer[len] = 0;
        if ( !strncmp(buffer, "FLAMETABLEFILE", len) )
        {
            if ( (unsigned int)(v6 - len) < 0x2800 )
            {
                memset((unsigned __int8 *)buffer, 0, 0x2800u);
                FS_Read((unsigned __int8 *)buffer, v6 - len, f);
                buffer[v6 - len] = 0;
                FS_FCloseFile(f);
                if ( Info_Validate(buffer) )
                {
                    SetConfigString((char **)&fTable->name, name);
                    if ( ParseConfigStringToStruct(
                                 (unsigned __int8 *)fTable,
                                 flameTableFields,
                                 119,
                                 buffer,
                                 0,
                                 0,
                                 SetConfigString2) )
                    {
                        if ( bg_nextParseFlameTableIndex >= 8
                            && !Assert_MyHandler(
                                        "C:\\projects_pc\\cod\\codsrc\\src\\bgame\\bg_weapons_load_obj.cpp",
                                        2123,
                                        0,
                                        "bg_nextParseFlameTableIndex doesn't index ARRAY_COUNT( bg_flameTables )\n\t%i not in [0, %i)",
                                        bg_nextParseFlameTableIndex,
                                        8) )
                        {
                            __debugbreak();
                        }
                        bg_flameTables[bg_nextParseFlameTableIndex++] = fTable;
                        return fTable;
                    }
                    else
                    {
                        return 0;
                    }
                }
                else
                {
                    Com_PrintWarning(17, "WARNING: \"%s\" is not a valid flametable file\n", dest);
                    return 0;
                }
            }
            else
            {
                Com_PrintWarning(
                    17,
                    "WARNING: \"%s\" Is too long of a flametable file to parse (fileLength = %d identifierLength = %d)\n",
                    dest,
                    v6,
                    len);
                FS_FCloseFile(f);
                return 0;
            }
        }
        else
        {
            Com_PrintWarning(17, "WARNING: \"%s\" does not appear to be a flametable file\n", dest);
            FS_FCloseFile(f);
            return 0;
        }
    }
    else
    {
        Com_PrintWarning(17, "WARNING: Could not load flametable file '%s'\n", dest);
        return 0;
    }
}

void __cdecl SetConfigString2(unsigned __int8 *pMember, const char *pszKeyValue)
{
    SetConfigString((char **)pMember, pszKeyValue);
}

void __cdecl InitFlameTable(flameTable *fTable)
{
    const cspField_t *pField; // [esp+4h] [ebp-8h]
    int iField; // [esp+8h] [ebp-4h]

    iField = 0;
    pField = flameTableFields;
    while ( iField < 119 )
    {
        if ( !pField->iFieldType )
            *reinterpret_cast<const char **>((char *)&fTable->flameVar_streamChunkGravityStart + pField->iOffset) = "";
        ++iField;
        ++pField;
    }
}

flameTable *__cdecl BG_GetFlameTable(const char *folder, char *name)
{
    unsigned int i; // [esp+0h] [ebp-4h]

    for ( i = 0; i < bg_nextParseFlameTableIndex; ++i )
    {
        if ( !I_stricmp(bg_flameTables[i]->name, name) )
            return bg_flameTables[i];
    }
    return BG_LoadFlameTableInternal(folder, name);
}

char __cdecl BG_LoadWeaponFile(char *szFileName, char *szBuffer, int iBufferSize)
{
    int hFile; // [esp+14h] [ebp-Ch] BYREF
    int iIdentifierLen; // [esp+18h] [ebp-8h]
    int iFileLength; // [esp+1Ch] [ebp-4h]

    iIdentifierLen = strlen("WEAPONFILE");
    iFileLength = FS_FOpenFileByMode(szFileName, &hFile, FS_READ);
    if ( iFileLength >= 0 )
    {
        FS_Read((unsigned __int8 *)szBuffer, iIdentifierLen, hFile);
        szBuffer[iIdentifierLen] = 0;
        if ( !strncmp(szBuffer, "WEAPONFILE", iIdentifierLen) )
        {
            if ( iFileLength - iIdentifierLen < iBufferSize )
            {
                memset((unsigned __int8 *)szBuffer, 0, iBufferSize);
                FS_Read((unsigned __int8 *)szBuffer, iFileLength - iIdentifierLen, hFile);
                szBuffer[iFileLength - iIdentifierLen] = 0;
                FS_FCloseFile(hFile);
                if ( Info_Validate(szBuffer) )
                {
                    return 1;
                }
                else
                {
                    Com_PrintWarning(17, "WARNING: \"%s\" is not a valid weapon file\n", szFileName);
                    return 0;
                }
            }
            else
            {
                Com_PrintWarning(
                    17,
                    "WARNING: \"%s\" Is too long of a weapon file to parse (fileLength = %d identifierLength = %d)\n",
                    szFileName,
                    iFileLength,
                    iIdentifierLen);
                FS_FCloseFile(hFile);
                return 0;
            }
        }
        else
        {
            Com_PrintWarning(17, "WARNING: \"%s\" does not appear to be a weapon file\n", szFileName);
            FS_FCloseFile(hFile);
            return 0;
        }
    }
    else
    {
        Com_PrintWarning(17, "WARNING: Could not load weapon file '%s'\n", szFileName);
        return 0;
    }
}

void __cdecl BG_LoadWeaponMergeSupport()
{
    char v0; // [esp+3h] [ebp-2Dh]
    char *v1; // [esp+8h] [ebp-28h]
    const char *v2; // [esp+Ch] [ebp-24h]
    char *buf; // [esp+24h] [ebp-Ch]
    const char *text_p; // [esp+28h] [ebp-8h] BYREF
    const char *token; // [esp+2Ch] [ebp-4h]

    s_numWeaponGripAnimSubstrings = 0;
    if ( !useFastFile->current.enabled )
    {
        BG_LoadWeaponAttachmentTable();
        buf = Com_LoadRawTextFile("weapon_gripanims.csv");
        if ( !buf )
            Com_Error(ERR_DROP, "Couldn't load file '%s'", "weapon_gripanims.csv");
        text_p = buf;
        Com_BeginParseSession("BG_LoadWeaponMergeSupport");
        while ( 1 )
        {
            token = (const char *)Com_Parse(&text_p);
            if ( !token || !*token )
                break;
            if ( (unsigned int)s_numWeaponGripAnimSubstrings >= 0x40 )
                Com_Error(ERR_DROP, "Weapon grip anim substrings table size exceeded");
            s_weaponGripAnimSubstrings[s_numWeaponGripAnimSubstrings] = (char *)Hunk_Alloc(
                                                                                                                                                        strlen(token) + 1,
                                                                                                                                                        "BG_LoadWeaponMergeSupport",
                                                                                                                                                        10);
            v2 = token;
            v1 = s_weaponGripAnimSubstrings[s_numWeaponGripAnimSubstrings];
            do
            {
                v0 = *v2;
                *v1++ = *v2++;
            }
            while ( v0 );
            ++s_numWeaponGripAnimSubstrings;
        }
        Com_EndParseSession();
        Com_UnloadRawTextFile(buf);
    }
}

char __cdecl BG_SplitWeaponDefNames(
                const char *name,
                char **sources,
                WeaponComponentList *componentAll,
                char *outputName)
{
    int slot; // [esp+0h] [ebp-27Ch]
    WeaponComponentList componentBase; // [esp+4h] [ebp-278h] BYREF
    WeaponComponentList componentBaseAttachment; // [esp+13Ch] [ebp-140h] BYREF
    int count; // [esp+278h] [ebp-4h]

    BG_WeaponNameToComponentList(name, componentAll);
    BG_WeaponComponentListIdentify(componentAll);
    BG_WeaponComponentListSort(name, componentAll, outputName);
    count = BG_WeaponComponentListCountAttachments(componentAll);
    if ( count <= 5 )
    {
        if ( count >= 2 )
        {
            for ( slot = 1; slot < count + 1; ++slot )
            {
                BG_WeaponComponentListNthAttachment(componentAll, slot, &componentBaseAttachment);
                BG_WeaponComponentListToName(&componentBaseAttachment, sources[slot], 64);
            }
            BG_WeaponComponentListRemoveAllAttachments(componentAll, &componentBase);
            BG_WeaponComponentListToName(&componentBase, *sources, 64);
            return 1;
        }
        else
        {
            Com_PrintError(1, "Can not create merged weapon %s.    Too few attachments, must have at least two.\n", name);
            return 0;
        }
    }
    else
    {
        Com_PrintError(1, "Can not create merged weapon %s.    Too many attachments, max is %i\n", name, 5);
        return 0;
    }
}

int __cdecl BG_MergeWeaponDefClipName(char **value, char *mergedValue, int size)
{
    char v4; // [esp+3h] [ebp-529h]
    WeaponComponent *v5; // [esp+8h] [ebp-524h]
    WeaponComponent *v6; // [esp+Ch] [ebp-520h]
    char v7; // [esp+13h] [ebp-519h]
    WeaponComponent *v8; // [esp+18h] [ebp-514h]
    WeaponComponent *v9; // [esp+1Ch] [ebp-510h]
    char v10; // [esp+23h] [ebp-509h]
    WeaponComponent *v11; // [esp+28h] [ebp-504h]
    WeaponComponent *v12; // [esp+2Ch] [ebp-500h]
    int m; // [esp+30h] [ebp-4FCh]
    WeaponComponentList merged; // [esp+34h] [ebp-4F8h] BYREF
    int comp[3]; // [esp+170h] [ebp-3BCh] BYREF
    WeaponComponentList componentList[3]; // [esp+17Ch] [ebp-3B0h] BYREF

    memset((unsigned __int8 *)&merged, 0, sizeof(merged));
    for ( m = 0; m < 3; ++m )
        BG_WeaponNameToComponentList(value[m], &componentList[m]);
    memset(comp, 0, sizeof(comp));
    while ( comp[0] < componentList[0].numComponents )
    {
        if ( comp[1] < componentList[1].numComponents
            && I_stricmp(componentList[1].components[comp[1]].name, componentList[0].components[comp[0]].name) )
        {
            v12 = &componentList[1].components[comp[1]];
            v11 = &merged.components[merged.numComponents];
            do
            {
                v10 = v12->name[0];
                v11->name[0] = v12->name[0];
                v12 = (WeaponComponent *)((char *)v12 + 1);
                v11 = (WeaponComponent *)((char *)v11 + 1);
            }
            while ( v10 );
            ++merged.numComponents;
        }
        ++comp[1];
        if ( comp[2] < componentList[2].numComponents
            && I_stricmp(componentList[2].components[comp[2]].name, componentList[0].components[comp[0]].name) )
        {
            v9 = &componentList[2].components[comp[2]];
            v8 = &merged.components[merged.numComponents];
            do
            {
                v7 = v9->name[0];
                v8->name[0] = v9->name[0];
                v9 = (WeaponComponent *)((char *)v9 + 1);
                v8 = (WeaponComponent *)((char *)v8 + 1);
            }
            while ( v7 );
            ++merged.numComponents;
        }
        ++comp[2];
        v6 = &componentList[0].components[comp[0]];
        v5 = &merged.components[merged.numComponents];
        do
        {
            v4 = v6->name[0];
            v5->name[0] = v6->name[0];
            v6 = (WeaponComponent *)((char *)v6 + 1);
            v5 = (WeaponComponent *)((char *)v5 + 1);
        }
        while ( v4 );
        ++merged.numComponents;
        ++comp[0];
    }
    BG_WeaponComponentListToName(&merged, mergedValue, size);
    return 1;
}

int __cdecl BG_MergeWeaponDefFloatValue(char **value, char *mergedValue)
{
    float valueRight; // [esp+8h] [ebp-8h]
    float valueLeft; // [esp+Ch] [ebp-4h]

    valueLeft = atof(value[1]);
    valueRight = atof(value[2]);
    Com_sprintf(mergedValue, 4u, "%.2f", (float)((float)(valueLeft + valueRight) / 2.0));
    return 1;
}

int __cdecl BG_MergeWeaponDefHidetags(char **value, char *mergedValue, int size)
{
    int v4; // [esp+10h] [ebp-10E0h]
    int i; // [esp+14h] [ebp-10DCh]
    _BYTE v6[196]; // [esp+18h] [ebp-10D8h] BYREF
    char *s1; // [esp+DCh] [ebp-1014h]
    char s0[4100]; // [esp+E0h] [ebp-1010h] BYREF
    int j; // [esp+10E8h] [ebp-8h]
    char *data_p; // [esp+10ECh] [ebp-4h] BYREF

    v4 = 0;
    for ( i = 0; i < 3; ++i )
    {
        data_p = value[i];
        for ( s1 = (char *)Com_Parse((const char **)&data_p); s1 && *s1; s1 = (char *)Com_Parse((const char **)&data_p) )
        {
            if ( strlen(s1) >= 0x40
                && !Assert_MyHandler(
                            "C:\\projects_pc\\cod\\codsrc\\src\\bgame\\bg_weapons_load_obj.cpp",
                            2474,
                            0,
                            "%s",
                            "strlen( token ) < MAX_TAG_LEN") )
            {
                __debugbreak();
            }
            for ( j = 0; j < v4; ++j )
            {
                if ( !I_stricmp(&s0[64 * j], s1) )
                {
                    v6[3 * j + i] = 1;
                    break;
                }
            }
            if ( j == v4 )
            {
                strncpy(&s0[64 * v4], s1, 0x40u);
                v6[3 * v4] = 0;
                v6[3 * v4 + 1] = 0;
                v6[3 * v4 + 2] = 0;
                v6[3 * v4++ + i] = 1;
            }
        }
    }
    *mergedValue = 0;
    for ( j = 0; j < v4; ++j )
    {
        if ( !v6[3 * j] || v6[3 * j + 1] && v6[3 * j + 2] )
        {
            if ( !v6[3 * j + 1]
                && !v6[3 * j + 2]
                && !Assert_MyHandler(
                            "C:\\projects_pc\\cod\\codsrc\\src\\bgame\\bg_weapons_load_obj.cpp",
                            2505,
                            0,
                            "%s",
                            "tagsPresent[tag][MERGE_LEFT] || tagsPresent[tag][MERGE_RIGHT]") )
            {
                __debugbreak();
            }
            I_strncat(mergedValue, size, &s0[64 * j]);
            I_strncat(mergedValue, size, "\n");
        }
    }
    return 1;
}

bool __cdecl BG_IsAnimationField(const char *fieldName)
{
    const char *v2; // [esp+Ch] [ebp-8h]

    v2 = &fieldName[strlen(fieldName) + 1];
    return v2 - (fieldName + 1) >= 4 && I_stricmp(v2 - 5, "anim") == 0;
}

bool __cdecl BG_IsADSField(const char *fieldName)
{
    return I_strnicmp(fieldName, "ads", 3) == 0;
}

char __cdecl BG_IsGripAnimationName(const char *name)
{
    int idx; // [esp+0h] [ebp-4h]

    for ( idx = 0; idx < s_numWeaponGripAnimSubstrings; ++idx )
    {
        if ( I_stristr(name, s_weaponGripAnimSubstrings[idx]) )
            return 1;
    }
    return 0;
}

int __cdecl BG_MergeWeaponDefAnimations(const char *fieldName, char **value, char *mergedValue, unsigned int size)
{
    char rightGrip; // [esp+1h] [ebp-3h]
    char leftGrip; // [esp+2h] [ebp-2h]
    bool isADS; // [esp+3h] [ebp-1h]

    leftGrip = BG_IsGripAnimationName(value[1]);
    rightGrip = BG_IsGripAnimationName(value[2]);
    isADS = BG_IsADSField(fieldName);
    if ( leftGrip && rightGrip )
    {
        Com_PrintError(
            1,
            "Can't merge anim field '%s', both '%s' and '%s' are bottom attachment point animations",
            fieldName,
            value[1],
            value[2]);
        return 0;
    }
    if ( !leftGrip && !rightGrip )
    {
        Com_PrintError(
            1,
            "Can't merge anim field '%s', neither '%s' nor '%s' are bottom attachment point animations",
            fieldName,
            value[1],
            value[2]);
        return 0;
    }
    if ( !leftGrip
        && !rightGrip
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\bgame\\bg_weapons_load_obj.cpp",
                    2564,
                    0,
                    "%s",
                    "leftGrip || rightGrip") )
    {
        __debugbreak();
    }
    if ( !isADS )
    {
        if ( !leftGrip )
            goto LABEL_15;
LABEL_16:
        strncpy(mergedValue, value[1], size);
        return 1;
    }
    if ( !leftGrip )
        goto LABEL_16;
LABEL_15:
    strncpy(mergedValue, value[2], size);
    return 1;
}

int __cdecl BG_MergeWeaponDefSpecialCases(const char *fieldName, char **value, char *mergedValue, int size)
{
    if ( I_stricmp(fieldName, "displayName") )
    {
        if ( I_stricmp(fieldName, "clipName") )
        {
            if ( I_stricmp(fieldName, "hideTags") )
            {
                if ( I_stricmp(fieldName, "adsViewKickCenterSpeed")
                    && I_stricmp(fieldName, "hipViewKickCenterSpeed")
                    && I_stricmp(fieldName, "hipViewKickYawMax") )
                {
                    if ( BG_IsAnimationField(fieldName) )
                        return BG_MergeWeaponDefAnimations(fieldName, value, mergedValue, size);
                    else
                        return 0;
                }
                else
                {
                    return BG_MergeWeaponDefFloatValue(value, mergedValue);
                }
            }
            else
            {
                return BG_MergeWeaponDefHidetags(value, mergedValue, size);
            }
        }
        else
        {
            return BG_MergeWeaponDefClipName(value, mergedValue, size);
        }
    }
    else
    {
        _snprintf(mergedValue, size, "%s_DUAL_ATTACH", *value);
        return 1;
    }
}

char __cdecl BG_LoadWeaponVariantDefFile(WeaponFullDef *weapFullDef, const char *folder, char *name)
{
    char *sourceName; // [esp+10h] [ebp-2847Ch] BYREF
    char *v5; // [esp+14h] [ebp-28478h]
    _BYTE *v6; // [esp+18h] [ebp-28474h]
    char *pszBuffer; // [esp+1Ch] [ebp-28470h] BYREF
    char *v8; // [esp+20h] [ebp-2846Ch]
    char *v9; // [esp+24h] [ebp-28468h]
    int v10; // [esp+28h] [ebp-28464h]
    char *sources[6]; // [esp+2Ch] [ebp-28460h] BYREF
    WeaponComponentList componentAll; // [esp+44h] [ebp-28448h] BYREF
    char szBuffer[16384]; // [esp+17Ch] [ebp-28310h] BYREF
    char v14; // [esp+417Ch] [ebp-24310h] BYREF
    _BYTE v15[64]; // [esp+1817Ch] [ebp-10310h] BYREF
    char v16[66116]; // [esp+181BCh] [ebp-102D0h] BYREF
    char **ppszConfigString; // [esp+28400h] [ebp-8Ch]
    char outputName[64]; // [esp+28404h] [ebp-88h] BYREF
    char dest[64]; // [esp+28444h] [ebp-48h] BYREF
    int i; // [esp+28488h] [ebp-4h]

    ppszConfigString = (char **)weapFullDef;
    for ( i = 0; i < 4; ++i )
    {
        v16[0x4000 * i + 320] = 0;
        v16[64 * i + 65856] = 0;
    }
    outputName[0] = 0;
    Com_sprintf(dest, 0x40u, "weapons/%s/%s", folder, name);
    if ( (int)FS_FOpenFileByMode(dest, 0, FS_READ) <= 0 )
    {
        for ( i = 0; i < 6; ++i )
            sources[i] = (char*)&v15[64 * i];
        if ( !BG_SplitWeaponDefNames(name, sources, &componentAll, outputName) )
            return 0;
        if ( componentAll.numComponents - 1 > 6
            && !Assert_MyHandler(
                        "C:\\projects_pc\\cod\\codsrc\\src\\bgame\\bg_weapons_load_obj.cpp",
                        2663,
                        0,
                        "%s",
                        "componentAll.numComponents - 1 <= MERGE_BASE_FILE_COUNT") )
        {
            __debugbreak();
        }
        for ( i = 0; i < componentAll.numComponents - 1; ++i )
        {
            Com_sprintf(dest, 0x40u, "weapons/%s/%s", folder, &v15[64 * i]);
            if ( !BG_LoadWeaponFile(dest, &szBuffer[0x4000 * i], 0x4000) )
            {
                Com_PrintError(
                    1,
                    "Can not create merged weapon %s.    Could not load base or attachment file %s",
                    outputName,
                    dest);
                return 0;
            }
        }
        SetConfigString(ppszConfigString, outputName);
        sourceName = (char*)v15;
        pszBuffer = szBuffer;
        v5 = v16;
        v8 = &v14;
        v10 = BG_WeaponComponentListCountAttachments(&componentAll);
        for ( i = 0; i < v10 - 1; ++i )
        {
            v6 = &v15[64 * i + 128];
            v9 = &szBuffer[0x4000 * i + 0x8000];
            if ( !ParseConfigStringToStructMerged(
                            (unsigned __int8 *)weapFullDef,
                            weaponDefFields,
                            748,
                            name,
                            (const char **)&pszBuffer,
                            (const char **)&sourceName,
                            &v16[0x4000 * i + 320],
                            43,
                            (int (__cdecl *)(unsigned __int8 *, const char *, const int, const int))BG_ParseWeaponDefSpecificFieldType,
                            SetConfigString2,
                            BG_MergeWeaponDefSpecialCases) )
                return 0;
            componentAll.numComponents = i + 3;
            BG_WeaponComponentListToName(&componentAll, &v16[64 * i + 65856], 64);
            strcat(&v16[64 * i + 65856], "_mp");
            v5 = &v16[64 * i + 65856];
            v8 = &v16[0x4000 * i + 320];
        }
    }
    else
    {
        if ( !BG_LoadWeaponFile(dest, szBuffer, 0x4000) )
            return 0;
        SetConfigString(ppszConfigString, name);
        if ( !ParseConfigStringToStruct(
                        (unsigned __int8 *)weapFullDef,
                        weaponDefFields,
                        748,
                        szBuffer,
                        43,
                        (int (__cdecl *)(unsigned __int8 *, const char *, const int, const int))BG_ParseWeaponDefSpecificFieldType,
                        SetConfigString2) )
            return 0;
    }
    return 1;
}

int __cdecl BG_ParseWeaponDefSpecificFieldType(WeaponFullDef *pStruct, const char *pValue, int iFieldType)
{
    unsigned __int16 StringOfSize; // ax
    unsigned __int16 v4; // ax
    unsigned __int16 LowercaseString; // ax
    unsigned __int16 v6; // ax
    int result; // eax
    char v8; // [esp+3h] [ebp-A9h]
    char *v9; // [esp+8h] [ebp-A4h]
    const char *v10; // [esp+Ch] [ebp-A0h]
    int v11; // [esp+10h] [ebp-9Ch]
    const char *pos; // [esp+48h] [ebp-64h] BYREF
    int numHideTags; // [esp+4Ch] [ebp-60h]
    WeaponFullDef *weapFullDef; // [esp+50h] [ebp-5Ch]
    int numNoteTrackMappings; // [esp+54h] [ebp-58h]
    WeaponVariantDef *weapVariantDef; // [esp+58h] [ebp-54h]
    char keyName[64]; // [esp+5Ch] [ebp-50h] BYREF
    int arrayIndex; // [esp+A0h] [ebp-Ch]
    const char *token; // [esp+A4h] [ebp-8h]
    WeaponDef *weapDef; // [esp+A8h] [ebp-4h]

    if ( !pStruct
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\bgame\\bg_weapons_load_obj.cpp", 1643, 0, "%s", "pStruct") )
    {
        __debugbreak();
    }
    if ( !pValue
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\bgame\\bg_weapons_load_obj.cpp", 1644, 0, "%s", "pValue") )
    {
        __debugbreak();
    }
    weapFullDef = pStruct;
    weapVariantDef = &pStruct->weapVariantDef;
    weapDef = &pStruct->weapDef;
    switch ( iFieldType )
    {
        case 15:
            arrayIndex = Weapon_GetStringArrayIndex(pValue, szWeapTypeNames, 8);
            if ( arrayIndex < 0 )
                Com_Error(ERR_DROP, "Unknown weapon type \"%s\" in '%s\"", pValue, weapVariantDef->szInternalName);
            weapDef->weapType = (weapType_t)arrayIndex;
            goto LABEL_104;
        case 16:
            arrayIndex = Weapon_GetStringArrayIndex(pValue, szWeapClassNames, 13);
            if ( arrayIndex < 0 )
                Com_Error(ERR_DROP, "Unknown weapon class \"%s\" in '%s\"", pValue, weapVariantDef->szInternalName);
            weapDef->weapClass = (weapClass_t)arrayIndex;
            goto LABEL_104;
        case 17:
            arrayIndex = Weapon_GetStringArrayIndex(pValue, szWeapOverlayReticleNames, 2);
            if ( arrayIndex < 0 )
                Com_Error(ERR_DROP, "Unknown weapon overlay reticle \"%s\" in '%s\"", pValue, weapVariantDef->szInternalName);
            weapDef->overlayReticle = (weapOverlayReticle_t)arrayIndex;
            goto LABEL_104;
        case 18:
            arrayIndex = Weapon_GetStringArrayIndex(pValue, penetrateTypeNames, 4);
            if ( arrayIndex < 0 )
                Com_Error(ERR_DROP, "Unknown penetration type \"%s\" in '%s\"", pValue, weapVariantDef->szInternalName);
            weapDef->penetrateType = (PenetrateType)arrayIndex;
            goto LABEL_104;
        case 19:
            arrayIndex = Weapon_GetStringArrayIndex(pValue, impactTypeNames_0, 16);
            if ( arrayIndex < 0 )
                Com_Error(ERR_DROP, "Unknown impact type \"%s\" in '%s\"", pValue, weapVariantDef->szInternalName);
            weapDef->impactType = (ImpactType)arrayIndex;
            goto LABEL_104;
        case 20:
            arrayIndex = Weapon_GetStringArrayIndex(pValue, szWeapStanceNames, 3);
            if ( arrayIndex < 0 )
                Com_Error(ERR_DROP, "Unknown weapon stance \"%s\" in '%s\"", pValue, weapVariantDef->szInternalName);
            weapDef->stance = (weapStance_t)arrayIndex;
            goto LABEL_104;
        case 21:
            arrayIndex = Weapon_GetStringArrayIndex(pValue, szProjectileExplosionNames, 10);
            if ( arrayIndex < 0 )
                Com_Error(ERR_DROP, "Unknown projectile explosion \"%s\" in '%s\"", pValue, weapVariantDef->szInternalName);
            weapDef->projExplosion = (weapProjExposion_t)arrayIndex;
            goto LABEL_104;
        case 22:
            arrayIndex = Weapon_GetStringArrayIndex(pValue, offhandClassNames, 5);
            if ( arrayIndex < 0 )
                Com_Error(ERR_DROP, "Unknown offhand class \"%s\" in '%s\"", pValue, weapVariantDef->szInternalName);
            weapDef->offhandClass = (OffhandClass)arrayIndex;
            goto LABEL_104;
        case 23:
            arrayIndex = Weapon_GetStringArrayIndex(pValue, offhandSlotNames, 5);
            if ( arrayIndex < 0 )
                Com_Error(ERR_DROP, "Unknown offhand slot \"%s\" in '%s\"", pValue, weapVariantDef->szInternalName);
            weapDef->offhandSlot = (OffhandSlot)arrayIndex;
            goto LABEL_104;
        case 24:
            arrayIndex = Weapon_GetStringArrayIndex(pValue, (const char **)g_playerAnimTypeNames, g_playerAnimTypeNamesCount);
            if ( arrayIndex < 0 )
                Com_Error(ERR_DROP, "Unknown player anim type \"%s\" in '%s\"", pValue, weapVariantDef->szInternalName);
            weapDef->playerAnimType = arrayIndex;
            goto LABEL_104;
        case 25:
            arrayIndex = Weapon_GetStringArrayIndex(pValue, activeReticleNames, 3);
            if ( arrayIndex < 0 )
                Com_Error(ERR_DROP, "Unknown active reticle type \"%s\" in '%s\"", pValue, weapVariantDef->szInternalName);
            weapDef->activeReticleType = (activeReticleType_t)arrayIndex;
            goto LABEL_104;
        case 26:
            arrayIndex = Weapon_GetStringArrayIndex(pValue, guidedMissileNames, 7);
            if ( arrayIndex < 0 )
                Com_Error(ERR_DROP, "Unknown guided missile type \"%s\" in '%s\"", pValue, weapVariantDef->szInternalName);
            weapDef->guidedMissileType = (guidedMissileType_t)arrayIndex;
            goto LABEL_104;
        case 27:
            weapDef->bounceSound = (const char **)BG_RegisterSurfaceTypeSounds(pValue);
            goto LABEL_104;
        case 28:
            arrayIndex = Weapon_GetStringArrayIndex(pValue, stickinessNames, 6);
            if ( arrayIndex < 0 )
                Com_Error(ERR_DROP, "Unknown weapon stickiness type \"%s\" in '%s\"", pValue, weapVariantDef->szInternalName);
            weapDef->stickiness = (WeapStickinessType)arrayIndex;
            goto LABEL_104;
        case 29:
            arrayIndex = Weapon_GetStringArrayIndex(pValue, rotateTypeNames, 3);
            if ( arrayIndex < 0 )
                Com_Error(ERR_DROP, "Unknown weapon rotate type \"%s\" in '%s\"", pValue, weapVariantDef->szInternalName);
            weapDef->rotateType = (WeapRotateType)arrayIndex;
            goto LABEL_104;
        case 30:
            arrayIndex = Weapon_GetStringArrayIndex(pValue, overlayInterfaceNames, 3);
            if ( arrayIndex < 0 )
                Com_Error(ERR_DROP, "Unknown weapon overlay interface \"%s\" in '%s\"", pValue, weapVariantDef->szInternalName);
            weapDef->overlayInterface = (WeapOverlayInteface_t)arrayIndex;
            goto LABEL_104;
        case 31:
            arrayIndex = Weapon_GetStringArrayIndex(pValue, szWeapInventoryTypeNames, 5);
            if ( arrayIndex < 0 )
                Com_Error(ERR_DROP, "Unknown weapon inventory type \"%s\" in '%s\"", pValue, weapVariantDef->szInternalName);
            weapDef->inventoryType = (weapInventoryType_t)arrayIndex;
            goto LABEL_104;
        case 32:
            arrayIndex = Weapon_GetStringArrayIndex(pValue, szWeapFireTypeNames, 7);
            if ( arrayIndex < 0 )
                Com_Error(ERR_DROP, "Unknown weapon fire type \"%s\" in '%s\"", pValue, weapVariantDef->szInternalName);
            weapDef->fireType = (weapFireType_t)arrayIndex;
            goto LABEL_104;
        case 33:
            arrayIndex = Weapon_GetStringArrayIndex(pValue, szWeapClipTypeNames, 6);
            if ( arrayIndex < 0 )
                Com_Error(ERR_DROP, "Unknown weapon clip type \"%s\" in '%s\"", pValue, weapVariantDef->szInternalName);
            weapDef->clipType = (weapClipType_t)arrayIndex;
            goto LABEL_104;
        case 34:
            arrayIndex = Weapon_GetStringArrayIndex(pValue, ammoCounterClipNames, 7);
            if ( arrayIndex < 0 )
                Com_Error(ERR_DROP, "Unknown weapon ammo counter clip \"%s\" in '%s\"", pValue, weapVariantDef->szInternalName);
            weapDef->ammoCounterClip = (ammoCounterClipType_t)arrayIndex;
            goto LABEL_104;
        case 35:
            arrayIndex = Weapon_GetStringArrayIndex(pValue, weapIconRatioNames, 3);
            if ( arrayIndex < 0 )
                Com_Error(ERR_DROP, "Unknown weapon hud icon ratio \"%s\" in '%s\"", pValue, weapVariantDef->szInternalName);
            weapDef->hudIconRatio = (weaponIconRatioType_t)arrayIndex;
            goto LABEL_104;
        case 36:
            arrayIndex = Weapon_GetStringArrayIndex(pValue, weapIconRatioNames, 3);
            if ( arrayIndex < 0 )
                Com_Error(ERR_DROP, "Unknown weapon ammo counter icon ratio \"%s\" in '%s\"", pValue, weapVariantDef->szInternalName);
            weapDef->ammoCounterIconRatio = (weaponIconRatioType_t)arrayIndex;
            goto LABEL_104;
        case 37:
            arrayIndex = Weapon_GetStringArrayIndex(pValue, weapIconRatioNames, 3);
            if ( arrayIndex < 0 )
                Com_Error(ERR_DROP, "Unknown weapon kill icon ratio \"%s\" in '%s\"", pValue, weapVariantDef->szInternalName);
            weapDef->killIconRatio = (weaponIconRatioType_t)arrayIndex;
            goto LABEL_104;
        case 38:
            arrayIndex = Weapon_GetStringArrayIndex(pValue, weapIconRatioNames, 3);
            if ( arrayIndex < 0 )
                Com_Error(ERR_DROP, "Unknown weapon dpad icon ratio \"%s\" in '%s\"", pValue, weapVariantDef->szInternalName);
            weapVariantDef->dpadIconRatio = (weaponIconRatioType_t)arrayIndex;
            goto LABEL_104;
        case 39:
            arrayIndex = Weapon_GetStringArrayIndex(pValue, weapIconRatioNames, 3);
            if ( arrayIndex < 0 )
                Com_Error(ERR_DROP, "Unknown weapon indicator icon ratio \"%s\" in '%s\"", pValue, weapVariantDef->szInternalName);
            weapDef->indicatorIconRatio = (weaponIconRatioType_t)arrayIndex;
            goto LABEL_104;
        case 40:
            numHideTags = 0;
            pos = pValue;
            while ( 1 )
            {
                token = (const char *)Com_Parse(&pos);
                if ( !pos )
                    break;
                if ( numHideTags >= 32 )
                    Com_Error(ERR_DROP, "maximum hide tags (%s) exceeded: %i > %i", token, numHideTags, 32);
                StringOfSize = SL_GetStringOfSize(SCRIPTINSTANCE_SERVER, (char *)token, 0, strlen(token) + 1, 10);
                weapVariantDef->hideTags[numHideTags] = StringOfSize;
                weapVariantDef->hideTags[numHideTags] = SL_ConvertToLowercase(
                                                                                                    SCRIPTINSTANCE_SERVER,
                                                                                                    weapVariantDef->hideTags[numHideTags],
                                                                                                    0,
                                                                                                    10);
                ++numHideTags;
            }
            goto LABEL_104;
        case 41:
            pos = pValue;
            token = (const char *)Com_Parse(&pos);
            if ( pos )
            {
                v4 = SL_GetStringOfSize(SCRIPTINSTANCE_SERVER, (char *)token, 0, strlen(token) + 1, 10);
                weapDef->explosionTag = v4;
                weapDef->explosionTag = SL_ConvertToLowercase(SCRIPTINSTANCE_SERVER, weapDef->explosionTag, 0, 10);
            }
            goto LABEL_104;
        case 42:
            numNoteTrackMappings = 0;
            pos = pValue;
            keyName[0] = 0;
            while ( 1 )
            {
                token = (const char *)Com_Parse(&pos);
                if ( !pos )
                    break;
                if ( numNoteTrackMappings >= 20 )
                    Com_Error(ERR_DROP, "Max notetrack-to-sound mappings (%i) exceeded with entry \"%s\".", 20, token);
                if ( keyName[0] )
                {
                    LowercaseString = SL_GetLowercaseString(keyName, 0, SCRIPTINSTANCE_SERVER);
                    weapFullDef->notetrackSoundMapKeys[numNoteTrackMappings] = LowercaseString;
                    v6 = SL_GetLowercaseString(token, 0, SCRIPTINSTANCE_SERVER);
                    weapFullDef->notetrackSoundMapValues[numNoteTrackMappings++] = v6;
                    keyName[0] = 0;
                }
                else
                {
                    v11 = strlen(token);
                    if ( v11 >= 63 )
                        Com_Error(ERR_DROP, "Noetrack-to-sopund: keyname \"%s\" is too long (length %i/%i).", token, v11, 63);
                    v10 = token;
                    v9 = keyName;
                    do
                    {
                        v8 = *v10;
                        *v9++ = *v10++;
                    }
                    while ( v8 );
                }
            }
            if ( keyName[0] )
                Com_PrintWarning(
                    0,
                    "Notetrack-to-Sound: Weapon '%s' has bad entry; notetrack '%s' doesn't have a corresponding sound.\n",
                    weapVariantDef->szInternalName,
                    keyName);
LABEL_104:
            result = 1;
            break;
        default:
            Com_Error(ERR_DROP, "Bad field type %i in %s", iFieldType, weapVariantDef->szInternalName);
            result = 0;
            break;
    }
    return result;
}

int __cdecl Weapon_GetStringArrayIndex(const char *value, const char **stringArray, int arraySize)
{
    int arrayIndex; // [esp+0h] [ebp-4h]

    if ( !value
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\bgame\\bg_weapons_load_obj.cpp", 1561, 0, "%s", "value") )
    {
        __debugbreak();
    }
    if ( !stringArray
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\bgame\\bg_weapons_load_obj.cpp",
                    1562,
                    0,
                    "%s",
                    "stringArray") )
    {
        __debugbreak();
    }
    for ( arrayIndex = 0; arrayIndex < arraySize; ++arrayIndex )
    {
        if ( !I_stricmp(value, stringArray[arrayIndex]) )
            return arrayIndex;
    }
    return -1;
}

char **__cdecl BG_RegisterSurfaceTypeSounds(const char *surfaceSoundBase)
{
    const char *v2; // eax
    unsigned __int8 *v3; // eax
    char v4; // [esp+3h] [ebp-149h]
    char *v5; // [esp+8h] [ebp-144h]
    const char *v6; // [esp+Ch] [ebp-140h]
    char v7; // [esp+23h] [ebp-129h]
    char *v8; // [esp+28h] [ebp-124h]
    char *v9; // [esp+2Ch] [ebp-120h]
    char **result; // [esp+40h] [ebp-10Ch]
    char aliasName[256]; // [esp+44h] [ebp-108h] BYREF
    int i; // [esp+148h] [ebp-4h]

    if (!surfaceSoundBase
        && !Assert_MyHandler(
            "C:\\projects_pc\\cod\\codsrc\\src\\bgame\\bg_weapons_load_obj.cpp",
            1584,
            0,
            "%s",
            "surfaceSoundBase"))
    {
        __debugbreak();
    }
    if (!*surfaceSoundBase)
        return 0;
    for (i = 0; i < surfaceTypeSoundListCount; ++i)
    {
        if (!I_strcmp(surfaceTypeSoundLists[i].surfaceSoundBase, surfaceSoundBase))
            return surfaceTypeSoundLists[i].soundAliasList;
    }
    if (surfaceTypeSoundListCount == 16)
        Com_Error(ERR_DROP, "Exceeded MAX_SURFACE_TYPE_SOUND_LISTS (%d)", 16);
    result = (char **)Hunk_AllocLow(31 * (unsigned int)sizeof(char *), "BG_RegisterSurfaceTypeSounds", 17);
    for (i = 0; i < 31; ++i)
    {
        v2 = Com_SurfaceTypeToName(i);
        Com_sprintf(aliasName, 0x100u, "%s_%s", surfaceSoundBase, v2);
        v3 = Hunk_AllocLow(&aliasName[strlen(aliasName) + 1] - &aliasName[1] + 1, "BG_RegisterSurfaceTypeSounds", 17);
        result[i] = (char *)v3;
        v9 = aliasName;
        v8 = result[i];
        do
        {
            v7 = *v9;
            *v8++ = *v9++;
        } while (v7);
    }
    surfaceTypeSoundLists[surfaceTypeSoundListCount].surfaceSoundBase = (char *)Hunk_AllocLow(
        strlen(surfaceSoundBase) + 1,
        "BG_RegisterSurfaceTypeSounds",
        17);
    v6 = surfaceSoundBase;
    v5 = surfaceTypeSoundLists[surfaceTypeSoundListCount].surfaceSoundBase;
    do
    {
        v4 = *v6;
        *v5++ = *v6++;
    } while (v4);
    surfaceTypeSoundLists[surfaceTypeSoundListCount++].soundAliasList = result;
    return result;
}

WeaponFullDef *__cdecl BG_LoadWeaponVariantDefInternal(const char *folder, char *name)
{
    WeaponFullDef *weapFullDef; // [esp+8h] [ebp-Ch]
    WeaponDef *weapDef; // [esp+10h] [ebp-4h]

    weapFullDef = (WeaponFullDef *)Hunk_AllocLow(sizeof(WeaponFullDef), "BG_LoadWeaponVariantDefInternal", 10);
    InitWeaponDef(weapFullDef);
    weapDef = &weapFullDef->weapDef;
    if (!BG_LoadWeaponVariantDefFile(weapFullDef, folder, name))
        return 0;
    weapFullDef->weapDef.flameTableFirstPersonPtr = 0;
    weapFullDef->weapDef.flameTableThirdPersonPtr = 0;
    bg_nextParseFlameTableIndex = 0;
    if (*weapFullDef->weapDef.flameTableFirstPerson)
        weapFullDef->weapDef.flameTableFirstPersonPtr = BG_GetFlameTable(
            folder,
            (char *)weapFullDef->weapDef.flameTableFirstPerson);
    if (*weapFullDef->weapDef.flameTableThirdPerson)
        weapFullDef->weapDef.flameTableThirdPersonPtr = BG_GetFlameTable(
            folder,
            (char *)weapFullDef->weapDef.flameTableThirdPerson);
    if (I_stricmp(name, "defaultweapon_mp"))
    {
        if (!weapFullDef->weapDef.viewLastShotEjectEffect)
            weapFullDef->weapDef.viewLastShotEjectEffect = weapFullDef->weapDef.viewShellEjectEffect;
        if (!weapFullDef->weapDef.worldLastShotEjectEffect)
            weapFullDef->weapDef.worldLastShotEjectEffect = weapFullDef->weapDef.worldShellEjectEffect;
        if (!weapFullDef->weapDef.raiseSound)
            weapFullDef->weapDef.raiseSound = "wpn_default_raise";
        if (!weapFullDef->weapDef.putawaySound)
            weapFullDef->weapDef.putawaySound = "wpn_default_putaway";
        if (!weapFullDef->weapDef.pickupSound)
            weapFullDef->weapDef.pickupSound = "wpn_default_pickup";
        if (!weapFullDef->weapDef.ammoPickupSound)
            weapFullDef->weapDef.ammoPickupSound = "wpn_default_ammo_pickup";
        if (!weapFullDef->weapDef.emptyFireSound)
            weapFullDef->weapDef.emptyFireSound = "wpn_default_no_ammo";
    }
    BG_SetupTransitionTimes(&weapFullDef->weapVariantDef);
    BG_CheckWeaponDamageRanges(weapDef);
    if (weapFullDef->weapDef.enemyCrosshairRange > 15000.0)
        Com_Error(ERR_DROP, "Enemy crosshair ranges should be less than %f ", 15000.0);
    if (weapFullDef->weapDef.weapType == WEAPTYPE_PROJECTILE)
        BG_CheckProjectileValues(weapFullDef);
    if (!G_ParseWeaponAccurayGraphs(weapDef))
        return 0;
    if (weapFullDef->weapDef.sharedAmmo)
    {
        I_strlwr((char *)weapFullDef->weapVariantDef.szAmmoName);
        I_strlwr((char *)weapFullDef->weapVariantDef.szClipName);
    }
    else
    {
        weapFullDef->weapVariantDef.szAmmoName = "";
        weapFullDef->weapVariantDef.szClipName = "";
    }
    //BG_EvalVehicleName();
    return weapFullDef;
}

void __cdecl BG_SetupTransitionTimes(WeaponVariantDef *weapVariantDef)
{
    if ( weapVariantDef->iAdsTransInTime <= 0 )
        weapVariantDef->fOOPosAnimLength[0] = 1.0 / 300.0;
    else
        weapVariantDef->fOOPosAnimLength[0] = 1.0 / (float)weapVariantDef->iAdsTransInTime;
    if ( weapVariantDef->iAdsTransOutTime <= 0 )
        weapVariantDef->fOOPosAnimLength[1] = 1.0 / 500.0;
    else
        weapVariantDef->fOOPosAnimLength[1] = 1.0 / (float)weapVariantDef->iAdsTransOutTime;
}

void __cdecl BG_CheckWeaponDamageRanges(WeaponDef *weapDef)
{
    if ( weapDef->fMaxDamageRange <= 0.0 )
        weapDef->fMaxDamageRange = 999999.0f;
    if ( weapDef->fMinDamageRange <= 0.0 )
        weapDef->fMinDamageRange = 999999.12f;
}

void __cdecl BG_CheckProjectileValues(WeaponFullDef *weaponFullDef)
{
    if ( weaponFullDef->weapDef.weapType != WEAPTYPE_PROJECTILE
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\bgame\\bg_weapons_load_obj.cpp",
                    1941,
                    0,
                    "%s",
                    "weaponDef->weapType == WEAPTYPE_PROJECTILE") )
    {
        __debugbreak();
    }
    if ( (float)weaponFullDef->weapDef.iProjectileSpeed <= 0.0 )
        Com_Error(
            ERR_DROP,
            "Projectile speed for WeapType %s must be greater than 0.0",
            weaponFullDef->weapVariantDef.szDisplayName);
    if ( weaponFullDef->weapDef.destabilizationCurvatureMax >= 1000000000.0
        || weaponFullDef->weapDef.destabilizationCurvatureMax < 0.0 )
    {
        Com_Error(
            ERR_DROP,
            "Destabilization angle for for WeapType %s must be between 0 and 45 degrees",
            weaponFullDef->weapVariantDef.szDisplayName);
    }
    if ( weaponFullDef->weapDef.destabilizationRateTime < 0.0 )
        Com_Error(
            ERR_DROP,
            "Destabilization rate time for for WeapType %s must be non-negative",
            weaponFullDef->weapVariantDef.szDisplayName);
}

void __cdecl InitWeaponDef(WeaponFullDef *weapFullDef)
{
    const cspField_t *pField; // [esp+8h] [ebp-Ch]
    int iField; // [esp+Ch] [ebp-8h]

    weapFullDef->weapVariantDef.weapDef = &weapFullDef->weapDef;
    weapFullDef->weapVariantDef.szInternalName = "";
    weapFullDef->weapVariantDef.szXAnims = weapFullDef->szXAnims;
    weapFullDef->weapVariantDef.hideTags = weapFullDef->hideTags;
    weapFullDef->weapDef.gunXModel = weapFullDef->gunXModel;
    weapFullDef->weapDef.notetrackSoundMapKeys = weapFullDef->notetrackSoundMapKeys;
    weapFullDef->weapDef.notetrackSoundMapValues = weapFullDef->notetrackSoundMapValues;
    weapFullDef->weapDef.worldModel = weapFullDef->worldModel;
    weapFullDef->weapDef.parallelBounce = weapFullDef->parallelBounce;
    weapFullDef->weapDef.perpendicularBounce = weapFullDef->perpendicularBounce;
    weapFullDef->weapDef.locationDamageMultipliers = weapFullDef->locationDamageMultipliers;
    iField = 0;
    pField = weaponDefFields;
    while ( iField < 748 )
    {
        if ( !pField->iFieldType )
            *(const char **)((char *)&weapFullDef->weapVariantDef.szInternalName + pField->iOffset) = "";
        ++iField;
        ++pField;
    }
}

// The config table addresses inline arrays in WeaponFullDef. Snapshot their
// backing storage as well as the two headers, then keep temporary pointers local.
static void BG_CopyWeaponFullDefForUpdate(WeaponFullDef *copy, const WeaponVariantDef *source)
{
    memset(copy, 0, sizeof(*copy));
    copy->weapVariantDef = *source;
    copy->weapDef = *source->weapDef;
    // Variant members live directly in source; handle them explicitly.
    if (source->szXAnims)
        memcpy(copy->szXAnims, source->szXAnims, sizeof(copy->szXAnims));
    if (source->hideTags)
        memcpy(copy->hideTags, source->hideTags, sizeof(copy->hideTags));
    copy->weapVariantDef.szXAnims = source->szXAnims ? copy->szXAnims : nullptr;
    copy->weapVariantDef.hideTags = source->hideTags ? copy->hideTags : nullptr;
#define COPY_DEF_ARRAY(member) \
    if (source->weapDef->member) \
        memcpy(copy->member, source->weapDef->member, sizeof(copy->member)); \
    copy->weapDef.member = source->weapDef->member ? copy->member : nullptr
    COPY_DEF_ARRAY(gunXModel);
    COPY_DEF_ARRAY(notetrackSoundMapKeys);
    COPY_DEF_ARRAY(notetrackSoundMapValues);
    COPY_DEF_ARRAY(worldModel);
    COPY_DEF_ARRAY(parallelBounce);
    COPY_DEF_ARRAY(perpendicularBounce);
    COPY_DEF_ARRAY(locationDamageMultipliers);
#undef COPY_DEF_ARRAY
    copy->weapVariantDef.weapDef = &copy->weapDef;
}

static void BG_CommitWeaponFullDefUpdate(WeaponVariantDef *target, WeaponFullDef *copy)
{
    WeaponDef *targetDef = target->weapDef;
    if (target->szXAnims)
        memcpy(target->szXAnims, copy->szXAnims, sizeof(copy->szXAnims));
    if (target->hideTags)
        memcpy(target->hideTags, copy->hideTags, sizeof(copy->hideTags));
    copy->weapVariantDef.szXAnims = target->szXAnims;
    copy->weapVariantDef.hideTags = target->hideTags;
#define COMMIT_DEF_ARRAY(member) \
    if (targetDef->member) \
        memcpy(targetDef->member, copy->member, sizeof(copy->member)); \
    copy->weapDef.member = targetDef->member
    COMMIT_DEF_ARRAY(gunXModel);
    COMMIT_DEF_ARRAY(notetrackSoundMapKeys);
    COMMIT_DEF_ARRAY(notetrackSoundMapValues);
    COMMIT_DEF_ARRAY(worldModel);
    COMMIT_DEF_ARRAY(parallelBounce);
    COMMIT_DEF_ARRAY(perpendicularBounce);
    COMMIT_DEF_ARRAY(locationDamageMultipliers);
#undef COMMIT_DEF_ARRAY
    copy->weapVariantDef.weapDef = targetDef;
    *targetDef = copy->weapDef;
    *target = copy->weapVariantDef;
}

char __cdecl BG_WeaponUpdateField(const char *weaponName, char *keyValue)
{
    WeaponFullDef weapFullDef; // [esp+8h] [ebp-C58h] BYREF
    WeaponVariantDef *weapVariantDef; // [esp+C58h] [ebp-8h]
    int w; // [esp+C5Ch] [ebp-4h]

    w = BG_FindWeaponIndexForName(weaponName);
    if ( w )
    {
        weapVariantDef = (WeaponVariantDef *)BG_GetWeaponVariantDef(w);
        BG_CopyWeaponFullDefForUpdate(&weapFullDef, weapVariantDef);
        if ( !ParseConfigStringToStruct(
                        (unsigned __int8 *)&weapFullDef,
                        weaponDefFields,
                        748,
                        keyValue,
                        43,
                        (int (__cdecl *)(unsigned __int8 *, const char *, const int, const int))BG_ParseWeaponDefSpecificFieldType,
                        SetConfigString2) )
            return 0;
        BG_CommitWeaponFullDefUpdate(weapVariantDef, &weapFullDef);
        BG_SetupTransitionTimes(weapVariantDef);
        BG_CheckWeaponDamageRanges(weapVariantDef->weapDef);
    }
    return 1;
}

bool __cdecl BG_FlameTableUpdateField(const char *flameTableName, char *keyValue)
{
    flameTable *ft; // [esp+0h] [ebp-4h]

    ft = Flame_FindFlameTable(flameTableName);
    return !ft
            || ParseConfigStringToStruct((unsigned __int8 *)ft, flameTableFields, 119, keyValue, 0, 0, SetConfigString2);
}

WeaponVariantDef *__cdecl BG_LoadDefaultWeaponVariantDef()
{
    if ( useFastFile->current.enabled )
        return (WeaponVariantDef *)BG_LoadDefaultWeaponVariantDef_FastFile();
    else
        return (WeaponVariantDef *)BG_LoadDefaultWeaponVariantDef_LoadObj();
}

WeaponFullDef *__cdecl BG_LoadDefaultWeaponVariantDef_LoadObj()
{
    InitWeaponDef(&bg_defaultWeaponFullDefs);
    bg_defaultWeaponFullDefs.weapVariantDef.weapDef = &bg_defaultWeaponFullDefs.weapDef;
    bg_defaultWeaponFullDefs.weapVariantDef.weapDef->playerAnimType = Weapon_GetStringArrayIndex(
                                                                                                                                            "default",
                                                                                                                                            (const char **)g_playerAnimTypeNames,
                                                                                                                                            g_playerAnimTypeNamesCount);
    if ( bg_defaultWeaponFullDefs.weapVariantDef.weapDef->playerAnimType < 0 )
        bg_defaultWeaponFullDefs.weapVariantDef.weapDef->playerAnimType = 0;
    bg_defaultWeaponFullDefs.weapVariantDef.szInternalName = "none";
    bg_defaultWeaponFullDefs.weapDef.accuracyGraphName[0] = "noweapon.accu";
    bg_defaultWeaponFullDefs.weapDef.accuracyGraphName[1] = "noweapon.accu";
    bg_defaultWeaponFullDefs.weapDef.sprintDurationScale = 1.75f;
    G_ParseWeaponAccurayGraphs(&bg_defaultWeaponFullDefs.weapDef);
    return &bg_defaultWeaponFullDefs;
}

WeaponVariantDef *__cdecl BG_LoadDefaultWeaponVariantDef_FastFile()
{
    return DB_FindXAssetHeader(ASSET_TYPE_WEAPON, (char*)"none", 1, -1).weapon;
}

