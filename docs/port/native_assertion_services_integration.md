# Native assertion lifecycle

`platform/android/assertion_services.cpp` supplies the genuine Android
`Assert_MyHandler` and `Assert_SetMonkeyCallbackHandler` counterparts. The working
Windows handler remains unchanged: it logs and breaks. The selected canonical
`assertive.cpp` address-state/reset owner remains unchanged; native assertion code
does not truncate actual 64-bit program counters into its fixed32 address table.

Native assertions format bounded local expression/source/line/type text, write
directly to stderr without engine print/critical-section locks, and capture the
actual stack through existing `CaptureStackTrace`. The reporter pointer is
published with release/acquire atomics. Callback code must have process lifetime;
clearing/replacing the pointer does not retire code already acquired by a handler.
No lock is held across callbacks. Recursive/concurrent assertions take immediate
actual `Sys_Error` termination rather than waiting behind a lock. Reporter
exceptions likewise terminate. Normal reporter return still proceeds to actual
`Sys_Error`; abort follows if that fatal owner unexpectedly returns.

The existing native fatal owner logs its own stack and exits the process with
code2. There is no successful assertion return, suppression, dummy readiness,
assertion bypass or claim of engine recovery. Nonlocal reporter escapes are not
certified as recovery: an escaped handler leaves the process assertion state set,
and a later assertion takes the recursive/concurrent fatal path. Static engine
reporters must not use nonlocal escape as an assertion continuation mechanism.

Integration recipe for root:

- Select `src/platform/android/assertion_services.cpp` beside the existing native
  stack/fatal providers.
- Retain canonical address globals/reset source; do not add another global table.
- Exclude only the FULL `Assert_MyHandler` unavailable endpoint once its actual
  new owner is selected. No central CMake or diagnostic source was edited here.

The actual native owner compiles ARM64/API28 with UBSan. No Windows source,
associated canonical header, test-slicing flag or protected checkout was changed.

## Concrete failure-path execution

`tools/port/tests/native_assertion_services_tests.cpp` directly links the actual
assertion source, stack capture and fatal source. It runs real assertions in
joined forked children with real stderr capture pipes and shared callback evidence.
The parent verifies actual fatal exit2, not a fixture fatal replacement. It also
executes concurrent atomic reporter publication/clear, then verifies reporter
registration and clearing, expression metadata, actual unwind frames with a PC
above4GiB, recursive reporter termination, exception termination and bounded long
expression logging. The stress case is not a TSan race proof or module-unload proof.

Build from BlackOpsARM:

```powershell
./tools/port/tests/build-native-assertion-services.ps1
```

NDK30.0.16248370 / API28 / ARM64 / C++20 / signed char / UBSan no recovery /
static libc++ / native MSVC spelling adapter / section garbage collection.

HONOR `AKWJ9X5116G04618` passes exit0 with
`ASSERTION_SERVICES PASS checks=34`. Logs:
`build-android-audit/native_assertion_services/build1.log`, `fixture-build1.log`,
and `honor-run1.log`. Only the new fixture executable was transferred.

Actual Monkey network reporting, initialized engine assertion consumers, callback
nonlocal escape, engine lock/TLS/error recovery and full Com_Init remain unverified.
Fatal termination is not successful shutdown or recovered state. Assets, ledger
database, STATE, commits and protected checkouts were untouched.
