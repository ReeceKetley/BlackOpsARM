# Original renderer JPEG owner and native destination handling

2026-10-13. Full Com_Init/Com_Frame incomplete; production executable unlinked.

Canonical r_jpeg.cpp selected in full database target beside matching modified
codec. Native R_SaveJpg keeps actual codec initialization/defaults/quality/scanline/
finish and canonical filesystem flow. Context size uses sizeof, dimensions/raw
indexing checked against codec/signed32 domains before arithmetic. Native typed
destination resides at the actual compressor dest field with actual allocation
size, replacing decompiled common-context offsets and fixed28 destination size.

Owned output starts at512 bytes and grows through actual tagged Z allocation,
copy and retirement up to the signed32 limit. External output retains reference
raw-RGB capacity contract (caller must own at least3*w*h bytes); exhaustion takes
actual ERR_FATAL then abort if returned, never resizes/overruns borrowed storage.
Actual destination empty-buffer callback replaces the incompatible Flame callback
cast. Termination measures actual capacity-minus-unused bytes; hackSize is native
thread-local. Native save frees owned output, destroys codec and restores previous
allocator hooks before reporting canonical FS_WriteFile failure through ERR_FATAL.
Windows original branches remain unchanged.

Native R_ReadJpgSetup context sizeof also corrected, but its original shared read
context/hunk-mark/free callbacks and R_ReadJpg pitch/height/ownership still require
full native lifetime/argument/error audit. Source selection is not decoder-ready
proof; no dummy hunk mark or replacement read success added.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link371.log records missing existing tagged
  allocator declarations; r_jpeg.h now owns canonical declarations. link372 compiles
  typed destination; link373 compiles final dimension/growth limits. link374 is
  latest production attempt after fixture config, unresolved genuine other owners.
- renderer-jpeg-definitions1.log confirms actual save/read/destination/allocator
  owners. Read owners remain unexecuted.
- takeover_jpeg_renderer_destination_windows1: two normal whole TUs x86/x64 pass;
  final changed renderer source rechecked bothABIs in
  takeover_jpeg_renderer_final_windows1, source manifests/exit0 retained.
- renderer-jpeg-build1.log: focused CMake target uses actual r_jpeg.cpp and original
  modified codec with GC/UBSan. Fixture tag11 allocation uses real malloc/free;
  FS provider uses actual fopen/write/close, isolating engine FS initialization.
- renderer-jpeg-push1.log/run1.log: HONOR AKWJ9X5116G04618 ADB-shell UID exit0.
  Actual R_SaveJpg2x2 RGB produces >512 bytes (real growth), writes a real JPEG,
  actual codec decodes dimensions/pixels within JPEG tolerance, no live tagged
  allocations remain and previous hooks restored. Shared-mmap borrowed3-byte
  destination exhausts through fatal callback with surrounding canaries intact.
  Injected file-write rejection reaches fatal callback only after all tracked
  allocations are retired. Test file removed; changed fixture only transferred.

No initialized production FS, APK UID/storage, GPU capture/D3DXSaveSurfaceToFileA,
real JPEG read/hunk pipeline, arbitrary parallel save calls, allocation failure,
all qualities/formats/extreme dimensions or recoverable longjmp error path proved.
Mid-encode errors remain terminal; no initialized recovery cleanup claim. Filename/
filesystem policy remains canonical. Renderer/API image save adapter, decoder
ownership and genuine audio/cinematic/input/Steam/debug/Monkey and other source
dependencies still prevent full link/Com_Init return.
