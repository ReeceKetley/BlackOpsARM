# Demo recording, animation command and session owner batch

2026-10-13. Root owns CMake/source selection, diagnostic exclusion, protected
checkout boundaries and durable ledger/state. No CMake/ledger/state/assets changes
by this worker. All assigned source files are frozen pending root final checks.

Original demo_recording.cpp/demo_profile.cpp/msg_anim_commands.cpp/live_sessions.cpp
were absent from the central source list. All four now compile as ordinary native
whole TUs with current full target options. Profile and animation source bodies
remain unchanged. Their runtime pointers/structures are native typed; animation
records and serialized fields remain fixed32. Native save job retains actual
JQ_CMD_SIZE and asserts demoSaveCmd16-byte size/eight-byte alignment.

Recording includes the existing genuine bounded CRT formatter adapter and uses
actual native time(), rejecting timestamps outside unsigned32 file representation.
Native save bounds input to the original message buffers and fails compression or
short writes explicitly. Native connected-player XUID assignment preserves all64
bits directly. Windows original bodies remain in their branches. CP1252 edits
round-trip bytes and verify untouched non-ASCII bytes.

Native streaming uses real mutex/CV ownership for its original393216-byte buffer,
indices, active/overflow state, occupied count and send history. Capacity is checked
before copying; one byte is reserved to preserve the canonical empty-index
convention. Wrap copies publish only complete accepted bytes. Stop wakes blocked
readers while queued bytes drain; restarting an active or undrained stream rejects
discard. Stream generations cancel old blocked readers after stop/restart.
Throttle consumes a coherent history/occupancy snapshot; logging/sleep/callbacks
occur after locks release. Atomicity of data/control derives from the real mutex,
not volatile fields or assumed x86 ordering.

Demo_StreamUploadDataCallback has the actual three-argument fileshare ABI. Inspection
of LiveStorage_FileShare_WriteFileData confirms that its third parameter is
bytesUploaded. The adapter serializes calls and compares that offset with actual
retired-byte count; retries/range/out-of-sequence calls reject explicitly after
releasing locks, since streaming storage cannot replay retired bytes. Root must
assign this adapter in demo_common.cpp's native branch. The accepted Windows
fileshare implementation remains disabled; this owner does not fake backend
readiness or implement unavailable upload transport.

Native metadata validates all bounded array counts and total record capacity
before changing/writing bytes. It emits the original fixed field order, raw
fixed-layout24/12/24-byte records and payload-size/version footer, with static
layout assertions. Exact-fit output works and insufficient output leaves storage
untouched. File metadata is encoded completely and checked for an exact write.
Demo_WriteToBuffer avoids signed/unsigned addition wrapping and validates storage.
Sessions replace potentially unaligned32-bit payload casts with actual memcpy.

Evidence:

- Four *-native-audit-final1.log and *-native-definitions-final1.log files record
  ordinary full-TU compile exit0 and canonical defined owners. Recording initially
  failed only _snprintf/_time64; initial logs retained.
- demo-batch-fixture-build1/2 fail logs are preserved. Whole recording TU brings
  its real job descriptor callback/root dependencies into a scoped link; these
  were not replaced with dummy providers. The fixture instead extracts exact
  stream/metadata owner bodies into demo_ring_metadata_actual.cpp, while linking
  ordinary profile/animation/session/msg_mp/huffman/com_shared originals.
- demo-batch-fixture-build3.log links that scoped fixture with ARM64 UBSan/no-
  recover. Platform printing/clock/backend notifications and FS initialization
  are isolated; metadata uses actual POSIX file I/O. Changed binary only sent
  to HONOR AKWJ9X5116G04618.
- demo-batch-fixture-run1.log reached the actual Huff build and failed UBSan:
  huffman.cpp:88 writes loc[257] for internal nodes, outside byte lookup0..256.
  Huff_Init also clears fixed19476 Win32 bytes rather than native sizeof. Root
  owns that newly exposed source repair; no sanitizer weakening or dummy Huff
  success is used. The failed run remains preserved.
- Root repaired native Huff clear to actual sizeof, kept internal257 nodes out of
  the byte-symbol lookup and bounded native node allocation. Final reproducible
  demo-batch-fixture-build.ps1 runs the exact-body extractor and records
  demo-batch-fixture-build4.log exit0. Final changed binary push2/run2 on HONOR
  passes UBSan with no owned source edits after the frozen hashes.

Final cases exercised ring wrap/canaries, overcapacity
rejection before copy, empty-reader stop wake, concurrent400000-byte FIFO across
wrap, typed callback sequential offsets/retry rejection, bounded metadata/footer
and actual704-byte file. The complete scoped fixture passes. It also covers
profile counters, genuine Huff/MSG initialization and actual animation types1..8/
future signed timestamps/truncation and changed QoS payload.

Frozen SHA256:

- recording.cpp:5A0887A444A0D473E8EC7E621DC2107F905E937636FF2219D11E5EC8AC4D25F9
- recording.h:2F36FD0AD9A19F7946A53D5D4E184C28D5C44224362D240AD8DCDF34428D6124
- profile.cpp:D3BA305F86F1EFF11C0030EDF08263B2769E1B7A1A6715BAA70A1CBDF480B799
- msg_anim_commands.cpp:32F29B87DC6DC31AE322DD3B489CA8537036FB97E14FB7F3DB0A4516207EB601
- sessions.cpp:74E1EF545F462B6D9594D582BC0D61E9DC6B55DF7304D7C85DE2D3FDB443387B

No actual Com_Init return, production demo recording/playback/async-save/file
storage, live upload backend, initialized recovery or consumer-wide lifetime is
proved. Extreme animation float/timestamp domains, all metadata modes and generic
parallel session payload readers remain unverified. Root runs final normal
Windows whole-TU comparisons for all four sources, both ABIs.

Root integration: four canonical sources + typed demo_common upload callback
selected in full engine. Normal Windows four originals/demo_common/huffman
whole TUs bothABIs pass (takeover_demo_huffman_windows1). Actual native Huff_Init
uses sizeof and valid symbol lookup only; internal257 node is not a byte slot.
Full-engine-link380 compiles this batch, remains unresolved genuine dependency
closure; no Com_Init/Com_Frame runtime proof.
