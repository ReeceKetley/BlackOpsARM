# Native NetField scalar access and copy-back

Actual full427 reached archived entity serialization and failed MSG_ValuesAreEqual in the separate writer owner on a four-byte scalar address2mod4. Source/metadata proof confirms this is legitimate: entityState_s is224 bytes, archivedEntity_s256, and clientLinkInfo is a four-byte composite at offset218..221 immediately before animtreeIndex222. Its actual members are parentEnt16/tagIndex8/flags8 and it uses26 transmitted bits. This is the unique unaligned4 entity/archive field among1049 literal rows inspected by the layout owner. No layout, metadata, padding, alignment assertion or wire-width adjustment is justified by the failure.

This batch owns only two bodies in src/qcommon/msg.cpp. Android native64 MSG_GetField now reads exact sizeof1/2/4 bytes into signed char, int16_t or int32_t local objects through std::memcpy, returning the same signed/int-extended value. Windows keeps the original char/int16/int loads. The original invalid-size assertion/debugbreak and fallback0 remain common and unchanged. Field bytes are object representation, including the packed composite span; no typed scalar dereference or wider read is required.

Android native64 MSG_CopyFieldOver keeps the original pointer/offset calculation, GetField call, metadata size read and switch order. It stores exact1/2/4 bytes from uint8_t/uint16_t/int32_t temporaries. Unsigned narrowing preserves the original low8/16 bits of the signed GetField result; four-byte storage preserves all original bits. Original Windows store expressions remain intact. The complete source load occurs before any destination store, preserving the original alias/ordering behavior. Invalid size still reaches the original GetField assertion/fallback and performs no store. No fourth byte is touched by a1/2-byte field, and the real offset218 four-byte span ends at221 without modifying the following field.

The field pointers remain synchronously borrowed views into current/baseline/archive state, or the existing local zero baselines. No helper retains a pointer, allocates state, mutates NetField definitions or changes snapshot lifetimes. MSG_ReadValue's toF parameter is unused: this function returns its decoded int and only obtains the baseline value through MSG_GetField. Its prior masks, sign extension, plain-width guards and control flow were deliberately left byte-for-byte unchanged. Other reader stores and all sv_msg_write_mp base-index/header operations remain untouched; those base-header accesses are not field-offset dereferences and the actual typed entity/archive base owners are aligned separately.

Verification under build-android-audit/native_msg_field_access:

- Final native ARM64/API28 actual whole TU compiled once, exit0 under production flags and UBSan/non-recovery: msg-native.rsp and native-tu-final.log. Existing32 warnings retained.
- Normal Windows x86/x64 actual whole TUs each compiled once at final source, exit0: {x86,x64}/build-result.json, command.txt and compile.log. Native selection is false.
- source-proof.json establishes that selecting only these new Windows branches yields the entire prior msg.cpp byte-for-byte. The two selected Windows function bodies also each match read-only BlackOps64 byte-for-byte. This comparison does not erase inherited Android width/rounding adaptations elsewhere in the file.
- The full MSG_ReadValue body remains byte-identical to the captured prior source; body SHA2567AE313CEDFACAA7B982BB06310BD68FA653AA9F2F8B6CC46558CD652F228687F. Prior whole source SHA256AA0F0BC1095F850DED614C1BABB71517AB851C62A9B9AE718EBA7708F88A3DBF is retained verbatim in msg-before.cpp.
- No duplicate fixture or runtime run was added here. The writer owner received this actual compiled msg.o for one combined genuine packed-field fixture including GetField/copy-back, odd/two-mod-four offsets, source-tail boundaries and actual clientLinkInfo26-bit composition. That owner's separate evidence records the combined execution result. These compilation/source proofs alone do not claim fixture success, completed archival, or server frames after the repair; root's production full428 supplies integration evidence.

Reproduce the actual native TU:

```powershell
& D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe '@build-android-audit/native_msg_field_access/msg-native.rsp'
```

Frozen msg.cpp SHA256: 4A3D8096B68651D048F8531F7917BAF3CEB2C42A86D1700A07094E01ECA66CE5. Companion actual native msg.o SHA256: AAC8F730FAB25A19119D2007FF4CEC459F1D4B83EEDB1BE69A24846020A89B4D. CP1252/CRLF preserved with exact encoded-byte readback; original baseline had no bareLF lines and none were normalized. This report is explicit UTF8, temporary-write/readback verified before publication. No other canonical file/header, CMake/ledger, production/device/asset/library state or protected reference was modified by this batch; no commit/reset/clean.
