# Native DDL, exposure and camera-shake owners

2026-10-13. Actual Com_Init/Com_Frame remains incomplete. No tablet run/transfers.

Full bo1_native_database selects six whole canonical sources: ddl_api, ddl_buffer,
ddl_lookup, ddl_converter, r_exposure and cg_camerashake. Real DDL initialization,
lookup/buffer/conversion, exposure and camera-shake owners are retained. Full ON
excludes the inherited unavailable DDL_Init endpoint because the real function
registers ddl_verbose; OFF retains its diagnostic. No fake ready state introduced.

Android DDL_MoveTo now uses actual va_list storage and va_copy, with both lists
closed on lookup failure and success. Original lookup traversal and partial result
behavior are retained. A Windows char* is not an ARM64 variadic-list representation.
Android's six DDL log wrappers use bounded std::vsnprintf, forward the result as
"%s" text, and close their lists even when verbose logging is disabled. This
avoids unterminated truncation and treating formatted percent signs as another
format. Original Windows calls/list representations remain unchanged.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link249.log preserves native CRT/va_list
  failures. link250 compiles the fix and exposes duplicate DDL_Init ownership;
  link251 excludes only the FULL diagnostic and reaches genuine remaining links.
- link252 compiles the final Android logging changes and fails real links.
- native_registry_closure/ddl-exposure-camerashake-definitions1.log confirms actual
  DDL initialization/movement/buffer/lookup/conversion, exposure and shake owners.
- takeover_ddl_exposure_camerashake_windows1: six whole normal TUs pass x86/x64.
  takeover_ddl_logging_final_windows1 repeats only the final modified DDL API;
  both ABIs pass exit0.

No initialized DDL/stat parsing/conversion, variadic/logging runtime, exposure or
camera-shake execution occurred. Actual ARM variadic traversal and failure/log
boundaries remain a focused runtime gate; compile evidence is not that proof.
Full entry still needs genuine audio/cinematic/live/profile/game/client/renderer/
network/recovery owners. Disk DDL fields/headers unchanged; no successful startup
claim and no unrelated probes repeated.
