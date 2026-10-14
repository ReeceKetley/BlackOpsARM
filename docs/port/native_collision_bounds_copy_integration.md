# Native collision bounds copy integration

Production full422 passed the real VM argument guard and map entity field parse, then stopped during trigger_use_touch/trigger_use_shared -> SV_SetBrushModel -> SV_LinkEntity -> CM_BoxLeafnums. UBSan identified a misaligned uint64 load from genuine entityShared.r.absmin float[3] at an address ending in b4. Four-byte alignment is valid for this owner, but the original QWORD expression requires eight. Matching full422 BuildId: 5618b88b8b0696069eb138e56ce36a2ed69e84e4. Root owns the production log and next full423 execution.

`src/qcommon/cm_test.cpp` now selects two exact twelve-byte std::memcpy operations for Android native64, into the actual float[2][3] leafList bounds rows. Float representation, row width, source reads, and destination layout stay unchanged. Normal Windows keeps the original QWORD/scalar expressions. No padding, packing, alignment promise, loader change, plane/traversal replacement, or assertion bypass is introduced.

The same-file bounded audit covered every function: point leaf traversal, box traversal/storage, point contents/brush traversal, transformed point contents and cluster PVS. Vector loops remain within three components; transformed axis accesses are already actual typed 3x3 rows; node, plane, leafbrush and brush pointers retain native-width owners. The only additional typed-copy hazard is CM_StoreLeafs' four-byte unsigned read at cLeaf_s.cluster, which is a signed16 field followed by two tail padding bytes. Its original four-byte meaning is retained through an object-representation byte pointer plus offsetof, with a compile-time proof that the four-byte span stays inside the actual leaf object. This deliberately preserves the all-FF comparison, including tail bytes; it does not replace it with signed16 cluster != -1, initialize padding, or alter FastFile data. CM_LeafCluster's separate signed16 API remains untouched.

All original decisions and traversal remain exact: child0 recursion before child1, negative child -> -1-child leaf number, representability assertion before uint16 publication, bounded count, overflow flag, and lastLeaf update even after overflow. No other canonical source or header was edited.

Verification:

- Final native whole TU compiled exit0 under the production ARM64/API28 flags, UBSan and non-recovering diagnostics. Evidence: build-android-audit/native_collision_bounds/native-tu-final2.log and cm-test-native.rsp. Existing cbrush declaration warning retained.
- Final normal Windows x86 and x64 whole TU builds exit0: build-android-audit/native_collision_bounds/windows-final-{x86,x64}/build-result.json and compile.log. The earlier Windows builds preceded the final Android-only guard/object-representation refinement and are retained as prior evidence.
- Selecting the Windows branches gives byte-for-byte the complete read-only BlackOps64 cm_test.cpp, including its unchanged original expressions and all function bodies: build-android-audit/native_collision_bounds/windows-source-proof.log. BlackOps64 was only read.
- Focused genuine ARM64/HONOR UBSan fixture exits0: build-android-audit/native_collision_bounds/honor-fixture.log, `PASS genuine collision bounds/traversal 356 checks; sources4mod8; actual math/node/leaf owners`. The fixture links the compiled actual cm_test object and previously frozen actual com_math object. Only assertion/diagnostic reporting are fixture seams; assertions hard-fail. Test-owned real cNode/cplane/cLeaf arrays and clipMap state supply geometry, not fake leaf/math/collision implementations.
- Both bounds are real guarded float[3] owners at addresses4mod8. Tests cover front/back/crossing axial planes, all eight nonaxial plane signbits, actual PointLeafnum traversal, original zero-width on-plane behavior, front-before-back ordered leaves, full/two/zero output capacity, direct overflow flag inspection, lastLeaf after overflow, all-FF cluster/tail exclusion and cluster16=-1 with zero tail inclusion. Source bounds, adjacent owners and unused output slots are unchanged. Actual canaries surround inputs, leafList and output list.

Reproduction from the Android worktree:

```powershell
& D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe '@build-android-audit/native_collision_bounds/cm-test-native.rsp'
& D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe '@build-android-audit/native_collision_bounds/fixture.rsp'
& D:/Android/Sdk/platform-tools/adb.exe -s AKWJ9X5116G04618 push build-android-audit/native_collision_bounds/collision-bounds-tests /data/local/tmp/blackopsarm-collision-bounds-tests
& D:/Android/Sdk/platform-tools/adb.exe -s AKWJ9X5116G04618 shell 'chmod 755 /data/local/tmp/blackopsarm-collision-bounds-tests && /data/local/tmp/blackopsarm-collision-bounds-tests'
```

Saved Windows command.txt files contain exact normal compile commands. Fixture rsp uses the frozen canonical com_math object at build-android-audit/native_registry_closure/com_math-rows-native.o; source SHA256 4CBF38F84A2E4C83F47F8EEC5835195BBF3D7503F67BFFEC5D4F10B17C01E6A9, object SHA256 3558929C41136F0CBF3E1D262F4720D6AF46F82A6F96991398F3D27698851C22. No math owner was rebuilt or edited by this batch.

Frozen source cm_test.cpp SHA256: 7E77C7D68E653F477AF62C1D1F674AA6B01858151E5C29739657071C8140154A. Fixture source SHA256: D2A8DADBEDD9FC58E11EC561DB5804912B995C8624B2AE31AC0C5E26AF7CE658. Native cm_test object SHA256: E1653E54C3BB1F4CF638C151BF08BBEE9633342401F9419416485DFE32E9721B. Sources retain CP1252/CRLF with exact encoded-byte readback; this report is explicit UTF8. The fixture binary alone was transferred; assets and libraries were reused. No production engine run, central CMake/ledger edit, commit, reset, clean or protected-tree write occurred.

This fixture proves the concrete bounds-copy and traversal risks, not initialized map/game/script frames or server/entity teardown. Root's genuine full423 production retry supplies the next integration evidence.
