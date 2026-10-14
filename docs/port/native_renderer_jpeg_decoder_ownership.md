# Native renderer JPEG decoder ownership

2026-10-13. Full production engine remains unlinked; actual Com_Init/Com_Frame and
initialized screenshot consumers have not executed.

Native R_ReadJpgSetup now owns a real tag11 zone allocation containing the actual
native codec/error structures. The modified original codec captures matching
Z_MallocJpeg/Z_FreeJpeg callbacks in its memory manager; setup restores previous
thread-local hooks immediately after codec creation. Codec storage and scanline
scratch are retired through their real frees, without hunk marks or no-op frees.
The Windows original read branches and native save/destination implementation are
preserved.

A thread-local registry verifies context membership before the public pointer is
dereferenced. Independent setup/read pairs may coexist on one thread and on
multiple threads. A context must be consumed or explicitly canceled with native
R_ReadJpgAbort on its creating thread before that thread exits. Input bytes remain
borrowed until completion/cancellation. Moving a context between threads fails
explicitly. The pointer API cannot distinguish a stale pointer from a newly
allocated context that later occupies the same address; callers must obey the
lifetime contract.

Setup checks pointers and signed32 input length before invoking the actual
modified source manager, uses sizeof for the codec, checks real header/start
results and bounds RGBA size to the screenshot consumer's signed32 allocation
domain before starting decompression. It retains reference RGB-only behavior. Unsupported grayscale returns
null after cleanup and clears width/height/format outputs. Read validates output
storage, positive pitch, full image height capacity, row width and native address
span before output writes. It decodes all rows through the actual codec, preserves
reference RGB-byte/opaque-alpha layout, respects row padding, finishes and frees
the entire owner. The API does not receive destination allocation length; callers
must provide the validated pitch/height storage.

Custom native decoder error_exit prints the codec diagnostic, destroys its codec,
frees scratch/context, restores allocator hooks if creation was interrupted, then
uses the real fatal engine callback. Argument/suspension rejection likewise
cleans an owned context before reporting fatal error. No initialized engine
longjmp/recovery claim follows from this scoped test.

Evidence in build-android-audit/native_registry_closure:

- renderer-jpeg-decode-direct-build2.log: final NDK30/API28 ARM64 direct build of actual
  r_jpeg.cpp and original modified codec with UBSan/no-recover, GC sections and
  static C++ runtime. Existing header/decompiled warnings only; exit0.
- renderer-jpeg-decode-push2.log and renderer-jpeg-decode-run2.log: final changed binary
  only sent to HONOR AKWJ9X5116G04618, ADB-shell UID exit0. Actual native
  setup/read/abort, 2x2 RGB pixels, padded rows/outer canaries, two interleaved
  contexts, two concurrent threads/eight iterations each, previous hook
  restoration and grayscale retirement pass. All fixture-tracked tag11 allocations
  retire.
- Fatal subprocess cases pass: malformed header, missing EOI during finish,
  insufficient pitch/height, oversized signed32 RGBA allocation dimensions,
  cross-thread/retired context access and injected
  context, codec memory-manager and scratch allocation failures. Cleanup cases
  observe zero live tag11 allocations before fatal callback. Shared output bytes
  remain untouched on argument/owner/allocation rejection.

The fixture tools/port/tests/native_renderer_jpeg_decode_tests.cpp isolates engine
memory initialization with counted malloc/free tag11 providers. It invokes real
decoder/encoder code rather than dummy codec results. Its success does not prove
initialized zone heap/filesystem, all JPEG modes/dimensions, retail screenshot
loading, thread teardown with abandoned contexts or actual rendered gameplay.

Root integration evidence: reproducible bo1_native_renderer_jpeg_decode_tests
CMake target build2 passed. Normal r_jpeg.cpp x86/x64 comparison in
build-android-audit/takeover_jpeg_decoder_windows2 passed with source hashes.
An initial comparison was rejected because source changed during compilation;
that evidence is retained, and only the frozen-source comparison is accepted.
Full-engine-link375 still fails genuine dependencies; no startup runtime proof.
