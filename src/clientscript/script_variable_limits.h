#pragma once

// mod: the server instance's object (parent) pool doubles with the startup dvar bo1_mod_scriptvars 1
// (Scr_InitVariables): parents 1..0xFFFD, children at 0x10000. Retail and the client instance: parents
// 1..0x7FFD (VARIABLELIST_PARENT_SIZE 0x7FFE), children 1..0x3FFFD at variableList[0x8000 + id].
extern unsigned int g_scrVarChildBegin[2];
#define VARIABLELIST_CHILD_BEGIN(inst) (g_scrVarChildBegin[(inst)])
#define VARIABLELIST_PARENT_SIZE(inst) (g_scrVarChildBegin[(inst)] - 2)
// mod: the server child (value) pool grows with bo1_mod_scriptvars 2+ (retail and the client: 0x3FFFE, hash % 0x3FFFD)
extern unsigned int g_scrVarChildSize[2];
#define VARIABLELIST_CHILD_SIZE_I(inst) (g_scrVarChildSize[(inst)])
#define VARIABLELIST_CHILD_HASH(inst) (g_scrVarChildSize[(inst)] - 1)
#define VARIABLELIST_SIZE(inst) (g_scrVarChildBegin[(inst)] + VARIABLELIST_CHILD_SIZE_I(inst))
// reserved child names just past the object-key names 0x10000 + id (retail 0x17FFE notify list, 0x17FFF thread stack)
#define VARIABLELIST_NOTIFY_NAME(inst) (0x10000 + VARIABLELIST_PARENT_SIZE(inst))
#define VARIABLELIST_STACK_NAME(inst) (0x10001 + VARIABLELIST_PARENT_SIZE(inst))
// mod: span of the object-key names checked by the notify-list asserts: retail 1 << 16, the parent pool when larger
#define SCR_OBJECT_KEY_SPAN(inst) (VARIABLELIST_PARENT_SIZE(inst) > 0x10000 ? VARIABLELIST_PARENT_SIZE(inst) : 0x10000u)

