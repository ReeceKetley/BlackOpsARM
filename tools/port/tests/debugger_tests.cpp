#include <clientscript/cscr_parsetree.h>
#include <clientscript/cscr_compiler.h>
#include <clientscript/cscr_evaluate.h>
#include <clientscript/cscr_vm.h>
#include <platform/android/zone_heap.h>
#include <universal/memory_services.h>
#include "hunk_test_checks.h"
#include <cstring>
#include <thread>
#include <vector>
#if defined(__ANDROID__)
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
template<class F> static void aborts(F fn) {
    pid_t p = fork(); CHECK(p >= 0);
    if (!p) { fn(); _exit(99); }
    int status; CHECK(waitpid(p, &status, 0) == p);
    CHECK(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
}
#endif
static_assert(sizeof(sval_u) == 4 && alignof(sval_u) == 4);
// Test-owned value/reference fixtures, observed by actual evaluator cleanup.
struct Value { bool live; unsigned clears; };
static Value values[2][8];
static unsigned refs[2][8], frees[2], removes[2];
void ClearVariableValue(scriptInstance_t inst, unsigned id) { CHECK(id < 8 && values[inst][id].live); ++values[inst][id].clears; }
void FreeValue(scriptInstance_t inst, unsigned id) { CHECK(id < 8 && values[inst][id].live); values[inst][id].live = false; ++frees[inst]; }
void RemoveRefToObject(scriptInstance_t inst, unsigned id) { CHECK(id < 8 && refs[inst][id] > 0); --refs[inst][id]; ++removes[inst]; }
scriptInstance_t gDebuggerInstance;
extern scrEvaluateGlob_t gScrEvaluateGlob[2];
static void release(scriptInstance_t inst, sval_u root)
{
    ScriptExpression_t expr{}; expr.parseData = root; expr.exprHead = g_debugExprHead;
    CHECK(expr.exprHead); g_debugExprHead = nullptr; // Transfer compile-list ownership to the expression.
    Scr_FreeDebugExpr(inst, &expr);
}
int main()
{
    using namespace bo1::android;
    CHECK(ZoneAllocationCount() == 0 && ZoneAllocatedBytes() == 0);
    for (auto inst : {SCRIPTINSTANCE_SERVER, SCRIPTINSTANCE_CLIENT}) {
        g_debugExprHead = nullptr;
        sval_u args[4] = {sval_u(0xffffffffu), sval_u(0), sval_u(37), sval_u(0x80000000u)};
        auto zero = debugger_node0(inst, ENUM_NOP);
        auto one = debugger_node1(inst, ENUM_integer, args[0]);
        auto two = debugger_node2(inst, ENUM_NOP, one, args[1]);
        auto three = debugger_node3(inst, ENUM_NOP, args[0], args[1], args[2]);
        auto four = debugger_node4(inst, ENUM_NOP, args[0], args[1], args[2], args[3]);
        CHECK(zero.node[0].type == ENUM_NOP && one.node[1].intValue == -1);
        CHECK(static_cast<sval_u *>(two.node[1].node) == static_cast<sval_u *>(one.node));
        CHECK(three.node[3].intValue == 37 && four.node[4].sourcePosValue == 0x80000000u);
        auto list = debugger_node0(inst, ENUM_NOP);
        debugger_prepend_node(inst, one, list);
        debugger_prepend_node(inst, three, list);
        sval_u *link = list.node[0].node;
        CHECK(static_cast<sval_u *>(link[0].node) == static_cast<sval_u *>(three.node));
        link = link[1].node;
        CHECK(static_cast<sval_u *>(link[0].node) == static_cast<sval_u *>(one.node));
        CHECK(link[1].node == nullptr);
        char input[] = "original debugger bytes";
        for (int alignment : {1, 4, 8, 32, 4096}) {
            auto buffer = debugger_buffer(inst, ENUM_statement, input, sizeof(input), alignment);
            CHECK(buffer.node[0].type == ENUM_statement);
            const char *copy = buffer.node[1].codePosValue;
            CHECK(copy != input && reinterpret_cast<std::uintptr_t>(copy) % alignment == 0);
            CHECK(std::memcmp(copy, input, sizeof(input)) == 0);
        }
        auto string = debugger_string(inst, ENUM_string, input);
        CHECK(std::strcmp(string.node[1].codePosValue, input) == 0);
        auto empty = debugger_buffer(inst, ENUM_statement, nullptr, 0, 32);
        CHECK(empty.node[0].type == ENUM_statement);
        for (unsigned id = 1; id <= 4; ++id) values[inst][id] = {true, 0};
        debugger_node4(inst, ENUM_local_variable, 0, 0, 0, sval_u(1));
        debugger_node4(inst, ENUM_local_variable_frozen, 0, 0, 0, sval_u(2));
        debugger_node2(inst, ENUM_variable, 0, sval_u(3));
        debugger_node2(inst, ENUM_call_expression, 0, sval_u(4));
        refs[inst][1] = 1;
        auto object = debugger_node1(inst, ENUM_thread_object, sval_u(1));
        Scr_ClearDebugExpr(inst, g_debugExprHead);
        for (unsigned id = 1; id <= 4; ++id) CHECK(values[inst][id].clears == 1);
        CHECK(refs[inst][1] == 1);
        ScriptExpression_t expr{}; expr.parseData = object; expr.exprHead = g_debugExprHead; expr.breakonExpr = 1;
        g_debugExprHead = nullptr;
        gScrVmDebugPub[inst].checkBreakon = 3;
        Scr_FreeDebugExpr(inst, &expr);
        CHECK(gScrVmDebugPub[inst].checkBreakon == 2);
        CHECK(frees[inst] == 4 && removes[inst] == 1 && refs[inst][1] == 0);
        for (unsigned id = 1; id <= 4; ++id) CHECK(!values[inst][id].live);
        CHECK(ZoneAllocationCount() == 0 && ZoneAllocatedBytes() == 0);
        // More real evaluator paths without supplying unavailable VM services.
        Scr_InitEvaluate(inst);
        // Execute literal paths from the real evaluator. Every unavailable
        // compiler/VM endpoint aborts if this bounded suite reaches it.
        for (int number : {0, 1, -1, 37, -999, INT32_MAX, INT32_MIN}) {
            auto literal = debugger_node1(inst, ENUM_integer, sval_u(number));
            auto expression = debugger_node1(inst, ENUM_primitive_expression, literal);
            ScriptExpression_t script{}; script.parseData = expression;
            VariableValue result{};
            CHECK(!Scr_EvalScriptExpression(inst, &script, 0, &result, false, false));
            CHECK(result.type == VAR_INTEGER && result.u.intValue == number);
            release(inst, expression);
        }
        for (float number : {0.0f, -0.0f, 1.25f, -37.5f}) {
            sval_u value; value.floatValue = number;
            auto literal = debugger_node1(inst, ENUM_float, value);
            VariableValue result{}; Scr_EvalPrimitiveExpression(inst, literal, 0, &result);
            CHECK(result.type == VAR_FLOAT && std::memcmp(&result.u.floatValue, &number, 4) == 0);
            release(inst, literal);
        }
        auto minusInt = debugger_node1(inst, ENUM_minus_integer, sval_u(127));
        VariableValue negative{}; Scr_EvalPrimitiveExpression(inst, minusInt, 0, &negative);
        CHECK(negative.type == VAR_INTEGER && negative.u.intValue == -127);
        release(inst, minusInt);
        sval_u positiveFloat; positiveFloat.floatValue = 1.25f;
        auto minusFloat = debugger_node1(inst, ENUM_minus_float, positiveFloat);
        Scr_EvalPrimitiveExpression(inst, minusFloat, 0, &negative);
        CHECK(negative.type == VAR_FLOAT && negative.u.floatValue == -1.25f);
        release(inst, minusFloat);
        for (auto token : {ENUM_false, ENUM_true}) {
            auto literal = debugger_node0(inst, token);
            VariableValue result{}; Scr_EvalPrimitiveExpression(inst, literal, 0, &result);
            CHECK(result.type == VAR_INTEGER && result.u.intValue == (token == ENUM_true));
            release(inst, literal);
        }
        // TASK-0007 bounded normal-to-debugger expression compilation.
        // Normal trees use yacc-shaped node*/node_pos constructors in the
        // HunkUser arena; actual Scr_CompileExpression /
        // Scr_CompilePrimitiveExpression emit debugger (Z heap) trees.
        // Only integer/float/minus/true/false outputs are evaluated here:
        // they need no VM service beyond the actual evaluator. String /
        // istring need SL_ConvertToString, variable/call need AllocValue,
        // expression lists need GetExpressionCount, and bool/or/and/binary
        // /not/complement evaluation needs CastBool/BinaryOperator fixtures:
        // every such endpoint aborts via generated services, so operator
        // trees are compilation-verified and freed, explicitly not
        // evaluated. List/string/callable ownership is inspected only;
        // full VM remains deferred (WARN-0007).
        Scr_InitAllocNode(inst);
        {
            auto pos = node_pos(0x1234u);
            for (int number : {0, 1, -1, 37, -999, INT32_MAX, INT32_MIN}) {
                auto normalPrim = node2(inst, ENUM_integer, sval_u(number), pos);
                auto normalExpr = node2(inst, ENUM_primitive_expression, normalPrim, pos);
                g_debugExprHead = nullptr;
                Scr_CompileExpression(inst, &normalExpr);
                CHECK(normalExpr.node[0].type == ENUM_primitive_expression);
                CHECK(normalExpr.node[1].node[0].type == ENUM_integer);
                CHECK(normalExpr.node[1].node[1].intValue == number);
                CHECK(reinterpret_cast<std::uintptr_t>(static_cast<sval_u *>(normalExpr.node)) % 4 == 0);
                CHECK(reinterpret_cast<std::uintptr_t>(static_cast<sval_u *>(normalExpr.node[1].node)) % 4 == 0);
                VariableValue result{}; Scr_EvalExpression(inst, normalExpr, 0, &result);
                CHECK(result.type == VAR_INTEGER && result.u.intValue == number);
                release(inst, normalExpr);
                CHECK(ZoneAllocationCount() == 0 && ZoneAllocatedBytes() == 0);
            }
            for (int magnitude : {1, 127, 999}) {
                auto normalPrim = node2(inst, ENUM_minus_integer, sval_u(magnitude), pos);
                auto normalExpr = node2(inst, ENUM_primitive_expression, normalPrim, pos);
                g_debugExprHead = nullptr;
                Scr_CompileExpression(inst, &normalExpr);
                CHECK(normalExpr.node[0].type == ENUM_primitive_expression);
                CHECK(normalExpr.node[1].node[0].type == ENUM_minus_integer);
                CHECK(normalExpr.node[1].node[1].intValue == magnitude);
                VariableValue result{}; Scr_EvalExpression(inst, normalExpr, 0, &result);
                CHECK(result.type == VAR_INTEGER && result.u.intValue == -magnitude);
                release(inst, normalExpr);
                CHECK(ZoneAllocationCount() == 0 && ZoneAllocatedBytes() == 0);
            }
            for (float number : {0.0f, -0.0f, 1.25f, -37.5f}) {
                sval_u bits; bits.floatValue = number;
                auto normalPrim = node2(inst, ENUM_float, bits, pos);
                auto normalExpr = node2(inst, ENUM_primitive_expression, normalPrim, pos);
                g_debugExprHead = nullptr;
                Scr_CompileExpression(inst, &normalExpr);
                CHECK(normalExpr.node[0].type == ENUM_primitive_expression);
                CHECK(normalExpr.node[1].node[0].type == ENUM_float);
                CHECK(std::memcmp(&normalExpr.node[1].node[1], &bits, 4) == 0);
                VariableValue result{}; Scr_EvalExpression(inst, normalExpr, 0, &result);
                CHECK(result.type == VAR_FLOAT && std::memcmp(&result.u.floatValue, &number, 4) == 0);
                release(inst, normalExpr);
                CHECK(ZoneAllocationCount() == 0 && ZoneAllocatedBytes() == 0);
            }
            {
                sval_u bits; bits.floatValue = 1.25f;
                auto normalPrim = node2(inst, ENUM_minus_float, bits, pos);
                auto normalExpr = node2(inst, ENUM_primitive_expression, normalPrim, pos);
                g_debugExprHead = nullptr;
                Scr_CompileExpression(inst, &normalExpr);
                CHECK(normalExpr.node[1].node[0].type == ENUM_minus_float);
                VariableValue result{}; Scr_EvalExpression(inst, normalExpr, 0, &result);
                CHECK(result.type == VAR_FLOAT && result.u.floatValue == -1.25f);
                release(inst, normalExpr);
                CHECK(ZoneAllocationCount() == 0 && ZoneAllocatedBytes() == 0);
            }
            for (auto token : {ENUM_false, ENUM_true}) {
                auto normalPrim = node1(inst, token, pos);
                g_debugExprHead = nullptr;
                Scr_CompilePrimitiveExpression(inst, &normalPrim);
                CHECK(normalPrim.node[0].type == token);
                VariableValue result{}; Scr_EvalPrimitiveExpression(inst, normalPrim, 0, &result);
                CHECK(result.type == VAR_INTEGER && result.u.intValue == (token == ENUM_true));
                release(inst, normalPrim);
                CHECK(ZoneAllocationCount() == 0 && ZoneAllocatedBytes() == 0);
            }
            {
                auto normalPrim = node1(inst, ENUM_true, pos);
                auto normalExpr = node2(inst, ENUM_primitive_expression, normalPrim, pos);
                g_debugExprHead = nullptr;
                Scr_CompileExpression(inst, &normalExpr);
                CHECK(normalExpr.node[0].type == ENUM_primitive_expression);
                CHECK(normalExpr.node[1].node[0].type == ENUM_true);
                VariableValue result{}; Scr_EvalExpression(inst, normalExpr, 0, &result);
                CHECK(result.type == VAR_INTEGER && result.u.intValue == 1);
                release(inst, normalExpr);
                CHECK(ZoneAllocationCount() == 0 && ZoneAllocatedBytes() == 0);
            }
            // Operator shapes compile only; evaluation would reach aborting
            // CastBool/BinaryOperator fixtures, so it stays unsupported.
            {
                auto innerPrim = node1(inst, ENUM_true, pos);
                auto innerExpr = node2(inst, ENUM_primitive_expression, innerPrim, pos);
                auto normalNot = node2(inst, ENUM_bool_not, innerExpr, pos);
                g_debugExprHead = nullptr;
                Scr_CompileExpression(inst, &normalNot);
                CHECK(normalNot.node[0].type == ENUM_bool_not);
                CHECK(normalNot.node[1].node[0].type == ENUM_primitive_expression);
                CHECK(normalNot.node[1].node[1].node[0].type == ENUM_true);
                release(inst, normalNot);
                CHECK(ZoneAllocationCount() == 0 && ZoneAllocatedBytes() == 0);
            }
            for (auto token : {ENUM_bool_or, ENUM_bool_and}) {
                auto leftPrim = node2(inst, ENUM_integer, sval_u(1), pos);
                auto leftExpr = node2(inst, ENUM_primitive_expression, leftPrim, pos);
                auto rightPrim = node2(inst, ENUM_integer, sval_u(0), pos);
                auto rightExpr = node2(inst, ENUM_primitive_expression, rightPrim, pos);
                auto normalOp = node5(inst, token, leftExpr, rightExpr, pos, pos, pos);
                g_debugExprHead = nullptr;
                Scr_CompileExpression(inst, &normalOp);
                CHECK(normalOp.node[0].type == token);
                CHECK(normalOp.node[1].node[0].type == ENUM_primitive_expression);
                CHECK(normalOp.node[2].node[0].type == ENUM_primitive_expression);
                CHECK(normalOp.node[1].node[1].node[1].intValue == 1);
                CHECK(normalOp.node[2].node[1].node[1].intValue == 0);
                release(inst, normalOp);
                CHECK(ZoneAllocationCount() == 0 && ZoneAllocatedBytes() == 0);
            }
            {
                auto leftPrim = node2(inst, ENUM_integer, sval_u(6), pos);
                auto leftExpr = node2(inst, ENUM_primitive_expression, leftPrim, pos);
                auto rightPrim = node2(inst, ENUM_integer, sval_u(7), pos);
                auto rightExpr = node2(inst, ENUM_primitive_expression, rightPrim, pos);
                auto normalBinary = node4(inst, ENUM_binary, leftExpr, rightExpr, node1_(inst, 102), pos);
                g_debugExprHead = nullptr;
                Scr_CompileExpression(inst, &normalBinary);
                CHECK(normalBinary.node[0].type == ENUM_binary);
                CHECK(normalBinary.node[1].node[0].type == ENUM_primitive_expression);
                CHECK(normalBinary.node[2].node[0].type == ENUM_primitive_expression);
                CHECK(normalBinary.node[3].intValue == 102);
                release(inst, normalBinary);
                CHECK(ZoneAllocationCount() == 0 && ZoneAllocatedBytes() == 0);
            }
        }
        Scr_ShutdownAllocNode(inst);
        CHECK(ZoneAllocationCount() == 0 && ZoneAllocatedBytes() == 0);
        CHECK(gScrEvaluateGlob[inst].canonicalStringLookup == nullptr);
        int lookup[3] = {0, 1, 0};
        gDebuggerInstance = inst; gScrEvaluateGlob[inst].canonicalStringLookup = lookup;
        unsigned a = 1, b = 2;
        CHECK(Scr_CompareCanonicalStrings(&a, &b) > 0 && Scr_CompareCanonicalStrings(&b, &a) < 0);
        CHECK(Scr_CompareCanonicalStrings(&a, &a) == 0);
        Scr_InitEvaluate(inst);
    }
    // Allocation metadata is independent of the owner-thread parse-list API.
    std::vector<std::thread> threads;
    for (unsigned t = 0; t < 8; ++t) threads.emplace_back([t] {
        for (unsigned i = 0; i < 1000; ++i) {
            char *p = reinterpret_cast<char *>(Z_Malloc(79, "heap concurrency", 7));
            for (unsigned j = 0; j < 79; ++j) CHECK(p[j] == 0);
            std::memset(p, t, 79); Z_Free(p, 7);
        }
    });
    for (auto &t : threads) t.join();
    CHECK(ZoneAllocationCount() == 0);
#if defined(__ANDROID__)
    char *p = reinterpret_cast<char *>(Z_Malloc(12, "negative cases", 7));
    aborts([&] { Z_Free(p, 8); });
    aborts([&] { Z_Free(p + 4, 7); });
    Z_Free(p, 7); aborts([&] { Z_Free(p, 7); });
    aborts([] { Z_Malloc(-1, "negative", 7); });
    aborts([] { debugger_buffer(SCRIPTINSTANCE_SERVER, ENUM_statement, nullptr, 0, 3); });
#endif
    g_debugExprHead = nullptr;
    auto final = debugger_node0(SCRIPTINSTANCE_SERVER, ENUM_NOP);
    release(SCRIPTINSTANCE_SERVER, final);
    CHECK(ZoneAllocationCount() == 0 && ZoneAllocatedBytes() == 0);
    std::puts("PASS actual debugger/evaluator: node0..4, prepend, aligned buffers/strings, both instance cleanup/canonical/literal paths, normal-to-debugger int/float/minus/bool compile+eval and bool-or/and/binary/not compile-only, 8000 zeroed owned heap allocations, four-byte handles");
}
