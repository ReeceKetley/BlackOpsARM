# Native rigid XModel index binding

2026-10-15 stage66. SDK585 passed the prior UBYTE4 vertex-fetch failure and reached the canonical R_DrawXModelRigidModelSurf D3D index-buffer assertion. In native mode COM fields remain NULL while real buffers live in typed owner-slot records.

The native branch now calls R_ChangeIndicesFromOwner for the actual XSurface::indexBuffer slot. It rejects missing/wrong native ownership through the existing geometry registry, which retains immutable GPU records and payload bounds. The existing native R_SetStreamsForXModelSurface still binds the real32-byte vertex stream; original per-primitive constants, draw arguments and statistics remain canonical. No index value, range check or failed draw is skipped.

The native compile flag is confirmed in the actual Ninja rule. The Windows body compares equal3175 tokens against checkpoint4247b269; source CP1252/CRLF and engine build pass. Source proof: native-xmodel-stage66-proof.json. No original assets, Windows reference files, provider versions or default backend change.

SDK586/stage155 engineade049dd911d3960f66a25eee088d061fb7381aa PID22048 reached CA_ACTIVE167755ms and passed this index binding into R_ResolveDistortion, then failed the legacy Image_GetSurface assertion. Matching stack: native-xmodel-stage66-next-stack.txt. Actual traversal reaches model and world work; a complete native room frame, gameplay/lifecycle and native performance remain unproved. Stage67 supplies the real scene image copy.
