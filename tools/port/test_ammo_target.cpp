#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#define ARRAY_COUNT(x) (sizeof(x) / sizeof((x)[0]))
static void require(bool value, const char *why) {
    if (!value) { fprintf(stderr, "FAIL: %s\n", why); exit(1); }
}
// PRODUCTION_TYPES
struct playerState_s { unsigned weapon, perks[2]; struct { unsigned weapon; } heldWeapons[15]; };
struct gclient_s { playerState_s ps; struct { struct { struct { int score; } score; } cs; } sess; };
struct gentity_s { gclient_s *client; struct { int number; } s; struct { bool inuse; float currentOrigin[3]; } r; const char *targetname, *target; };
gentity_s g_entities[2];
struct { int time = 1, num_entities = 0; } level;
std::vector<int> s_plan;
int s_index, s_stepStart, s_ent, s_phase, s_phaseTime, s_startScore, s_startAmmo;
int s_spent, s_largestDrop, s_lastScore, s_uses, s_lastUse, s_defconUseTime;
bool s_ridePanelUsed;
int s_rideCar, s_spins, s_stepShots, s_shots, s_fireDec, s_fireDt;
int s_intervalSum, s_intervalCount, s_reloadSum, s_reloadCount, s_pathGoal, s_progressTime;
unsigned s_startWeapons[15];
float s_startOrigin[3], s_progressOrigin[3];
int s_teleportTime, s_kinoOutTime, s_kinoReturnTime;
enum { F_PHONE, F_TRAPPIECE, F_DEFCON, F_TRAPINUSE, F_INUSE };
unsigned measuredWeapon;
int ammo[512];
unsigned BG_FindWeaponIndexForName(const char *name) {
    if (!strcmp(name, "pm63_zm")) return 27;
    if (!strcmp(name, "rottweil72_zm")) return 38;
    if (!strcmp(name, "m14_zm")) return 24;
    if (!strcmp(name, "mp5k_zm")) return 40;
    return 0;
}
const char *BG_WeaponName(unsigned) { return "fixture_weapon"; }
int TotalAmmo(const playerState_s *, unsigned weapon) { measuredWeapon = weapon; return ammo[weapon]; }
std::string WeaponList(const playerState_s *) { return "fixture_weapons"; }
void Vec3Copy(const float *a, float *b) { memcpy(b, a, 12); }
int Floor(float) { return 2; }
void Com_Printf(int, const char *, ...) {}
void Log(const char *, const char *) {}
int LevelInt(int, int) { return 0; }
int NotifyTime(const char *, int) { return -1; }
bool G_SP_HasPerk(gclient_s *, const char *) { return false; }
unsigned BG_GetPerkIndexForName(const char *) { return 0; }
float Dvar_GetFloat(const char *) { return 1; }
bool HasWeaponNamed(const playerState_s *, const char *) { return false; }
bool WeaponsChanged(const playerState_s *) { return false; }
bool EntField(int, int, void *, int *) { return false; }
bool DefconSwitchReady(const gentity_s *) { return false; }
const char *Str(const char *s) { return s ? s : ""; }
// PRODUCTION_FUNCTIONS

static int index(const char *name) {
    for (int i = 0; i < int(ARRAY_COUNT(s_steps)); ++i) if (!strcmp(s_steps[i].name, name)) return i;
    require(false, "real step missing"); return 0;
}
int main() {
    gclient_s client{}; gentity_s player{}; player.client = &client;
    for (const char *name : {"ammo_pm63", "ammo_rottweil", "ammo_m14", "ammo_mp5k"}) {
        const PlanStep &step = s_steps[index(name)];
        const unsigned target = BG_FindWeaponIndexForName(step.value);
        for (unsigned raised : {target, 200u, 201u}) {
            for (unsigned &value : s_startWeapons) value = 0;
            memset(ammo, 0, sizeof(ammo)); ammo[target] = 30;
            if (raised != target) ammo[raised] = raised == 200 ? 376 : 0;
            client.ps.weapon = raised; client.sess.cs.score.score = 11640;
            s_plan = {index(name)}; s_index = 0; StartStep(&player);
            require(measuredWeapon == target && s_startAmmo == 30, "baseline must measure purchased gun with Death Machine/empty gun raised");
            const char *why = nullptr;
            client.sess.cs.score.score -= 500;
            require(!CheckUseSuccess(&player, step, &why), "payment without target ammo increase must fail");
            if (raised != target) {
                ammo[raised] += 50;
                require(!CheckUseSuccess(&player, step, &why), "another gun gaining ammo must fail");
            }
            ammo[target] = 100; client.sess.cs.score.score = s_startScore;
            require(!CheckUseSuccess(&player, step, &why), "ammo increase without payment must fail");
            client.sess.cs.score.score -= 500;
            require(CheckUseSuccess(&player, step, &why) && why && !strcmp(why, "ammo"), "paid target refill must pass with another gun raised");
        }
    }
    puts("PASS: production ammo baseline/payment/refill select the target gun across 12 scenarios");
}
