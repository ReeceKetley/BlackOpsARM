#pragma once
#include <clientscript/cscr_variable.h>
struct gentity_s;
struct client_t;
struct usercmd_s;
// Opt-in diagnostic harness. Ordinary gameplay never enables this.
void G_SP_WeaponSweepRegister();
void G_SP_WeaponSweepReset();
void G_SP_WeaponSweepFields(scriptInstance_t);
bool G_SP_WeaponSweepCommand(gentity_s *, client_t *, usercmd_s *);
void G_SP_WeaponSweepNotify(scriptInstance_t, unsigned int, unsigned int, const VariableValue *);
void G_SP_WeaponSweepMelee(const gentity_s *, unsigned int);
