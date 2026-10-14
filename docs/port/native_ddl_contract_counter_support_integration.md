# Original DDL commands, contracts, counters and UI/render support owners

2026-10-13. Full engine remains unlinked; actual Com_Init/Com_Frame incomplete.

Full database target now selects seven canonical sources: ddl/ddl_cmd.cpp,
live/live_counter.cpp, live/live_contracts.cpp, qcommon/profanityfilter.cpp,
qcommon/legacyhacks.cpp, gfx_d3d/r_devgui.cpp and gfx_d3d/r_globalgfxdata.cpp.
No source bodies, feature guards, assertions or initialization return values changed.

Original DDL_Cmd_Init registers ddlLoadAsset and ddlShow with actual callbacks and
command nodes. Original counter initialization/updates, contract commands and
renderer developer-menu command insertion are selected, with their real remaining
dependencies. Exposure settings and download bookkeeping use original single
global owners. Zero-initialized storage is not a claim of initialized renderer or
download services.

The profanity filter deliberately returns false in the validated BlackOps64
reference; that accepted existing policy is preserved unchanged, not newly
implemented backend filtering or a new missing-service workaround. Existing
counter/backend feature guards likewise remain reference policy and do not prove
online counters or contract fetch readiness.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link349.log compiles the first three sources;
  link350.log compiles the additional four and still fails true remaining links.
- ddl-contract-counter-support-definitions1.log confirms DDL initialization and
  callbacks, counter init/update, contract filter, renderer developer-menu setup,
  original profanity policy and actual exposure/download state owners.
- takeover_ddl_contract_counter_support_windows1: seven normal whole translation
  units pass Windows x86 and x64, with final source manifests and exit0.

No native behavioral rewrite or new ABI boundary was introduced, so no unchanged
device fixtures or game asset transfers repeated. Actual command registration/
execution, DDL asset parsing, counter synchronization/backend callbacks, contract
data/state, renderer menus and exposure consumers remain unexecuted. Retained
decompiled arithmetic and initialized lifetimes still need runtime audit. Native
audio/cinematic/renderer, Steam, debug-network, Monkey and other source/platform
dependencies remain unresolved; use the entire latest linker log, not its ordering,
to choose the next dependency. Full Com_Init has not returned.
