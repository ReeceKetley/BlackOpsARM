# TASK46: production startup services with genuine MP71 loading

Verified from `8f81f9c`,12 October2026. Actual main TLS, `Com_InitParse`,
`Dvar_Init`, server/client `SL_Init`, `Cbuf_Init` and `Cmd_Init` now execute
together with genuine registered settings and the existing production MP zone
reader. This is integrated production-function execution in a native test
executable. Actual `Com_Init`, `Com_Frame`, application startup and gameplay
have not executed yet.

The service selection reuses original Dvar/command/parser/string/memory/math
bodies. Necessary native changes preserve the original Windows branches:
three exactly eight-byte vec3 copies avoid unaligned64-bit access; string-pointer
classification uses numeric addresses with the original inclusive two-buffer
cutoff; parser/formatting callers close native va_lists; `Com_DvarDumpSingle`
uses the canonical native `DvarDumpInfo.match` field. The original log-handle
getter and storage, string-copy/free leaves, vector/filter/input-character leaves
and debugger command monitor are selected once. No renderer/input architecture
has been substituted. Selected unsupported application services abort explicitly
in test support; they are not implementations in the production library.

Standalone acceptance passes original Windows branches x86/x64, native host
branches x86/x64 and HONOR O0/O3 with UBSan. Each42-record trace matches the
original Windows oracle after pointer-width normalization. It covers23 real
command nodes/callback identities, three argument resets,13 direct operations,
actual deferred `vstr`/`wait` command-buffer execution, nine transitions across
three vector types with neighboring-byte canaries, nine SL/MT ownership stages,
two references for equal external name/value text and zero ownership at shutdown.
Thirty unavailable-dependency controls produce exact Windows3/Android134 exits
and source-backed first-dependency diagnostics. Registered callbacks are not all
claimed functional: filesystem, expression, localization, clock and server
dependencies remain explicit at this boundary.

Same-process zone acceptance passes Windows x86/x64 and HONOR O0/O3: actual
services, five registered Dvars,23 commands and both string pools precede all71
genuine `code_pre_gfx_mp.ff` assets. Existing four-cycle288-record oracle checks,
284 registrations,1792 queries per cycle and retained/removal lifetime checks
pass. The three required loader settings use exact original common/DB parameters.
They remain a driver-controlled subset; this is not whole `Com_InitDvars` or
`DB_Init`. Services remain alive during loading and shut down afterwards. ARM
link proofs show single canonical storage and one hunk service object.

Independent prerequisite checks pass230 CRT records including truncation,
mixed GP/FP varargs, finite float boundaries and integer overflow; original Dvar
layouts on three ABIs; and4100 byte offsets for the historical pointer policy.
Forty-two normal Windows function bodies and legacy source-comment bytes remain
unchanged; all11 touched complete Windows units compile for x86/x64. Protected
19930 paths/reference Git states and1514 older frozen records remain unchanged.
Failed first log dependency and two incorrect driver expectations are retained;
production setter rejection/latch behavior was preserved.

Evidence: `docs/port/evidence/arm64_native_startup_services/inventory.json`
freezes290 metadata records. Key local runs are `startup_services_root_final1`,
`startup_services_zone_host_run1`, `startup_services_zone_O0_run1/O3_run1`,
`startup_services_source_final1.json` and `startup_services_evidence_final1.json`.
Ledger FACT135-140 and the final checkpoint entry distinguish preliminary and
completed validation.

Reproduce integrated host build:

```powershell
./tools/port/build_native_startup_services_host.ps1 -OutputDirectory build-android-audit/native_startup_services_host_final4
./tools/port/build_native_startup_services_zone_host.ps1 -OutputDirectory build-android-audit/native_startup_services_zone_host1 -StartupServicesBuildDirectory build-android-audit/native_startup_services_host_final4
```

Use fresh output directories on rerun. ARM build and run:

```powershell
./tools/port/build_native_startup_services_zone_arm64.ps1 -BuildDirectory build-android-audit/native_startup_services_zone_O0_final1 -Optimization O0
& 'C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' -B tools/port/run_native_startup_zone_gate.py --build build-android-audit/native_startup_services_zone_O0_final1 --output build-android-audit/startup_services_zone_O0_run1 --serial AKWJ9X5116G04618 --optimization O0 --startup-services --binary-name bo1_native_startup_services_zone_tests
```

Detect the device first; repeat with O3 only when required. The existing collector
retains the retail checksum and independent decoded metadata. Only the necessary
genuine FF is copied to the designated device testing directory.

Next: select the complete unchanged actual `Com_Init`/inner startup bodies and
canonical globals, preserve the existing runtime dedicated/headless path, and
drive progress through executed failures. Canonical client-state declarations
currently import Windows XAPO headers, so a move-once header closure is required.
Required early localization, timing and platform memory setup must remain honest.
Then resolve real database events/worker/file loading/allocator ownership as they
are reached. Enter bounded actual `Com_Frame` only after `Com_Init` returns.
