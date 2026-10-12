# TASK46: genuine startup-service integration

Verified parent: `8f81f9c`. This milestone selects actual production functions;
it does not implement a substitute `Com_Init` or claim full engine startup.

Implementation agent owns narrow production selections, necessary native
platform leaves, isolated build files and integration driver. Root owns
independent Windows oracles, device collection, source preservation proofs,
Ledger and final verification. Stable BlackOps64 and retail files stay read-only.

The intended boundary follows production ordering: actual main thread data,
`Com_InitParse`, `Dvar_Init` and its18 command registrations, real server/client
string-pool initialization, then `Cbuf_Init` and `Cmd_Init`. Verify the resulting
23 actual command nodes and callbacks, all three argument resets, selected real
command operations and string-reference lifetimes. Integrate the existing
genuine MP startup-zone reader with registered settings when closure permits.
Filesystem, localization, server forwarding and other unavailable endpoints
must remain real unresolved application dependencies or explicit test failures.

Independent review requires original Windows behavior for initial bool Dvars,
canonical Dvar layouts, the original two-buffer `CanKeepStringPointer` cutoff,
three misaligned eight-byte vec3 copies and SL ownership transitions. Never
compare uninitialized union bytes or widen the pointer predicate to four buffers.
The three native vec3 repairs must preserve exactly the original eight bytes
at offset4 and surrounding writes.

CRT prerequisite now passes230 exact records on original MSVC x86/x64, native
wrapper host x86/x64 and HONOR ARM64 O0/O3 with UBSan. These cover112 simple
format/canary cases,98 mixed integer/int64/float/vector/string cases and8 integer
conversion cases including overflow saturation and ERANGE, plus12 finite float
boundary/exponent/signed-zero cases. Exact-cap formatting
writes no terminator and returns capacity; overflow returns-1. This certifies
the selected caller formats, not arbitrary CRT format compatibility.

