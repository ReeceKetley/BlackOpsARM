# Native plain snapshot value reader masks

Full426 completed actual G_InitGame/SV_SpawnServer (markers frame0/script0) then stopped at a negative writer shift during first snapshot work. This reader repair complements the separately owned writer. No completed game-frame claim follows those initialization markers.

Concrete paired reader issue: MSG_ReadValue with absbits31 computes signed (1<<31)-1. The shift can produce INTMIN under C++20, but subtracting1 overflows. Native MSG_ReadValue now uses uint32 masks with explicit full32 UINTMAX and zero-width0, uint32 XOR of the canonical signed MSG_GetField baseline bits, and sign extension ONLY when original bits is negative. The final ordinary int result is bit_cast from the exact32bits. A plain signed width range check[-32,32] runs before negation or message reads; INTMIN and specialized negative codes cannot reach width arithmetic. Existing dispatchers retain all specialized-code algorithms. An invalid-width reporting path cannot continue after fatal because abort follows a returning reporter.

MSG_ReadValueNoXor and MSG_GetField remain byte-identical to the captured prechange bodies. In particular no high-byte <<24 or char/short signedness changes were made. Fixed wire slots and Narrow32 representation unchanged. All changes/includes Android-only; original Windows MSG_ReadValue body matches read-only BlackOps64 oracle ignoring whitespace. Source CP1252/CRLF.

Validation: replayed actual production compile command to isolated msg.native.o, native-whole1.log exit0. Normal Windows wholeTU x86/x64 both exit0. body-comparison.json preserves scope checks; original prechange source and exact command retained. The writer owner receives this exact frozen object for ONE actual canonical writer/reader wire fixture with +/-31/+/-32 and signed high-bit cases; no duplicate fixture/test was added or run by this owner. Runtime fixture evidence belongs to native_snapshot_value_mask and must be reported by that owner. No shared full build, engine device run or ledger edit.

Frozen msg.cpp SHA256: aa0f0bc1095f850ded614c1babb71517ab851c62a9b9ae718eba7708f88a3dbf

Evidence: build-android-audit/native_snapshot_value_reader/native-whole1.log, native-command.json, msg.native.o, windows-x86.log/windows-x64.log, body-comparison.json, msg.cpp.before, source-sha256.txt.
