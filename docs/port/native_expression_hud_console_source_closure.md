# Original expression, HUD and console integration

2026-10-13. Latest full-engine-link159 reaches linking but fails on remaining
genuine engine/platform dependencies. Actual Com_Init and Com_Frame unexecuted.

The full database target now selects complete canonical com_expressions_eval,
cg_hudelem, cl_console, con_channels and eval sources. Full cl_console replaces
the partial boot selection in entry_state, preserving one global console owner.
Original console Dvar/message-window initialization, restricted config logic,
channel filtering, arithmetic expression logic and scoped placement constructors/
destructors are retained. No additional placeholder or success state introduced.

Native compile adaptations:

- com_expressions_eval and eval use existing Android legacy CRT formatting and
  string comparison services; Windows continues its original calls.
- Android expression dispatch explicitly converts the existing function pointer
  to its native-width void pointer cache. The original union, fixed32 disk
  metadata and database expansion remain unchanged. Existing dispatch casts
  back to its original signature when called; runtime behavior unverified.
- Android live_counter declarations forward-declare bdRemoteTaskManager rather
  than transitively include the lobby/container implementation. live_storage
  supplies the actual base/types consumed here. Windows retains its previous
  includes. No DemonWare container or reference-count implementation was changed
  to make this unrelated expression source compile.
- CP1252 evaluator source encoding preserved.

Inherited diagnostic exports for GetExpressionResultStringCompile,
Con_Restricted_InitLists, Con_Restricted_SecureConfigs, Con_Restricted_ExecuteBuf,
Con_OneTimeInit, CL_ConsolePrint_AddLine, Con_InitChannels and Con_IsChannelVisible
are excluded only in FULL where selected original bodies own them. OFF diagnostic
selection remains unchanged. Duplicate failures preserved in153/155/158.

Evidence:

- ARM compile failures152 (formatter, implicit pointer conversion, unnecessary
  lobby header) corrected;153 reached diagnostic duplicate;154 reached remaining
  genuine dependencies. Original HUD stack and GetExpression* gates resolve.
-155 compiles unchanged full console and catches diagnostics;156 resolves them.
-157 catches eval string comparison dependency;158 compiles it and catches channel
  diagnostic duplicate;159 reaches remaining genuine link failures.
- takeover_expression_hud_windows1: whole normal evaluator, HUD and live_counter
  TUs pass x86/x64, exit0 with manifests and commands.
- Console source SHA256 remains
  9dd818e72de0494bb340eb040fa4354c7eed4070e37fa86397374ff96dda835d:
  matches both manifests in existing passing takeover_console_windows1. Reused
  these normal Windows whole-source results. Attempt to use that existing output
  directory was correctly rejected before compilation; no files overwritten.
- takeover_eval_channels_windows1: two whole normal Windows TUs pass both ABIs.

No new runtime probes or tablet/asset transfers. Selected native expression
execution/cache mutation, console initialization/printing/filtering and HUD stack
execution remain unverified. Existing decompiled eval exception sites still use
the reference's assertion placeholder and contain pointer-width warnings; this
selection does not certify error-path correctness or repair accepted reference
stubs. No runtime success claimed for those paths.

Next real link159 dependencies include cinematic IO, audio device capability,
playlist and further UI/client/game/script/physics/renderer/network/recovery
owners. Continue source/platform integration and execute production startup
once the actual entry links. Scope of compilation is not full initialization.
