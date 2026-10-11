#include "g_sp_weapon_sweep.h"
#include <game_mp/g_main_mp.h>
#include <game_mp/g_client_script_cmd_mp.h>
#include <client_mp/sv_client_mp.h>
#include <server_mp/sv_bot_mp.h>
#include <bgame/bg_weapons.h>
#include <bgame/bg_weapons_def.h>
#include <bgame/bg_weapons_ammo.h>
#include <bgame/bg_misc.h>
#include <game/g_weapon.h>
#include <game/actor.h>
#include <game_mp/actor_mp.h>
#include <clientscript/cscr_vm.h>
#include <clientscript/cscr_stringlist.h>
#include <clientscript/cscr_variable.h>
#include <clientscript/cscr_compiler.h>
#include <clientscript/scr_const.h>
#include <universal/dvar.h>
#include <universal/com_math.h>
#include <qcommon/common.h>
#include <qcommon/cmd.h>
#include <cstring>
#include <vector>
#include <string>

extern bot_info_t botInfos[32];
namespace {
const dvar_s *s_mode;
struct Entry { unsigned int give, hold; };
std::vector<Entry> s_entries;
int s_index, s_start, s_shots, s_throws, s_melees, s_pass, s_fail, s_player;
unsigned int s_previous, s_pistol;
bool s_built, s_done, s_given, s_raised, s_ads, s_reload, s_swapped;
int s_reloadClip, s_lastReport, s_startReserve, s_hits, s_kills;
unsigned int s_weaponField, s_upgradeField;
unsigned int Field(unsigned int object, const char *name, bool array=false) {
    const unsigned int string=SL_FindString(name,SCRIPTINSTANCE_SERVER);
    const unsigned int key=array ? string : !strcmp(name,"zombie_weapons") ? s_weaponField : s_upgradeField;
    return object && key ? FindVariable(SCRIPTINSTANCE_SERVER,object,key) : 0;
}
unsigned int Object(unsigned int variable) {
    return variable && GetValueType(SCRIPTINSTANCE_SERVER,variable)==VAR_POINTER
        ? GetVariableValueAddress(SCRIPTINSTANCE_SERVER,variable)->u.pointerValue : 0;
}
bool CombatName(const char *name) {
    return name && (strstr(name, "_zm") || !strncmp(name, "zombie_", 7));
}
unsigned int Parent(unsigned int w) {
    if (BG_GetWeaponDef(w)->inventoryType != WEAPINVENTORY_ALTMODE) return w;
    for (unsigned int p=1; p<BG_GetNumWeapons(); ++p)
        if (BG_GetWeaponVariantDef(p)->altWeaponIndex == w
            && BG_GetWeaponDef(p)->inventoryType == WEAPINVENTORY_PRIMARY) return p;
    return 0;
}
bool Candidate(unsigned int w) {
    const WeaponDef *d=BG_GetWeaponDef(w);
    const char *n=BG_WeaponName(w);
    return CombatName(n) && d->weapClass != WEAPCLASS_NON_PLAYER && d->weapClass != WEAPCLASS_TURRET
        && !strstr(n, "lh_") && !strstr(n, "_left")
        && strncmp(n,"explosive_bolt",14) && strcmp(n,"zombie_nesting_doll_single")
        && strncmp(n,"sniper_explosive_bolt",21)
        && (d->inventoryType!=WEAPINVENTORY_ITEM || !strcmp(n,"claymore_zm"))
        && (d->damage > 0 || d->weapType != WEAPTYPE_BULLET);
}
void Build() {
    s_built=true;
    const char *map=Dvar_GetString("mapname");
    const unsigned int weapons=Object(Field(gScrVarPub[SCRIPTINSTANCE_SERVER].levelId,"zombie_weapons"));
    for (unsigned int w=1; w<BG_GetNumWeapons(); ++w) {
        const WeaponDef *d=BG_GetWeaponDef(w);
        const WeaponVariantDef *v=BG_GetWeaponVariantDef(w);
        const unsigned int parent=Parent(w);
        const unsigned int data=Object(Field(weapons,v->szInternalName,true));
        const unsigned int upgrade=Field(data,"upgrade_name");
        if(upgrade && GetValueType(SCRIPTINSTANCE_SERVER,upgrade)==VAR_STRING)
            Com_Printf(16,"bo1_sweep_mapping: map=%s base=%s upgrade=%s\n",map,v->szInternalName,
                SL_ConvertToString(GetVariableValueAddress(SCRIPTINSTANCE_SERVER,upgrade)->u.stringValue,SCRIPTINSTANCE_SERVER));
        Com_Printf(16,"bo1_sweep_catalog: map=%s name=%s w=%u candidate=%d parent=%u type=%d class=%d inv=%d clip=%d damage=%d fire=%d delay=%d reload=%d empty=%d raise=%d drop=%d ads=%d alt=%u dual=%u def=%p variant=%p\n",
            map,v->szInternalName,w,(int)Candidate(w),parent,d->weapType,d->weapClass,d->inventoryType,
            v->iClipSize,d->damage,d->iFireTime,d->iFireDelay,v->iReloadTime,v->iReloadEmptyTime,
            d->iRaiseTime,d->iDropTime,(int)d->aimDownSight,v->altWeaponIndex,d->dualWieldWeaponIndex,d,v);
        if (!strcmp(s_mode->current.string,"all") && Candidate(w) && parent) s_entries.push_back({parent,w});
    }
    if (strcmp(s_mode->current.string,"all") && strcmp(s_mode->current.string,"catalog")) {
        std::string list=s_mode->current.string;
        size_t p=0;
        while (p<list.size()) {
            const size_t b=list.find_first_not_of(", ",p);
            if (b==std::string::npos) break;
            const size_t e=list.find_first_of(", ",b);
            const std::string name=list.substr(b,e==std::string::npos ? e : e-b);
            const unsigned int w=BG_FindWeaponIndexForName(name.c_str());
            if (w && Parent(w)) s_entries.push_back({Parent(w),w});
            else { ++s_fail; Com_Printf(16,"bo1_sweep_result: name=%s FAIL missing_weapon\n",name.c_str()); }
            if(e==std::string::npos) break;
            p=e+1;
        }
    }
    Com_Printf(16,"bo1_sweep: map=%s catalog_complete weapons=%u selected=%zu mode=%s\n",map,BG_GetNumWeapons()-1,s_entries.size(),s_mode->current.string);
}
void FinishEntry(gentity_s *player) {
    const Entry &e=s_entries[s_index];
    const WeaponDef *d=BG_GetWeaponDef(e.hold);
    const bool melee=d->inventoryType==WEAPINVENTORY_MELEE;
    const bool off=d->inventoryType==WEAPINVENTORY_OFFHAND;
    const bool fired=melee ? s_melees>0 : off ? s_throws>0 : s_shots>0;
    const bool adsNeeded=!melee && !off && d->aimDownSight && !d->bDualWield;
    const bool reloadNeeded=!melee && !off && s_startReserve>0 && BG_GetWeaponVariantDef(e.hold)->iReloadTime>0;
    const bool swapNeeded=!melee && !off && s_pistol!=e.hold;
    const bool ok=s_given && fired && (off || melee || s_raised) && (!adsNeeded || s_ads)
        && (!reloadNeeded || s_reload) && (!swapNeeded || s_swapped);
    if(ok) ++s_pass; else ++s_fail;
    Com_Printf(16,"bo1_sweep_result: map=%s name=%s %s given=%d raised=%d ads=%d ads_required=%d reload=%d swapped=%d shots=%d throws=%d melees=%d clip=%d stock=%d reload_required=%d swap_required=%d hits=%d kills=%d\n",
        Dvar_GetString("mapname"),BG_WeaponName(e.hold),ok?"PASS":"FAIL",(int)s_given,(int)s_raised,(int)s_ads,
        (int)adsNeeded,(int)s_reload,(int)s_swapped,s_shots,s_throws,s_melees,
        BG_GetAmmoInClip(&player->client->ps,e.hold),BG_GetAmmoNotInClip(&player->client->ps,e.hold),
        (int)reloadNeeded,(int)swapNeeded,s_hits,s_kills);
    s_previous=e.give;
    ++s_index;
    s_start=0;
}
void Select(usercmd_s *cmd,client_t *client,unsigned int w,unsigned int parent=0) {
    cmd->weapon=w;
    if(parent && parent!=w) cmd->lastWeaponAltModeSwitch=parent;
    if(client->bIsTestClient) botInfos[client-svs.clients].weapon=w;
}
}
void G_SP_WeaponSweepFields(scriptInstance_t inst) {
    if(inst!=SCRIPTINSTANCE_SERVER) return;
    const unsigned int weapons=SL_FindString("zombie_weapons",inst);
    const unsigned int upgrade=SL_FindString("upgrade_name",inst);
    s_weaponField=weapons ? gScrCompilePub[inst].canonicalStrings[weapons] : 0;
    s_upgradeField=upgrade ? gScrCompilePub[inst].canonicalStrings[upgrade] : 0;
}
void G_SP_WeaponSweepRegister() {
    s_mode=_Dvar_RegisterString("bo1_weaponsweep","",0,"TEST ONLY: catalog, all, or comma-separated registered Zombies weapon names; exercise ordinary usercmds and log observed activity");
}
void G_SP_WeaponSweepReset() {
    s_entries.clear(); s_index=s_start=s_shots=s_throws=s_melees=s_pass=s_fail=s_lastReport=0;
    s_previous=s_pistol=0; s_built=s_done=false; s_player=-1;
}
bool G_SP_WeaponSweepCommand(gentity_s *player,client_t *client,usercmd_s *cmd) {
    if(!s_mode || !s_mode->current.string[0]) return false;
    playerState_s *ps=&player->client->ps;
    s_player=player->s.number;
    if(level.time<15000 || ps->pm_flags & 0x800) return true;
    if(!s_built) Build();
    if(s_index >= (int)s_entries.size()) {
        if(!s_done) {
            s_done=true;
            Com_Printf(16,"bo1_sweep_complete: map=%s PASS=%d FAIL=%d tested=%zu\n",Dvar_GetString("mapname"),s_pass,s_fail,s_entries.size());
            Cbuf_AddText(0,"quit\n");
        }
        return true;
    }
    const Entry &e=s_entries[s_index];
    const WeaponDef *d=BG_GetWeaponDef(e.hold);
    const bool melee=d->inventoryType==WEAPINVENTORY_MELEE;
    const bool off=d->inventoryType==WEAPINVENTORY_OFFHAND;
    if(!s_start) {
        s_start=level.time;
        s_shots=s_throws=s_melees=0;
        s_hits=s_kills=0;
        s_given=s_raised=s_ads=s_reload=s_swapped=false; s_reloadClip=-1;
        if(!s_pistol) s_pistol=ps->weapon;
        // Ordinary grenade input selects the first held offhand in its slot.
        // Give the requested test grenade that slot without competing spawn grenades.
        if(off) for(unsigned int w=1; w<BG_GetNumWeapons(); ++w) {
            const WeaponDef *other=BG_GetWeaponDef(w);
            if(w!=e.give && other->inventoryType==WEAPINVENTORY_OFFHAND
                && other->offhandSlot==d->offhandSlot && BG_PlayerHasWeapon(ps,w))
                BG_TakePlayerWeapon(ps,w);
        }
        if(s_previous && s_previous!=e.give && s_previous!=s_pistol && BG_PlayerHasWeapon(ps,s_previous))
            BG_TakePlayerWeapon(ps,s_previous);
        if(!BG_PlayerHasWeapon(ps,e.give)) {
            // Retail Zombies uses camo 15 (gold) for Pack-a-Punch; preserve the player selector bits.
            renderOptions_s options; options.i=((player->s.number+1)&31)<<21;
            if(strstr(BG_WeaponName(e.give),"upgraded")) options.i|=15;
            s_given=G_GivePlayerWeapon(ps,e.give,0,options)!=0;
        } else s_given=true;
        if(s_given) G_InitializeAmmo(player,e.give,0,0);
        s_startReserve=BG_GetAmmoNotInClip(ps,e.hold);
        Com_Printf(16,"bo1_sweep_start: t=%d index=%d/%zu name=%s give=%s given=%d type=%d inv=%d\n",level.time,s_index,s_entries.size(),BG_WeaponName(e.hold),BG_WeaponName(e.give),(int)s_given,d->weapType,d->inventoryType);
    }
    const int t=level.time-s_start;
    if(t>=18000) { FinishEntry(player); return true; }
    if(ps->weapon==e.hold && ps->weaponstate==WEAPON_READY) s_raised=true;
    if(ps->weapon==e.hold && ps->fWeaponPosFrac>0.5f) s_ads=true;
    if(ps->weapon==e.hold && ps->weaponstate>=WEAPON_RELOADING_RIGHT && ps->weaponstate<=WEAPON_RELOAD_QUICK_EMPTY) {
        if(s_reloadClip<0) s_reloadClip=BG_GetAmmoInClip(ps,e.hold);
    }
    if(s_reloadClip>=0 && BG_GetAmmoInClip(ps,e.hold)>s_reloadClip) s_reload=true;
    // Aim towards a live zombie when one exists; otherwise keep the map's ordinary spawn view.
    gentity_s *target=nullptr;
    float best=1e12f;
    for(int a=0; level.actors && a<MAX_ACTORS; ++a) {
        actor_s &actor=level.actors[a];
        if(!actor.inuse || !actor.ent || actor.ent->health<=0) continue;
        const float dist=Vec3DistanceSq(ps->origin,actor.ent->r.currentOrigin);
        if(dist<best) {best=dist; target=actor.ent;}
    }
    if(target) {
        float eye[3],dir[3],angles[3]; G_GetPlayerViewOrigin(ps,eye);
        Vec3Sub(target->r.currentOrigin,eye,dir); dir[2]+=40;
        vectoangles(dir,angles);
        for(int i=0;i<2;++i) cmd->angles[i]=(unsigned short)(int)((angles[i]-ps->delta_angles[i])*182.04445f);
    }
    if(melee) {
        Select(cmd,client,s_pistol);
        if(t>=2500 && t<6500 && t%1000<100) cmd->button_bits.setBit(2);
    } else if(off) {
        Select(cmd,client,s_pistol);
        cmd->offHandIndex=e.hold;
        const int bit=d->offhandSlot==OFFHAND_SLOT_TACTICAL_GRENADE ? 15 : 14;
        if(t>=2500 && t<2900) cmd->button_bits.setBit(bit);
    } else {
        Select(cmd,client,e.hold,e.give);
        if(t>=2000 && t<5000) cmd->button_bits.setBit(11);
        if((t>=3000 && t<5000) || (t>=9500 && t<11000)) {
            if(d->fireType==WEAPON_FIRETYPE_FULLAUTO || t%200<50) cmd->button_bits.setBit(0);
            if(d->bDualWield && (d->fireType==WEAPON_FIRETYPE_FULLAUTO || t%200<50)) cmd->button_bits.setBit(11);
        }
        if(t>=5200 && t<8500 && t%500<100) cmd->button_bits.setBit(5);
        if(t>=11500 && t<13500 && s_pistol!=e.hold) {
            Select(cmd,client,s_pistol);
            if(ps->weapon==s_pistol) s_swapped=true;
        }
    }
    if(level.time-s_lastReport>=1000) {
        s_lastReport=level.time;
        Com_Printf(16,"bo1_sweep_sample: t=%d name=%s w=%u state=%d time=%d clip=%d stock=%d ads=%.3f shots=%d throws=%d melee=%d\n",
            level.time,BG_WeaponName(e.hold),ps->weapon,ps->weaponstate,ps->weaponTime,BG_GetAmmoInClip(ps,e.hold),BG_GetAmmoNotInClip(ps,e.hold),ps->fWeaponPosFrac,s_shots,s_throws,s_melees);
    }
    return true;
}
void G_SP_WeaponSweepNotify(scriptInstance_t inst,unsigned int owner,unsigned int name,const VariableValue *top) {
    if(!s_mode || !s_mode->current.string[0] || inst!=SCRIPTINSTANCE_SERVER || !s_start || s_index>=(int)s_entries.size()) return;
    if(GetObjectType(inst,owner)!=VAR_ENTITY) return;
    const scr_entref_t ref=Scr_GetEntityIdRef(inst,owner);
    if(!ref.classnum && g_entities[ref.entnum].actor) {
        const bool damage=name==scr_const.damage && top->type==VAR_INTEGER;
        const bool death=name==scr_const.death && g_entities[ref.entnum].health<=0;
        const VariableValue *source=damage ? top-1 : top;
        if((damage || death) && source->type==VAR_POINTER
            && GetObjectType(inst,source->u.pointerValue)==VAR_ENTITY
            && Scr_GetEntityIdRef(inst,source->u.pointerValue).entnum==s_player) {
            if(damage) ++s_hits; else ++s_kills;
        }
        return;
    }
    if(name!=scr_const.weapon_fired && strcmp(SL_ConvertToString(name,inst),"grenade_fire")) return;
    if(ref.classnum || ref.entnum!=s_player) return;
    const Entry &e=s_entries[s_index];
    if(name==scr_const.weapon_fired && top->type==VAR_STRING) {
        const char *w=SL_ConvertToString(top->u.stringValue,inst);
        if(!strcmp(w,BG_WeaponName(e.hold)) || (BG_GetWeaponDef(e.hold)->dualWieldWeaponIndex
            && !strcmp(w,BG_WeaponName(BG_GetWeaponDef(e.hold)->dualWieldWeaponIndex)))) ++s_shots;
    } else if(top->type==VAR_POINTER && (top-1)->type==VAR_STRING) {
        if(!strcmp(SL_ConvertToString((top-1)->u.stringValue,inst),BG_WeaponName(e.hold))) ++s_throws;
    }
}
void G_SP_WeaponSweepMelee(const gentity_s *player,unsigned int w) {
    if(s_start && s_index<(int)s_entries.size() && player->s.number==s_player && w==s_entries[s_index].hold) ++s_melees;
}
