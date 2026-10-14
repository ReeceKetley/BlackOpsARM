# Native packed compiler operands

Full416 reached actual retail script compilation; EmitShort then stored a uint16 at a legitimate odd bytecode address. Real TempMallocAlignStrict uses Hunk_UserAlloc alignment1: instructions own packed bytes.

Native cscr_compiler.cpp now uses the shared frozen bytecode memcpy helpers for18 two/four-byte emission or backpatch writes and3 packed operand reads. Cursor advancement and operand lengths are unchanged; EmitCodepos retains BO1_NARROW_ENCODE and four-byte slots. Switch records remain eight bytes; an exact constvoid* qsort callback loads unsigned keys and preserves descending order and paired handles. No padding or sanitizer suppression.

Two insertion shifts indexed beyond localVarsInitBits[8] into the following real eight-byte scr_localVar_t rows. At j>i>=0 those addresses denote localVars[j-1]; native typed row assignment preserves the original shift/count/name/sourcePos algorithm and mask. Original Windows branches remain.

Native wholeTU compile2 and normal Windows wholeTU x86/x64 pass. Nine affected original Windows bodies were compared with the read-only oracle ignoring whitespace; results in windows-oracle-bodies.json. HONOR UBSan fixture honor-run2 exit0:22 actual emission/backpatch/switch/row observations, with canonical temporary hunk allocation and Narrow32 provider. It checks exact odd-address short/uint/float bytes, unchanged cursor, actual4-byte/null handle, actual break/continue patching, actual comparator paired-record sorting, both row shifts and canaries. Isolated real borrowed hunk and failstop diagnostic providers; no retail parsing/full-init claim. Root owns integration.

Evidence directory: build-android-audit/native_compiler_bytecode (native-whole2.log, windows-x86.log, windows-x64.log, windows-oracle-bodies.json, fixture-build3.log, honor-run2.log, source-sha256.txt). Fixture source/build: tools/port/tests/native_compiler_bytecode_tests.cpp and build-native-compiler-bytecode.ps1.

Frozen SHA256: 66fafa4f6bbe2e673063788b58fef7c59e8d9fbbdfeb82aea01aa6a137102ef6

## Full417 linked-call followup

Full417 passed original emissions, then LinkThread dereferenced a decoded unsigned-int pointer at an odd packed call operand. Native LinkThread now calls two narrowly scoped actual packed-operand helpers: decode the operand Narrow32 handle as a byte address, Load<uint32_t> for the sentinel and Store<uint32_t> for the actual code-handle bits. The original condition short-circuit, unknown-function/developer branches, variable removal and four-byte patch representation remain unchanged. Windows LinkThread body matches the read-only oracle ignoring whitespace.

Semantic audit traced every compiler BO1_NARROW_DECODE use, temporary-code cursor and locally typed pointer dereference, rather than only cast syntax. The two LinkThread accesses were the remaining decoded scalar accesses; other decoded addresses feed diagnostics. Multi-byte direct emit/backpatch/switch-key accesses use the prior packed helpers. Opcode pointers dereference one byte; local variable rows and parse-tree nodes are typed real containers, whereas int* parameters hold ordinary counter/type outputs. VariableValue/VariableValueInternal fields are typed variable-pool storage, not packed call operands. Audit inventory: indirect-access-audit.txt. No further indirect packed operand access was found in this compiler TU; other consumers have separate owners.

Final native wholeTU native-whole3.log and normal Windows windows3-x86.log/windows3-x64.log pass. Focused HONOR UBSan honor-linked-run1.log exit0 proves5 actual producer+codec+LinkThread-helper observations: an odd operand produced by canonical EmitByte/EmitUInt32 with real temporary hunk; sentinel1 read; actual code handle patch/decode; high-bit patch exact bytes with neighboring opcode canaries; unchanged6-byte cursor and trailing canary. This calls the actual helpers used by LinkThread, not synthetic variable-pool implementations. Full LinkThread variable bookkeeping remains a production integration check. Prior22 broader fixture cases were not rerun. Build log fixture-build4.log; run binary with argument linked.

Final frozen SHA256: e5c3c5c80d5192841d490a967ac5468b6da401c95028cbe3207d8719efc6d937
