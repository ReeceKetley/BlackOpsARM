#include <clientscript/cscr_vm.h>
#include <clientscript/cscr_stringlist.h>
#include <clientscript/cscr_memorytree.h>
#include <clientscript/cscr_animtree.h>
#include <platform/android/thread_services.h>
#include <platform/android/script_string_services.h>
#include <platform/critical_section_services.h>
#include <bgame/bg_public.h>
#include <universal/dvar.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/wait.h>
#include <unistd.h>
#include <initializer_list>
extern gentity_s g_entities[];
static unsigned checks;
static void Check(bool ok, const char *why) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", why); std::abort(); }
    ++checks;
}
static void Retire(scriptInstance_t inst) {
    gScrVmPub[inst].outparamcount = gScrVmPub[inst].inparamcount;
    gScrVmPub[inst].inparamcount = 0;
    Scr_ClearOutParams(inst);
}
static void Fault(scriptInstance_t inst, bool eval) {
    const pid_t child = fork(); Check(child >= 0, "real overflow child");
    if (!child) {
        auto &pub = gScrVmPub[inst]; pub.inparamcount = pub.outparamcount = 0;
        if (eval) pub.top = &gScrVmGlob[inst].eval_stack_storage[2];
        else pub.top = pub.maxstack;
        Scr_AddInt(17, inst); _exit(91);
    }
    int status{};
    Check(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 2,
          eval ? "actual third eval push rejected by original precondition" : "actual main overflow fatal before write");
}
int main() {
    alarm(45); Sys_InitMainThread(); Sys_InitializeCriticalSections(); bo1::script::RegisterServerThread(); Dvar_Init();
    static_assert(sizeof(VariableValue) == 8 && sizeof(VariableUnion) == 4);
    static_assert(sizeof(scrVmGlob_t::eval_stack_storage) == 3 * sizeof(VariableValue));
    static_assert(sizeof(scrVmGlob_t::localVarsStack_storage) == 2049 * sizeof(unsigned));
    for (auto inst : {SCRIPTINSTANCE_SERVER, SCRIPTINSTANCE_CLIENT}) {
        SL_Init(inst);
        // Scoped real owner arena: canonical range initialization and all actual
        // allocation/refcount algorithms run; this does not claim map startup.
        auto *arena = static_cast<VariableValueInternal *>(std::calloc(VARIABLELIST_SIZE(inst), sizeof(VariableValueInternal)));
        Check(arena != nullptr, "owned canonical variable arena"); gScrVarGlob[inst].variableList = arena;
        Scr_InitVariableRange(inst, 1, VARIABLELIST_CHILD_BEGIN(inst) - 1);
        Scr_InitVariableRange(inst, VARIABLELIST_CHILD_BEGIN(inst), VARIABLELIST_SIZE(inst));
        Scr_VM_Init(inst);
        auto &pub = gScrVmPub[inst]; auto &glob = gScrVmGlob[inst];
        Check(pub.top == pub.stack && pub.stack[0].type == VAR_CODEPOS && pub.maxstack == pub.stack + 2047,
              "actual main initialization owns sentinel and preserves capacity");
        Check(pub.localVars == glob.localVarsStack_storage, "actual VM local cursor starts at owned sentinel");
        glob.localVarsStack_storage[0] = 0x13579bdf; glob.starttime = 0x2468ace;
        for (int i = 1; i <= 2047; ++i) Scr_AddInt(i, inst);
        Check(pub.top == pub.maxstack && pub.inparamcount == 2047 && pub.top->u.intValue == 2047,
              "actual main argument pushes preserve full live capacity");
        Fault(inst, false); Retire(inst);
        Check(Scr_IsStackClear(inst) && !pub.outparamcount, "actual main arguments retire to owned sentinel");
        glob.eval_stack_storage[0] = {{0x55aa55aa}, VAR_CODEPOS};
        pub.top = glob.eval_stack_storage;
        Scr_AddInt(-27, inst); Scr_AddFloat(-2.5f, inst);
        Check(pub.top == glob.eval_stack_storage + 2 && pub.inparamcount == 2 &&
              glob.eval_stack_storage[1].type == VAR_INTEGER && glob.eval_stack_storage[1].u.intValue == -27 &&
              glob.eval_stack_storage[2].type == VAR_FLOAT && glob.eval_stack_storage[2].u.floatValue == -2.5f,
              "actual eval two-value argument range");
        Fault(inst, true); Retire(inst);
        Check(pub.top == glob.eval_stack_storage && glob.eval_stack_storage[0].u.intValue == 0x55aa55aa &&
              glob.eval_stack_storage[0].type == VAR_CODEPOS, "eval arguments retire within owner and preserve prefix canary");
        Scr_AddString(const_cast<char *>("owned-vm-argument"), inst);
        const unsigned id = pub.top->u.stringValue; Scr_AddConstString(id, inst);
        Check(GetRefString(inst, id)->refCount == 2 && gScrMemTreeGlob[inst].totalAlloc == 1,
              "actual eval string arguments own two references to one MT allocation");
        Retire(inst);
        Check(pub.top == glob.eval_stack_storage && !gScrMemTreeGlob[inst].totalAlloc &&
              !gScrStringDebugGlob[inst]->totalRefCount, "actual clear arguments frees final string owner");
        float vector[] = {1.5f, -2.0f, 7.25f}; Scr_AddVector(vector, inst);
        Check(pub.top == glob.eval_stack_storage + 1 && gScrVarPub[inst].totalVectorRefCount == 1 &&
              !std::memcmp(static_cast<const float *>(pub.top->u.vectorValue), vector, sizeof(vector)),
              "actual eval vector allocation and Narrow32 value publication");
        // IncInParam must actually clear the outgoing reference before reusing
        // logical eval slot0 for the next incoming argument.
        pub.outparamcount = 1; pub.inparamcount = 0; Scr_AddInt(42, inst);
        Check(pub.top == glob.eval_stack_storage + 1 && pub.top->u.intValue == 42 &&
              !gScrVarPub[inst].totalVectorRefCount && !gScrMemTreeGlob[inst].totalAlloc,
              "actual IncInParam retires outgoing vector then reuses same owned slot");
        Retire(inst);
        gScrVarPub[inst].varUsagePos = "<actual VM stack fixture locals>";
        const unsigned object = AllocObject(inst); unsigned ids[2048];
        for (unsigned i = 0; i < 2048; ++i) ids[i] = GetNewVariable(inst, object, GetInternalVariableIndex(inst, i));
        Check(Scr_AddLocalVars(inst, object) == 2048 && pub.localVars == glob.localVarsStack_storage + 2048,
              "actual local sibling producer fills original2048 ID capacity");
        for (unsigned index : {0u, 1u, 5u, 255u}) {
            const char operand = static_cast<char>(index);
            Check(Scr_GetLocalVar(inst, &operand) == ids[2047 - index], "actual unsigned cached local lookup through owned array");
        }
        Check(glob.starttime == 0x2468ace && glob.localVarsStack_storage[0] == 0x13579bdf,
              "actual local producer never aliases clock or sentinel");
        Scr_ResetTimeout(inst);
        Check(glob.starttime != 0x2468ace && glob.localVarsStack_storage[0] == 0x13579bdf && pub.localVars[0] == ids[2047],
              "actual clock reset does not corrupt local IDs or empty sentinel");
        pub.localVars -= 2048; RemoveRefToObject(inst, object); VM_Shutdown(inst);
        Check(pub.localVars == glob.localVarsStack_storage && !gScrVarPub[inst].numScriptValues &&
              !gScrVarPub[inst].numScriptObjects && !gScrVarPub[inst].totalObjectRefCount,
              "actual canonical local variable/object ownership fully retires");
        std::free(arena); gScrVarGlob[inst].variableList = nullptr; SL_Shutdown(inst);
    }
    // Actual server entity getter dispatch, using its real canonical entity and
    // field table; no replacement Scr_GetObjectField/CScr_GetObjectField.
    auto inst = SCRIPTINSTANCE_SERVER; auto &pub = gScrVmPub[inst]; auto &glob = gScrVmGlob[inst];
    g_entities[0].r.inuse = true; g_entities[0].actor = nullptr; g_entities[0].health = 173;
    const auto undefined = GetEntityFieldValue(inst, 0, 0, 0, 0x8000);
    Check(undefined.type == VAR_UNDEFINED && pub.top == glob.eval_stack_storage && !pub.inparamcount,
          "actual absent actor field returns undefined at owned sentinel");
    const auto integer = GetEntityFieldValue(inst, 0, 0, 0, 8);
    Check(integer.type == VAR_INTEGER && integer.u.intValue == 173 && pub.top == glob.eval_stack_storage + 1 && !pub.inparamcount,
          "actual real entity health getter returns owned eval value and resets argument count");
    std::printf("PASS: %u actual VM stack/argument/reference/local-ID/clock/field-getter observations; scoped owner fixture, no map/opcode proof\n", checks);
}
