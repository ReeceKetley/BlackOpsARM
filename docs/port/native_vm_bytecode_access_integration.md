# Native VM packed operand access

Root's actual full416 stopped during compilation at an odd-address EmitShort store. This batch owns only `clientscript/cscr_vm.cpp`, the matching consumer side, coordinated with the separate compiler/debugger owners and root's frozen `platform/bo1_bytecode_io.h`.

Instruction streams and archived stack cells are packed bytes, not aligned scalar objects. Native fixed-width reads/stores now use the shared memcpy scalar helper. Windows uses the equivalent original typed loads/stores through local wrappers. Aligned VariableValue, native frame/stack structures and their real member accesses are unchanged. No padding, packing, changed bytecode format, skipped instruction or sanitizer suppression was introduced.

Coverage includes next-code-position/jump/switch scans; actual unsigned-short/int/float/code-position readers; direct VM opcode short operands; packed archive/notify/trim/unarchive cell payloads and raw header byte views. Header offsets remain codepos0/size4/bufLen6/localId8/time12; payload begins13 with five-byte type+four-byte-value cells. Native code addresses and vector/stack values remain existing four-byte Narrow32 handles. `Bo1_LoadCodePos` already had memcpy native access and remains valid. Unarchive native payload traversal now uses char pointers plus four rather than typed uint32 pointer arithmetic into unaligned cell storage.

Two adjacent concrete operand contracts were repaired:

- Native Scr_ReadUnsigned reads uint32 and advances four bytes. Its consumers are thread parameter counts and switch case values; the old uintptr_t load advanced eight on ARM64. Other platforms retain existing behavior. No native pointer is decoded through this scalar reader.
- OP_GetVector retains its twelve-byte, three-float instruction operand and exact cursor advance. Native execution loads each float bytewise into an actual Scr_AllocVector allocation and publishes that aligned engine vector. It does not return a pointer into unaligned bytecode, use temporary/TLS storage or add a second retain. Scr_AllocVector initializes the metadata word to zero and increments totalVectorRefCount for the original owning reference; AddRefToVector increments additional references on copies and RemoveRefToVector decrements/frees the last reference when the additional count is zero. Existing bytecode constant vectors use the preceding nonzero opcode marker and were immortal; the new owned vector follows the actual dynamic reference protocol. Numeric bit representations and compiler opcode bytes remain unchanged.

Evidence:

- `build-android-audit/native_registry_closure/cscr_vm-bytecode-native.log`: whole actual NDK TU exit0.
- `build-android-audit/takeover_vm_bytecode_windows1/{x86,x64}/build-result.json`: normal Windows TU both ABIs pass once.
- `vm-scalar-reader-tests.cpp/.rsp`, `vm-scalar-reader-tests-build.log`, `vm-scalar-reader-honor.log` in native_registry_closure: scoped UBSan fixture links the actual compiled VM object using section GC, without fake engine/providers. HONOR AKWJ9X5116G04618 passes offsets1..7 for actual Short/Unsigned/Int/Float readers, exact two/four-byte cursor increments, canaries, unsigned/signed0xfedcba98 bits and negative-zero/1.5/NaN payload representations. This executes scalar readers only; it does not claim opcode execution, archive/unarchive integration or vector retirement proof.

Reproduce the existing scoped fixture after rebuilding its VM object with the saved whole-TU response file:

```powershell
& D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe '@build-android-audit/native_registry_closure/cscr_vm-bytecode-native.rsp'
& D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe '@build-android-audit/native_registry_closure/vm-scalar-reader-tests.rsp'
& D:/Android/Sdk/platform-tools/adb.exe -s AKWJ9X5116G04618 push build-android-audit/native_registry_closure/vm-scalar-reader-tests /data/local/tmp/blackopsarm-vm-scalar-reader-tests
& D:/Android/Sdk/platform-tools/adb.exe -s AKWJ9X5116G04618 shell 'chmod 755 /data/local/tmp/blackopsarm-vm-scalar-reader-tests && /data/local/tmp/blackopsarm-vm-scalar-reader-tests'
```

Frozen cscr_vm.cpp SHA256: 6FEA10493E62E932030DE2201066173CD585801FEC12444460A3E862F3D4A60C. CP1252/original newline style retained. No production run, asset transfer, new CMake source or flags. Root's next full integration supplies real compiler/VM/script execution evidence.
