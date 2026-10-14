# Original CubeMapGen CPU image and bounding-box dependencies

2026-10-13. Full engine still unlinked; actual Com_Init/Com_Frame incomplete.

Full target selects canonical CubeMapGenLib/CImageSurface.cpp and CBBoxInt32.cpp.
Native image allocation validates nonnegative dimensions and checked size_t products
against the existing signed32 pixel-index domain before allocating. No allocator
failure/readiness substitute. Existing clear/reinit/destructor ownership retained.
Native fatal errors print actual diagnostic text (escaped non-ASCII code points),
flush stderr and retain the original terminal exit EM_FATAL_ERROR (-15). Windows
keeps its original dialog callback. Native raw float bit conversions use bit_cast
instead of incompatible pointer aliasing; the reference half conversion arithmetic
is retained, not certified for all IEEE values.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link364.log records missing cstdio names;
  link365 compiles final two canonical owners. link366 is the latest production
  link attempt after fixture configuration, unresolved actual remaining services.
- takeover_cubemap_image_bbox_windows1: two normal whole TUs pass Windows x86/x64
  with final source manifests/exit0.
- cubemap-image-build1/2.log: focused CMake fixture uses actual CImageSurface source,
  GC/UBSan, no replacement image implementation or OS/mock allocator.
- cubemap-image-run1.log aborts on initial wrong flip-direction expectation.
  Reading source showed InPlaceHorizonalFlip swaps rows while InPlaceVerticalFlip
  swaps columns. Fixture selected the actual column-flip operation; production
  algorithm unchanged. Initial failed evidence retained. A guarded template-span
  selection also failed before mutation, then corrected before fixture build.
- cubemap-image-push2.log/run2.log: HONOR AKWJ9X5116G04618 ADB-shell UID exit0.
  Pitched 2x2 RGB float copies with row-padding sentinels, column flip, reinit,
  idempotent clear and terminal negative/overflow dimension rejection pass.
  Child processes print actual fatal diagnostics and exit241 (-15 low byte).
  Only changed fixture binaries transferred; no staged game assets recopied.

CubeMapProcessor constructor/filter/worker dependencies remain unresolved. Source
inspection identifies Win32 CreateThread/TerminateThread/CloseHandle and shared
worker/progress state requiring real native lifecycle, cooperative retirement,
synchronization and ownership adaptation. No worker count forced zero and no fake
completed filtering. D3DX image-file saving remains a separate real dependency.
No cubemap filter output, GPU image/save, rectangular flip domain, all half values,
unaligned typed external buffers, malformed stride or initialized renderer tested.
Those consumer/lifetime/format audits remain required before full filter claims.
Actual Com_Init has not returned; complete frame/runtime execution unproven.
