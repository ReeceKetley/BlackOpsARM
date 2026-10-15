# Native UBYTE4 vertex fetch conversion

2026-10-15 stage65. Native backend/gameplay/mobile performance goal remains incomplete.

The actual Adreno610 supports normalized RGBA8 vertex fetch but not R8G8B8A8_USCALED (D3D UBYTE4). The vertex routing adapter now queries each required format. If the original scaled format is supported it stays unchanged. Otherwise, only that UBYTE4 route may use supported R8G8B8A8_UNORM and produce a per-VS-input conversion bit. UBYTE4N routes remain normalized. Failure remains explicit if neither fetch format is supported or the consumer cannot accept shader specialization metadata.

A project-local vkd3d compile option reconstructs marked directly addressed float VS3 inputs once at entry: multiply by255 and ROUND_NE into newly allocated temporary registers. Original instructions read those reconstructed temporaries with their original swizzles and source modifiers. Original vertex bytes and shader bytecode remain immutable. Relative vertex input addressing, non-float inputs and unused/invalid mask slots fail explicitly when conversion is requested; zero-mask compilation remains unchanged.

The C++ translator validates stage/range before scanning, passes the option through scan and compile, and clears failed output. Original shader reflection drives routing. GraphicsProgram links the specialized VS with its actual PS. ProgramDescription/PipelineKey carry the VS mask explicitly; copies, hashing and equality include it. The same resolved vertex snapshot yields the actual attribute formats and mask. Production material dispatch and both existing preview consumers pass the specialization metadata through.

## Actual tablet verification

native-ubyte-stage65-tablet2.log and native_vulkan_shader3/manifest-ubyte65-2.json record six actual GPU cases. Every case draws256 values in all four byte channels, samples actual vertex inputs, completes a real fence and reads float RGBA pixels. Channels use different byte permutations so all values0..255 are covered per component.

1. Original unsigned byte float values at VS input5, conversion mask32.
2. Negation and reversed swizzle applied after reconstruction.
3. Reconstructed byte input5 plus unchanged normalized input2.
4. Both input5 and input2 reconstructed, mask36.
5. Partial input declaration mask and replicated-X source swizzle.
6. Zero-mask normalized behavior stays unchanged.

Converted integer cases require exact equality (not an epsilon), including sums0..510; mixed/normalized floating results allow0.0001 tolerance. All cases pass on Adreno610/Vulkan1.1.128. All six generated VS modules pass Vulkan1.1 SPIR-V validation. Pixel-stage/range/unrouted conversion masks reject and clear output. Same shaders/attributes with a different conversion mask produce different pipeline keys. Bytecode vector identity is checked after actual pipeline creation.

The first run used a general float tolerance and passed; the second strengthened integer checks to exact equality and passed. Both results are preserved. The --ubyte-only helper avoids rerunning unrelated shader/math, depth-comparison and bias families. No game assets are copied and no installed engine process is restarted by this focused helper.

Engine build1 passes. SDK585/stage154 engine82ee72e7fa7714697302a5c6ad28c34f8202b68a PID21272 completes Com_Init and CA_ACTIVE168122ms, passes the previous vertex-fetch gate and advances to R_DrawXModelRigidModelSurf, where canonical line332 asserts the intentionally absent D3D index buffer. The unchanged BSP path reaching a later model family is integration progress, not proof of a completed native frame or final pixels. Stage66 adapts that XSurface index producer to its existing real native buffer owner. See current STATE for latest runtime result. Vulkan version/providers, Windows reference and original assets remain unchanged.
