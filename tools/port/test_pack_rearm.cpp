#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <initializer_list>
#define ARRAY_COUNT(x) (sizeof(x) / sizeof((x)[0]))
void require(bool value, const char *why) {
    if (!value) { fprintf(stderr, "FAIL: %s\n", why); exit(1); }
}
enum { STEP_TOUCH, STEP_USE, PLAN_INSERTED = 1, PLAN_ARMED = 2, F_DEFCON = 0, F_BONFIRE = 1 };
struct PlanStep { int kind; const char *name; const char *notify; };
PlanStep s_steps[] = {{STEP_TOUCH, "portal_pack", "open_pack_hideaway"},
                     {STEP_USE, "defcon", nullptr}, {STEP_USE, "floor2", nullptr},
                     {STEP_TOUCH, "portal", nullptr}};
std::vector<int> s_plan;
std::vector<unsigned char> s_planFlags;
std::vector<unsigned> s_planReplace;
int s_index, s_recoveryFails, defconLevel, bonfire;
struct gentity_s { struct { bool inuse; float currentOrigin[3]; } r; const char *targetname; };
gentity_s g_entities[2];
struct { int time = 1, maxclients = 1, num_entities = 1; } level;
int LevelInt(int, int) { return defconLevel; }
int ZombieVar(int) { return bonfire; }
const char *Str(const char *s) { return s ? s : ""; }
int Floor(float z) { return z < -600 ? 3 : z < -400 ? 2 : 1; }
void Center(gentity_s *p, float *center) { memcpy(center, p->r.currentOrigin, 12); }
void Com_Printf(int, const char *, ...) {}
// PRODUCTION_FUNCTIONS

static void reset(int next = 0) {
    s_plan = {next}; s_planFlags = {PLAN_ARMED}; s_planReplace = {0};
    s_index = 0; s_recoveryFails = 0; bonfire = 0;
    level.num_entities = 1;
}
int main() {
    gentity_s player{}; player.r.currentOrigin[2] = -512;
    reset(); defconLevel = 1;
    PackCheck(&player);
    require(s_plan.size() == 5 && s_plan.back() == 0, "four pulls precede the original portal");
    require(!(s_planFlags.back() & PLAN_ARMED), "portal must recheck after inserted pulls");
    for (int i = 0; i < 4; ++i)
        require(s_plan[i] == 1 && (s_planFlags[i] & PLAN_INSERTED), "inserted pulls retain recovery classification");
    // Reproduce the failed route: a thief/Bonfire resets DEFCON during the pulls,
    // and the last pull reaches only level 2. The deferred portal must add three more.
    s_index = 4; defconLevel = 2;
    if (!(s_planFlags[s_index] & PLAN_ARMED)) {
        s_planFlags[s_index] |= PLAN_ARMED;
        PackCheck(&player);
    }
    require(s_plan.size() == 8 && !(s_planFlags.back() & PLAN_ARMED), "reset during dependencies rearms instead of entering wrong portal");
    s_index = 7; defconLevel = 5;
    s_planFlags[s_index] |= PLAN_ARMED;
    PackCheck(&player);
    require(s_plan.size() == 8 && (s_planFlags.back() & PLAN_ARMED), "ready portal adds no unnecessary pulls");
    for (int value : {-1, 0, 5, 6}) {
        reset(); defconLevel = value; PackCheck(&player);
        require(s_plan.size() == 1, "unknown or ready level does not insert pulls");
    }
    reset(); defconLevel = 1; bonfire = 1; PackCheck(&player);
    require(s_plan.size() == 1, "active Bonfire Sale uses existing readiness policy");
    reset(); defconLevel = 1; s_recoveryFails = 2; PackCheck(&player);
    require(s_plan.size() == 1, "existing failure cap retained");
    reset(3); defconLevel = 1; PackCheck(&player);
    require(s_plan.size() == 1, "ordinary portal is unaffected");
    reset(); defconLevel = 4; player.r.currentOrigin[2] = -712;
    level.num_entities = 2; g_entities[1].r.inuse = true;
    g_entities[1].targetname = "punch_switch"; g_entities[1].r.currentOrigin[2] = -512;
    PackCheck(&player);
    require(s_plan.size() == 3 && s_plan[0] == 2 && s_plan[1] == 1 && s_plan[2] == 0,
            "ride precedes pulls when switches are on another floor");
    require(!(s_planFlags[2] & PLAN_ARMED), "portal remains deferred after floor recovery too");
    puts("PASS: production PackCheck reset recovery, readiness, failure cap and floor ordering");
}
