# ARM64 normal-to-debugger expression compilation gate

Completed 2026-10-11 under TASK-0007. The actual production expression
compiler is now exercised: yacc-shaped normal parse trees built with the real
`node*`/`node_pos` constructors in the HunkUser arena are compiled by the real
`Scr_CompileExpression` / `Scr_CompilePrimitiveExpression`, and the resulting
debugger (Z heap) trees are evaluated by the real evaluator for
integer / float / minus-integer / minus-float / true / false inputs on HONOR
and Windows x86/x64. This gate starts expression compilation; it does not
certify text parsing, string interning, callable slots, operator evaluation or
the VM. Exact incremental change: `tools/port/tests/debugger_tests.cpp` (+141).

## Source changes

- Only the standalone probe changed; no engine source was modified.
- Normal trees mirror `cscr_yacc.cpp` shapes exactly: `node2(integer,value,pos)`,
  `node2(primitive_expression,prim,pos)`, `node1(true/false,pos)`,
  `node2(bool_not,child,pos)`, `node5(bool_or/and,left,right,pos,pos,pos)` and
  `node4(binary,left,right,node1_(opcode),pos)`.
- Compilation drops source-pos slots exactly as production does
  (`debugger_node1/2/3` keep type/value/opcode); four-byte `sval_u`
  size/alignment is statically asserted and decoded pointers are
  4-aligned. The x64 host caught one probe-side `reinterpret_cast` of a
  `Narrow32` handle; the fix decodes via `static_cast` first.
- Literal-derived trees are evaluated (`Scr_EvalExpression` /
  `Scr_EvalPrimitiveExpression`) for 0/1/-1/37/-999/INT32_MAX/INT32_MIN,
  0.0/-0.0/1.25/-37.5, minus magnitudes 1/127/999, minus-float 1.25 and
  false/true, for both instances, with `ZoneAllocationCount/Bytes` back to
  zero after every case via the existing `release()` ownership transfer.
- Operator trees (bool_not/or/and, binary opcode 102) are
  compilation-verified and freed only. Their evaluation needs
  `Scr_CastBool` / `Scr_EvalBinaryOperator` / `RemoveRefToValue`, which remain
  generated aborting fixtures: no engine body was copied and no success stub
  was added. String/istring (`SL_ConvertToString`), variable/call
  (`AllocValue`) and expression-list (`GetExpressionCount`) paths are likewise
  explicitly unsupported and never reached by passing tests.
- List/string/callable ownership was inspected and deliberately left
  unimplemented; full VM, text parsing and gameplay remain deferred.

## Verification

Final NDK r30 / Clang 21 / API 28 / arm64-v8a / Debug C++20 build
`build-android-audit/expr-20261011-184501781`: **0 compiler errors, 1261
retained warnings** (unchanged from the debugger_pass10 baseline), successful
link. Actual parsetree/evaluator objects and probe bodies keep
non-recovering UBSan; passing paths report no findings.

All **13 native probes exit 0** on HONOR NDL-L09 / API36 / page size 4096,
serial AKWJ9X5116G04618, device dir
`/data/local/tmp/blackopsarm-phase1-20261011-184513410`, including the
extended debugger probe. Frozen log:
[device-tests.log](evidence/arm64_expr_compile/device-tests.log).

Host checks pass from this worktree (vswhere requires
`C:/Program Files (x86)/Microsoft Visual Studio/Installer` on PATH):
`check_debugger` x86/x64 (debugger + firstfit),
`build-android-audit/host-debugger-20261011-184418437`;
`check_parsetree` x86/x64, `host-parsetree-20261011-184549529`;
`check_script_headers` x86/x64, `host-script-20261011-184610230`;
`check_narrow32`, `host-narrow-20261011-184635216`.
`git diff --check` clean; `ledger doctor` healthy.

## Runtime boundary and next work

Fifty unavailable compiler/VM endpoints still abort if reached; the three
cleanup callbacks remain test-owned fixtures. These files must never be linked
into an engine/APK. String interning, callable-slot registry/list traversal,
nonliteral operators/variables, operator evaluation services, text parsing and
full VM execution remain unvalidated. The production Windows Z heap keeps its
legacy 164-byte prefix; host debugger tests use the Android heap and do not
certify Windows heap/debugger integration. Default hunks and the global
debugger compile list stay owner-thread confined; Narrow32 does not revoke
stale handles. Real 16 KB Android hardware, Samsung S25, replay/profiler
reporting and graphics/audio/input remain untested.

Next: string interning / callable-slot ownership inspection, then operator
evaluation services only as justified by tests. WARN-0008 supersedes WARN-0007.
TASK-0007 is complete; no commit or push was performed (no authorization).
Frozen summary:
[compiler_summary.json](evidence/arm64_expr_compile/compiler_summary.json).
