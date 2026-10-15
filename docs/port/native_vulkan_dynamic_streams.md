# Canonical dynamic stream consumers

Stage18, 2026-10-14. Native backend integration remains incomplete.

`R_SetStreamSourceFromOwner` passes the canonical vertex-buffer slot to the
native geometry binding map, retaining the published record and its GPU backing.
It preserves offset and stride, clears the two unused streams, and leaves COM
handles null. The default branch delegates to the original D3D9 helper.

Native branches now use this adapter in the generic tessellated draw technique,
uncached skinned model draw, both skinned static-model draw lists, mesh stream
binding and cached skinned model draw. Cached skinned indices also bind the
actual XSurface index slot. Existing source assertions and D3D9 calls are retained
in the default branch. Required producers are not faked: cached skin/water/mesh
uploads that are still unported cannot produce a valid native binding.

SDK509/stage78, BuildId a9c4fcc5b51f31be5ca308796fb893cf21a59a11, passed the actual
ARM64 engine build, packaging/provider checks and authorized tablet execution.
The captured-material draw uses the canonical helper at vertex byte offset96
with stride32 after a dynamic append. The test checks the actual borrowed record,
native offset, null COM slot and cleared extra streams. The resulting image is
visually unchanged from the earlier captured-room material. Borrowed geometry
survives owner release and is retained by the draw/frame arenas.

This executes the shared adapter and its indexed GPU submission on the tablet;
the entire live tess/skinned/static-model call chains are compiled but have not
yet executed through native Com_Init. Diagnostic FPS is not game FPS.

The legacy body proof removes only native guards and the two new owner-aware
wrapper definitions: preexisting r_state.cpp, rb_shade.cpp, rb_tess.cpp and
r_draw_staticmodel.cpp match checkpoint4247b269. Evidence:
`build-android-audit/native-stream-stage18-legacy-proof.json`,
`native-stream-stage18-build1.log`, and
`native_registry_closure/sdk-native-vulkan509-{presentation,exit}.log` plus screen.

Next adapt remaining actual startup allocations and queued/skinned/water/mesh
uploads, render-target initialization/sharing/resolve, production material
pipeline submission and window/device startup before genuine native Com_Init.
