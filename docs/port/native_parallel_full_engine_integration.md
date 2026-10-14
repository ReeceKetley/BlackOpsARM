# Parallel full-engine integration and retirement of test exclusions

User authorized parallel agents and explicitly requested retiring standalone test
exclusions from the full production build while keeping historical tests usable.

The full native-engine-entry CMake removes BO1_STARTUP_SERVICES=1 and
BO1_STRING_POOL_GATE=1 from bo1_native_startup_services only when
BO1_NATIVE_FULL_DATABASE=ON. The standalone startup-services CMake retains its
original definitions. This restores canonical bodies rather than adding another
substitute implementation. Common, command/Dvar and script native compilation
is being checked separately before the full production relink.

Completed parallel changes and evidence:

- JPEG decoder: real independent tagged owners and retirement; final HONOR UBSan
  and normal frozen Windows x86/x64 comparisons pass. Reproducible CMake fixture
  target builds. See native_renderer_jpeg_decoder_ownership.md.
- Surface saving: real JPEG/TGA encoders, checked temporary-file I/O and commit,
  real surface lock/ROI/error contracts. Final HONOR UBSan allocation/write
  failure checks pass; reproducible CMake fixture builds. Live GPU unverified.
  See native_surface_image_save_integration.md.
- Debug TCP: real native sockets and canonical partial reads/writes/acks/restart.
  Final HONOR UBSan and normal Windows x86/x64 win_net comparisons pass;
  reproducible CMake fixture builds. See native_debug_tcp_transport_integration.md.
- Seven tables/curve/asset helpers: original sources selected together; scoped
  native vehicle pointer-tail fields, graph parsing/file cleanup and yEnc bounds
  adapted. HONOR UBSan combined49 checks pass; seven normal Windows whole TUs
  both ABIs pass. See native_tables_asset_helpers_integration.md.
- Original reliablemsg.cpp and matching modified zlib uncompr.c are selected
  unchanged. Both normal Windows ABIs pass. Real RMsg_Init and uncompress archive
  definitions verified. FULL RMsg_Init diagnostic excluded only with real owner.
- Original Cbuf_Execute restored, including actual profile scope/client/server
  command execution. Native startup archive owner and normal Windows bothABIs
  verified before broader test-flag retirement; FULL diagnostic excluded.

Integration logs: build-android-audit/native_registry_closure/
parallel-database-build1.log, parallel-database-definitions1.log,
parallel-service-cmake-build1.log, reliablemsg-uncompress-windows1.log,
cbuf-startup-build1.log, cbuf-windows1.log, asset-helper-batch-windows1.log,
debug-tcp-windows1.log, full-dvar-windows1.log.

The new database archive compiles. These checks do not prove full engine linking,
actual Com_Init return, initialized retail consumers, script/server thread
registration, recovery, Com_Frame, GPU capture or gameplay. Latest full link375
predates this completed source batch and test-exclusion retirement. Common
restored bodies now compile; final full-engine-link378 fails real remaining
linker dependencies. See native_full_common_owner_retirement.md.

Generated production compile_commands audit: all14 startup-library translation
units have neither BO1_STARTUP_SERVICES nor BO1_STRING_POOL_GATE definitions.
Evidence native_registry_closure/full-startup-gate-retirement1.json. This confirms
the configuration change, independent of the pending common build/runtime proof.