Reproduce Windows reference:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/port/build_startup_crt_oracle.ps1 -OutputDirectory build-android-audit/startup_crt_original4
powershell -NoProfile -ExecutionPolicy Bypass -File tools/port/build_startup_crt_oracle.ps1 -OutputDirectory build-android-audit/startup_crt_native_host4 -NativeWrapper
```

Use fresh output directories on rerun. Device comparison:

```powershell
& 'C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' -B tools/port/run_startup_crt_native.py --output build-android-audit/startup_crt_honor3 --original build-android-audit/startup_crt_original4 --serial AKWJ9X5116G04618
```

Detect the connected serial first. The collector records source/binary hashes,
commands and exact Windows/device output. FACT-0135 records this prerequisite;
Standalone integrated services now pass six original/native Windows and HONOR
O0/O3 runs with42 state records each, plus30 exact unavailable-dependency
controls. The three native `va_end` omissions are corrected. All11 touched
production units compile in normal Windows x86/x64 configurations. Evidence:
`startup_services_root_final1`, `startup_services_windows_full1`; MP71 joining
these initialized services is still in progress. No fullCom_Init/Frame claim.

Next boundaries after this milestone: genuine database worker/event/read
orchestration with explicit allocation ownership, actual `Com_Init` and bounded
`Com_Frame`, then server/world/script simulation before graphics.


# Next authentic startup-services integration

Read-only focused review after `c297348`, 12 October 2026. Research agent
`native_startup_services_review` made no edits, builds, device calls or Git
mutations. This supplements `production_startup_dependency_map.md`.

## Bounded target

One native process should execute actual `Com_InitThreadData`, `Com_InitParse`,
`Dvar_Init`, `SL_Init`, `Cbuf_Init`, `Cmd_Init`, command-driven Dvar operations,
then the existing genuine MP71 zone loader with actual registered Dvars.
This is integrated startup services, not completed `Com_Init` or gameplay.

## Required ordering and source contracts

- `win_main.cpp:2417-2441` initializes critical sections, main identity,
  PMem/tracking/localization, parse and then Dvars. `dvar.cpp:3250-3259` registers
  two bool Dvars and immediately calls `Dvar_AddCommands`.
  `dvar_cmds.cpp:823-842` registers 18 static nodes before `Cmd_Init`.
  `cmd.cpp:1282-1293` resets argument pools and adds five commands without
  clearing earlier registrations. Preserve all 23 nodes.
- `threads.cpp:100-114` registers main context and calls real
  `Com_InitThreadData(0)`; `:190` selects other role rows; `:707-716` binds TLS.
  `q_shared.cpp:682-690` binds va/error/trace/command slots. Context1 uses
  `g_cmd_args[1]`; other contexts retain the original shared argument object.
  TASK41's lookup selection excludes this body and does not certify engine TLS.
- `q_parse.cpp:35-49` initializes four parse contexts; `:75-95` selects
  main/backend/server/database and rejects unsupported roles.
- Android hunk main registration is immutable (`hunk_services.cpp:14-25`).
  Real native engine identity must match it. Duplicate live role registration
  must fail rather than substituting an always-true role predicate.
- Existing native recursive sections support SL27/MT28/DBHASH74. General
  services must preserve those same lock objects, and provide the other engine
  sections, including COM_ERROR12, ASSERT29 and CBUF57.

## Additional ABI and integration dependencies

`FastCriticalSection` (`win_common.h:8-11`) has volatile `long` read/write
counters: Windows `long` is four bytes; ARM64 LP64 `long` is eight. Writer bodies
are in `db_registry.cpp:1951-1982`, reader unlock in `dvar.cpp:729-742`.
Interlocked updates mix with plain volatile polling (`dvar.cpp:35-37`, `:3239`,
`:3267`, `:3279`, `:3301`, `:3325`, `:3359`). Audit the complete admission
protocol and provide consistently atomic fixed32 accesses. Do not cast an
eight-byte `long` to a four-byte atomic pointer. Existing `platform/atomic32.h`
provides fixed32 full-barrier operations; enum mutexes alone do not fix this.

Vec3 copies at `dvar.cpp:2111`, `:2246`, `:2248` dereference `__int64*` at union
offset4. An exact eight-byte `memcpy` repair needs independent Windows result
comparison and ARM UBSan coverage; broad layout changes are unnecessary.

Dvar string ownership calls real `CopyString`/SL interning
(`dvar.cpp:1538-1542`, `com_memory.cpp:1379-1387`) and real ref removal
(`com_memory.cpp:1403-1414`). Initial bool registration precedes SL; do not add
string Dvars there. `Cbuf_AddText` calls real `Scr_MonitorCommand`, whose
`cscr_debugger.cpp:702-714` body handles `map_restart`. Tokenization uses actual
expression evaluation for `(` (`cmd.cpp:675`, `:781-807`) and localization for
`@` (`:687`). Registered exec callbacks retain FS/FastFile dependencies.

## Implementation and acceptance sequence

1. Native role/TLS services and all recursive engine sections, binding the
   actual TLS initialization and existing allocator/SL lock identities.
2. Fixed32 FastCriticalSection storage plus complete atomic protocol, exposing
   original writer/reader bodies without requiring the whole database registry.
3. Actual parse, Dvar, command registry/tokenizer/Cbuf and Dvar commands through
   narrow production selections. Keep unsupported services explicit.
4. Genuine entry ordering, then SL and string Dvars, then MP71 loading with
   actual settings; prefer actual `Com_InitDvars` registration (`common.cpp:2262`,
   `useFastFile` at `:2371`) over manually fabricated test pointers.
5. Actual DB worker/events/file publication follows this service closure.

Validate Windows x86/x64 and HONOR O0/O3: exact role/TLS identities and shared
CmdArgs policy; duplicate/unregistered-role rejection; recursive lock/try-lock
contention and independence; bounded reader/writer contention and zero final
counters; two initial bool Dvars and all 23 command nodes; real quoted command,
set/toggle/reset/vstr/wait/argument/latch/domain/cheat behavior and SL cleanup;
vec3/color/XYZ UBSan coverage; all existing MP71 query/lifetime/pool checks in
the same process. Compare semantic records across ABIs.

Native libc `jmp_buf` isolation does not prove full `Com_Error` recovery.
`common.cpp:740-838` retains TaskManager/Glass/demo/console/error dependencies.
Keep owning C++ state outside jump scopes. Full `Com_Init` and genuine frames
remain separate acceptance milestones.
