# Existing custom rules, script evaluation/parser and SP field integration

2026-10-13. Genuine Com_Init/Com_Frame remains incomplete; no tablet run/transfers.

Full bo1_native_database selects eight whole existing sources:
ui_gametype_custom_mp, cscr_evaluate, cscr_yacc, g_sp_ext, g_trigger_mp,
g_client_fields, actor_fields and sentient_fields. Real custom-rule policy,
expression evaluation/script parsing, SP runtime extensions, trigger and field
registration/get/set behavior are retained. No new substitute algorithms or fake
readiness introduced.

Android lexer creation/init uses canonical FILE declarations instead of Windows
_iobuf. Its pointer-containing yy_buffer_state allocation now uses sizeof rather
than original40 bytes. The same original allocation/free and fatal-error owners
remain. Custom-rule metadata uses std::time(nullptr), actual native wall-clock
seconds, rather than Windows _time64; its existing DDL integer assignment policy
is unchanged. Original Windows signatures/allocations/time calls retained.

Evidence in build-android-audit:

- native_registry_closure/full-engine-link266.log preserves Windows file/time
  declaration failures; link267 compiles corrections and reaches genuine links.
- native_registry_closure/custom-eval-yacc-sp-fields-definitions1.log verifies
  actual custom policy/parser/lexer/SP extension/trigger/field registration owners
  in the native archive. Evaluation whole source compiles without source changes.
- takeover_custom_eval_yacc_sp_trigger_fields_windows1: eight whole normal TUs
  pass x86/x64 exit0 after final source changes.

No initialized lexer/parser/evaluation/custom-rule/SP/trigger/field runtime has
executed. Lexer buffer/input-size lifecycle, VM slots, SP extension publication,
field offsets/ownership and time narrowing remain runtime/caller audit gates.
Remaining links include other omitted original sources and services needing real
Android adaptation; unresolved does not mean all code is absent. Full startup
and frame execution still unproven. Earlier unrelated probes not repeated.
