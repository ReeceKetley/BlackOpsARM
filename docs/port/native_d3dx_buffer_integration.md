# Native D3DX buffers and material integration

2026-10-13. Full Com_Init remains incomplete. DXVK Native 1.9.2 and its Vulkan
requirements were not changed; the support code is in the Android worktree.

Original model LOD, draw method, light/primary-light, XSurface and full material
loader sources are selected. The first five compile on ARM64. The material
loader currently fails compilation on Windows file-search/time/cache services.
Actual original algorithms remain in their source owner; no unavailable endpoint
was added to cover the newly exposed dependencies.

The native D3DX API declaration header retains the consumed SDK COM buffer slots
and fixed CTAB metadata sizes (28-byte table,20-byte info,8-byte semantic).
Constant-table interface currently declares only its inherited buffer surface,
which is what this loader consumes. These declarations are not implementations
of reflection, semantic extraction or HLSL compilation; those remain required.
Reference: installed June2010 DirectX SDK d3dx9core.h/d3dx9shader.h.

## Implemented and verified

Real D3DXCreateBuffer allocates byte storage, exposes its exact DWORD length and
preserves contents. IUnknown/ID3DXBuffer identity, retained query references,
atomic AddRef/Release and final destruction are implemented. Invalid create
output returns D3DERR_INVALIDCALL; allocation failure returns E_OUTOFMEMORY with
backing cleanup. A zero-length buffer has valid object ownership. Allocation
failure cleanup is implemented but forced-OOM execution was not tested.

The focused test uses actual production d3dx_buffer.cpp, with HONOR UBSan:
null output, zero size, 70001-byte patterned storage, supported/unsupported COM
identity, 40000 concurrent native reference cycles, intact contents and final
release. Evidence in native_registry_closure/d3dx-buffer-build1.log/run1.log.
The test source later changed only its Windows concurrency branch and PASS text;
the executed Android concurrency/body assertions remain the same.

Windows D3DX9_43 executes the common ownership/content cases and 40000 serial
cycles successfully in d3dx-buffer-windows-build2.log/run2.log. Initial threaded
Windows run1 failed its final reference-count assertion; preserved evidence means
concurrent Windows parity is not claimed. Native concurrency is its own verified
ownership gate. COM behavior also cross-checked against
[Wine's primary interface tests](https://github.com/wine-mirror/wine/blob/master/dlls/d3dx9_36/tests/core.c).

Six full Windows model/material TUs pass both ABIs in
build-android-audit/takeover_material_lod_windows1. Subsequent shared source changes
only add Android FILE/ASCII include-prefix adaptation; Windows branch unchanged.
Only the small buffer test binary was transferred; no game assets copied.

## Current blockers and next actions

Full-engine-link52 preserves missing d3dx9.h;53/54 reveal actual material cache
dependencies after compatible declarations. Latest55 fixes FILE and prefix
comparison but still fails on find-data/systemtime/filetime/delete/enumeration
services. The full target now fails at compilation, not the older link51 gate.

Implement real shader-cache directory/time/IO behavior under the engine user
root, preserving reference age/policy and protecting staged retail data. Then
complete genuine D3DX constant-table/semantic parsing and HLSL compiler integration
beside DXVK. Do not supply empty shader data, fake constants or success responses.
Original broader engine dependencies from link51 remain unresolved as well.
