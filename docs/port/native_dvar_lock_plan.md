# Authentic Dvar synchronization prerequisite

Read-only focused audit after TASK43, 12 October 2026. No edits, builds or
device operations were performed by `native_startup_services_review`.
This is an implementation plan, not a passing native Dvar milestone.

The production FastCriticalSection is a separate reader/writer admission
protocol; recursive enum critical sections cannot replace it.

## Original protocol and callers

Readers increment readCount before polling writeCount until zero. Waiting
readers therefore remain counted. Writers (`db_registry.cpp:1951-1962`) wait
while readers exist, increment writeCount, and enter only when that increment
returns1 and a second reader check is zero. Otherwise they undo the increment
and yield. Reader/writer release asserts signed-positive count before subtracting
one (`dvar.cpp:729-742`, `db_registry.cpp:1965-1978`).

`NET_Sleep(0)` is real scheduler `Sleep(0)` (`win_net.cpp:119-122`); a native
contended path must actually yield. Objects are static zero-initialized; no
explicit counter reset was found. Dvar shutdown resets registry state while
holding the writer and releases normally.

Dvar reader admissions occur at `dvar.cpp:35-37`, `:3238-3239`, `:3266-3267`,
`:3278-3279`, `:3300-3301`, `:3324-3325`, `:3358-3359`. Additional direct accesses
are savegame admission/release (`game_sp/g_sp_savegame.cpp:408-410`, `:437`) and
profile cache polling (`live/live_pcache.cpp:46`).

Writer users include Dvar sort/shutdown/registration (`dvar.cpp:57/75`,
`:1397/1422`, `:2220-2288`), DB reorder (`db_registry.cpp:1922/1946`, `:2120/2130`,
`:4550/4552`, `:4809-4847`), profile cache (`live_pcache.cpp:36/41`), renderer
resources (`rb_resource.cpp:21/26`), image loading (`r_image_load_obj.cpp:842/844`)
and stream allocation (`r_streamalloc.cpp:94/99`). Six actual lock objects exist:
Dvar, DB reorder, profile cache, resource, image and stream allocation.

## Bounded implementation

Extract the canonical FastCriticalSection from `win_common.h:8-12` into a
lightweight header. Windows keeps its exact volatile long members. Android
needs fixed32 storage with size8/alignment4/offsets0/4; do not cast an eight-byte
LP64 long to a four-byte atomic pointer.

Use existing `platform/atomic32.h` native seq-cst loads/increments/exchange-adds.
Preserve original Windows typed Interlocked operations and original writer
control flow/release diagnostics. Expose original writer and reader unlock
bodies through narrow selections in their existing translation units.
Every concurrently accessed counter must use atomic operations, including polls
and assertion reads. Increment-only adaptation leaves races.

If native storage is unsigned, signed-positive assertions must check nonzero
and <=INT32_MAX; a merely nonzero negative bit pattern must remain invalid.
Inventory unselected savegame/profile accesses as deferred, or adapt them in the
same reviewed change before those units execute concurrently.

## Acceptance and limits

Compare original Windows protocol with native x86/x64/HONOR O0/O3. Verify
fixed32 layout plus adjacent canaries, simultaneous readers, writer blocking
until all readers release, queued readers behind an active writer, competing
reader/writer coherent complementary payloads and exact final update counts,
independent objects, nested balanced readers and zero final counters. Negative
counter/unbalanced releases must remain explicit failures. Bound concurrency
tests with a watchdog. UBSan alone does not detect data races; use TSan where
available.

Preserve nonrecursive writer/no-upgrade/no-fairness contracts. This protocol
protects registry admission, not arbitrary concurrent Dvar mutation: lookup
releases admission before returning a pointer, and several nominal read
operations mutate values/flags. Registration's hash-collision fatal path can
call Com_Error while holding the writer (`dvar.cpp:2279` vicinity); do not
certify native error recovery or reinitialization until actual cleanup closes.
