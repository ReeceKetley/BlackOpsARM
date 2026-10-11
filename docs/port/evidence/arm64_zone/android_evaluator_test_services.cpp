// Generated test-only endpoints. Never link into an engine or APK.
#include <clientscript/cscr_variable.h>
#include <clientscript/cscr_compiler.h>
#include <clientscript/cscr_evaluate.h>
#include <clientscript/cscr_debugger.h>
#include <clientscript/cscr_stringlist.h>
#include <clientscript/cscr_vm.h>
#include <clientscript/cscr_parser.h>
#include <clientscript/cscr_animtree.h>
#include <clientscript/cscr_tempmemory.h>
#include <clientscript/cscr_yacc.h>
#include <universal/q_shared.h>
#include <xanim/xanim_debug.h>
#include <cstdio>
#include <cstdlib>
[[noreturn]] static void unavailable(const char *name) { std::fprintf(stderr, "Unavailable evaluator test service: %s\n", name); std::abort(); }
void I_strncpyz(char *dest, const char *src, int destsize) { unavailable("I_strncpyz"); }
int Com_sprintf(char *dest, unsigned int size, const char *fmt, ...) { unavailable("Com_sprintf"); }
unsigned int __cdecl AllocValue(scriptInstance_t inst) { unavailable("AllocValue"); }
unsigned int __cdecl Scr_GetSelf(scriptInstance_t inst, unsigned int threadId) { unavailable("Scr_GetSelf"); }
unsigned int __cdecl FindVariable(scriptInstance_t inst, unsigned int parentId, unsigned int unsignedValue) { unavailable("FindVariable"); }
VariableValue __cdecl Scr_FindVariableField(scriptInstance_t inst, unsigned int parentId, unsigned int name) { unavailable("Scr_FindVariableField"); }
void __cdecl Scr_EvalSizeValue(scriptInstance_t inst, VariableValue *value) { unavailable("Scr_EvalSizeValue"); }
unsigned int __cdecl GetArraySize(scriptInstance_t inst, unsigned int id) { unavailable("GetArraySize"); }
bool __cdecl IsFieldObject(scriptInstance_t inst, unsigned int id) { unavailable("IsFieldObject"); }
bool __cdecl Scr_IsThreadAlive(unsigned int thread, scriptInstance_t inst) { unavailable("Scr_IsThreadAlive"); }
void __cdecl Scr_EvalBoolNot(scriptInstance_t inst, VariableValue *value) { unavailable("Scr_EvalBoolNot"); }
void __cdecl Scr_EvalBoolComplement(scriptInstance_t inst, VariableValue *value) { unavailable("Scr_EvalBoolComplement"); }
void __cdecl Scr_CastBool(scriptInstance_t inst, VariableValue *value) { unavailable("Scr_CastBool"); }
char __cdecl Scr_GetEntClassId(scriptInstance_t inst, unsigned int id) { unavailable("Scr_GetEntClassId"); }
int __cdecl Scr_GetEntNum(scriptInstance_t inst, unsigned int id) { unavailable("Scr_GetEntNum"); }
void __cdecl Scr_CastVector(scriptInstance_t inst, VariableValue *value) { unavailable("Scr_CastVector"); }
unsigned int __cdecl Scr_EvalFieldObject(scriptInstance_t inst, unsigned int tempVariable, VariableValue *value) { unavailable("Scr_EvalFieldObject"); }
void __cdecl Scr_EvalBinaryOperator(scriptInstance_t inst, int op, VariableValue *value1, VariableValue *value2) { unavailable("Scr_EvalBinaryOperator"); }
bool __cdecl IsObjectFree(scriptInstance_t inst, unsigned int id) { unavailable("IsObjectFree"); }
unsigned int __cdecl GetObjectType(scriptInstance_t inst, unsigned int id) { unavailable("GetObjectType"); }
unsigned int __cdecl FindEntityId(scriptInstance_t inst, unsigned int entnum, unsigned int classnum, int clientNum) { unavailable("FindEntityId"); }
void __cdecl Scr_EvalArray(scriptInstance_t inst, VariableValue *value, VariableValue *index) { unavailable("Scr_EvalArray"); }
scr_entref_t __cdecl Scr_GetEntityIdRef(scriptInstance_t inst, unsigned int entId) { unavailable("Scr_GetEntityIdRef"); }
int __cdecl Scr_GetClassnumForCharId(scriptInstance_t inst, char charId) { unavailable("Scr_GetClassnumForCharId"); }
bool __cdecl Scr_RefToVariable(scriptInstance_t inst, unsigned int id, int isObject) { unavailable("Scr_RefToVariable"); }
char *__cdecl SL_ConvertToString(unsigned int stringValue, scriptInstance_t inst) { unavailable("SL_ConvertToString"); }
unsigned int __cdecl SL_FindString(const char *str, scriptInstance_t inst) { unavailable("SL_FindString"); }
unsigned int __cdecl SL_GetString_(scriptInstance_t inst, char *str, unsigned int user, int type) { unavailable("SL_GetString_"); }
void __cdecl SL_ShutdownSystem(scriptInstance_t inst, unsigned int user) { unavailable("SL_ShutdownSystem"); }
void (__cdecl *__cdecl GetFunction(scriptInstance_t inst, const char **pName, int *type))() { unavailable("GetFunction"); }
void (__cdecl *__cdecl GetMethod(scriptInstance_t inst, const char **pName, int *type))(scr_entref_t) { unavailable("GetMethod"); }
int __cdecl GetExpressionCount(sval_u exprlist) { unavailable("GetExpressionCount"); }
unsigned int __cdecl Scr_GetBuiltin(scriptInstance_t inst, sval_u func_name) { unavailable("Scr_GetBuiltin"); }
unsigned int Scr_FuncPtrToSlot(void *func) { unavailable("Scr_FuncPtrToSlot"); }
void *Scr_SlotToFuncPtr(unsigned int slot) { unavailable("Scr_SlotToFuncPtr"); }
void __cdecl Scr_CompileStatement(scriptInstance_t inst, sval_u parseData) { unavailable("Scr_CompileStatement"); }
void Com_PrintError(int channel, const char *fmt, ...) { unavailable("Com_PrintError"); }
void __cdecl RemoveRefToValue(scriptInstance_t inst, int type, VariableUnion u) { unavailable("RemoveRefToValue"); }
XAnim_s *__cdecl Scr_GetAnims(unsigned int index, scriptInstance_t inst) { unavailable("Scr_GetAnims"); }
VariableValue __cdecl Scr_EvalVariable(scriptInstance_t inst, unsigned int id) { unavailable("Scr_EvalVariable"); }
void __cdecl AddRefToValue(scriptInstance_t inst, int type, VariableUnion u) { unavailable("AddRefToValue"); }
void __cdecl AddRefToObject(scriptInstance_t inst, unsigned int id) { unavailable("AddRefToObject"); }
void __cdecl Scr_ClearErrorMessage(scriptInstance_t inst) { unavailable("Scr_ClearErrorMessage"); }
void __cdecl Scr_ClearOutParams(scriptInstance_t inst) { unavailable("Scr_ClearOutParams"); }
void __cdecl Scr_Error(scriptInstance_t inst, const char *error, bool force_terminal) { unavailable("Scr_Error"); }
void __cdecl Scr_GetCodePos(
                scriptInstance_t inst,
                const char *codePos,
                unsigned int index,
                char *outBuf,
                unsigned int outBufLen) { unavailable("Scr_GetCodePos"); }
char *__cdecl XAnimGetAnimDebugName(const XAnim_s *anims, unsigned int animIndex) { unavailable("XAnimGetAnimDebugName"); }
void __cdecl TempMemoryReset(HunkUser *user) { unavailable("TempMemoryReset"); }
char *__cdecl TempMallocAlignStrict(int len) { unavailable("TempMallocAlignStrict"); }
void __cdecl ScriptParse(scriptInstance_t inst, sval_u *parseData, unsigned __int8 user) { unavailable("ScriptParse"); }
// Test-owned zeroed context, not a VM implementation.
scrVarPub_t gScrVarPub[2]{};
scrCompilePub_t gScrCompilePub[2]{};
scrVmPub_t gScrVmPub[2]{};
unsigned int g_scrVarChildBegin[2]{};
thread_local int g_breakonExpr{}, g_breakonHit{};
thread_local unsigned int g_breakonString{}, g_breakonObject{};
const dvar_t *com_sv_running = nullptr;
