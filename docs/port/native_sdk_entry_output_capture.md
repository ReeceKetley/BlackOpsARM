# Actual SDK stdout/stderr capture before engine startup

Rendered-only capture is implemented in native_engine_runtime.cpp. The exact
pre-edit source and current full430 baseline ELF hash were retained before the
change. This does not rebuild or replace the CLI artifact being used for the
baseline and same-artifact BO1_TRACE_POSTINIT comparison.

## Runtime behavior and ownership

After validating the explicit roots and actual SDK Activity adoption/context,
and after the rendered chdir succeeds, the entry installs actual file capture
before creating its watchdog thread or entering canonical engine startup. It
resolves the real user root, opens its directory without following a final
symlink, then uses openat for native-entry.log with O_WRONLY | O_CREAT | O_APPEND
| O_CLOEXEC | O_NOFOLLOW and mode0600. Existing files must be regular, owned by
the effective app UID, have one link, and have no group/other access.

Each temporary descriptor has one noncopyable owner and closes once at scope
exit. Directory/file descriptors are moved above fd2 if the OS allocated a
standard slot; source and backup descriptors use F_DUPFD_CLOEXEC with minimum3.
This keeps owner destructors from closing the final fd1/fd2 bindings. Existing
stdout and stderr must be real available descriptors. Missing standard output
causes genuine launch rejection instead of installing a fictional stream.

Actual dup2 calls bind fd1 and fd2 to the same append-file description. If the
first dup fails, original streams remain. If the second fails, stdout is restored
from its owned backup with its original descriptor flags. A failed rollback
closes the affected stdout binding and still rejects the launch. Temporary
source/backups/directory/file handles retire regardless of the outcome. Successful
capture retains only fd1/fd2 for process-owned standard output and emits the actual
capture-installed marker. No stdin producer is installed by this rendered path.

SDK argv/root/adoption/chdir/watchdog-argument/capture rejection uses the real
SDL_LogError Android provider while the full SDK profile exists, as well as the
existing stderr diagnostic stream. The historical non-full profile retains the
FULL guards and explicit rejection without introducing SDK provider references.
The outdated library-argv wording now says SDK program. Actual pinned SDK argv0
remains app_process; there is no normalization.

Dedicated execution never calls capture or SDK rejection logging. Its stdout
policy, environment/default roots, stdin producer, command-line bytes, watchdog,
process guard and real engine ownership are unchanged. The canonical tail from
policy/path installation through actual Com_Init, initialization-return
publication, Bo1_InitializationTraceReturned and the real frame/quit loop is
byte-identical to the exact pre-edit source. The root-owned post-init trace gate
remains after actual Com_Init and retains its BO1_TRACE_POSTINIT behavior.

## Verification and boundaries

The actual complete runtime TU compiles0 in isolation using the existing
production runtime compiler command, NDK30.0.16248370/API28, FULL_DATABASE,
COM_BOOT, PIC, O0, MS extensions/signed char, and UBSan/no recovery. No startup
stub/string-pool gate profile was added. The existing cscr_main.h138 const-struct
warning remains. Compile command/log, ARM64 ELF symbol log and precise before/
after diff are in build-android-audit/native_registry_closure/
sdk-entry-output-capture4311.

The real object references SDL_LogError, NDK fortified __open_2, openat, dup2,
fcntl, close, and the original trace-return owner. The source-tail byte comparison
and source/baseline ELF hash comparisons pass. Two audit setup corrections are
recorded in audit-notes.txt: inherited root LF lines were preserved rather than
normalized, and the actual fortified open provider was recognized. Neither was
a native compile failure or a provider substitution.

Encoding is CP1252. New lines are CRLF; the eight already-existing root-owned LF
lines are preserved exactly, including the trace-return insertion. Only the
runtime source was edited. No common trace source, CMake, device state, assets,
full production build or saved CLI binary was modified. Logging error paths are
source/ownership audits and actual TU compilation; capture on the Activity,
shared431 build, APK launch and rendered engine behavior remain unexecuted by
this change.

## Frozen evidence

- Before runtime SHA256: 2EF884B9B91F1319375A016A0FA26C7E203CE39075064C5AFAD0D8012C851945.
- Final runtime SHA256: 84AEE03D7B2EDD76201F897FF16AC43054FBDE54FBA209C45395D5EBC48569C5 (12722 bytes).
- Unchanged full430 baseline ELF SHA256: 2D32496485634C67AF842DE1A94EDCA7F0E80E8DDC6F47D25204DAE4A08DC852.
- Canonical startup/trace/frame tail byte comparison: passed.
- Actual runtime native compile: exit0.

The exact before source is native_engine_runtime.before.cpp. before-manifest.json,
final-manifest.json, provider-reference-check.json, runtime.diff and compile/
symbol logs retain the scope and reproduction data. Root owns shared431 build,
SDK launch, current-state/ledger publication and the first actual log-file proof.
