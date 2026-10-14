# Native dynamic entity bounds copies

Frozen source `src/DynEntity/DynEntity_server.cpp`: SHA256 `696D5B2ED62F64477B06925DB1D7E42BB68B896EF5BD928F20594AD2B8313AC8`.

Android DynEntSv_LinkModel uses memcpy for all three model min components into float4 plus an explicit zero fourth lane; it copies three completed world-min components back to the actual float[3] via memcpy. These preserve the original 8-byte pair and 4-byte third-lane bit copies without requiring an eight-byte-aligned float array. DynEntSv_LinkBrush copies the two computed float components represented in each 64-bit scalar to the respective min/max float arrays using memcpy of sizeof(the scalar). The third-component formulas, quaternion transformation, bounds ordering, destination dimensions and collision linking remain unchanged. Four native bytecopy sites; Windows originals retained in #else. No collision list, allocator, pointer representation or other functions changed. Existing brush input float assignments already use actual float arrays and needed no modification.

The source float arrays guarantee four-byte alignment, insufficient for the old QWORD accesses' eight-byte requirement. This is an allocation/type-contract repair: full420's already executed pre-UnitQuat model copy was not observed misaligned, and no claim is made that this would have been the next production failure.

Actual FULL ARM64 whole TU compile passed: `build-android-audit/native_registry_closure/dynentity-bounds-native1.log`; exact argv and compile source hash saved alongside. Normal Windows whole TU x86/x64 once passed: `build-android-audit/dynentity-bounds-windows1`, with source manifests. CP1252/CRLF byte-preserving edit and direct readback verified all four replacements and retained Windows expressions. Frozen manifest saved next to native evidence.

No additional fixture, production build, device run, assets transfer, definitions or source-list changes. Full421 runtime integration belongs to the root agent.
