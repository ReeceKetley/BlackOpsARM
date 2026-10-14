# Native destructible tag translation and matrix rows

This bounded batch complements the actual full425 G_CalcTagParentAxis first-row failure repaired by its separate g_utils owner. It does not claim that G_GetTagTransform, client destructibles, or Vec3RotateTranspose were executed by full425. Root owns the next full426 production integration and all engine execution.

Only src/physics/destructible.cpp changed. Android native64 now uses:

- G_GetTagTransform fallback: the three original indices9/10/11 become tagMat[3][0/1/2]. The actual sole server caller DamagePiece supplies stack float tagMat[4][3]. It consumes row3 for sound position. G_DObjGetWorldTagMatrix's successful branch already produces the same four-row contract; the fallback preserves AnglesToAxis, currentOrigin values, boneName and return behavior. Nearby client tagMat[3][3] owners call the distinct CG_GetTagTransform API with a separate float3 origin, not this server API.
- Vec3RotateTranspose: all nine reads use actual matrix[row][column], mapping original linear index n to n/3,n%3. Every cast, arithmetic grouping, expression/store order, const qualifier and in!=out assertion remains exact. CG_DestructibleShake owns actual axis[3][3], avel[3] and local_avel[3]; RB_Vec3DirWorldToView owns actual viewAxis[3][3], copied component-by-component from the view matrix before calling the helper. No independent transform implementation or flattened pointer was introduced.
- CG_DestructibleHitEvent: std::memcpy(hitp, bone_axis[3], sizeof(hitp)) replaces the original QWORD plus third-float assignment. Both owners are literal stack arrays: hitp[3] and bone_axis[4][3]. The exact12-byte span preserves all three float representations and reads/writes no fourth component. The translation row is at offset36 and needs only float alignment, which does not justify an eight-byte scalar access.

Normal Windows retains the original expressions in all three functions. There are no API/declaration, array-size, allocator, pointer layout, FastFile, network format, packing or assertion changes. Six other same-file QWORD operations belong to fixed network union byte stores/copies at original1530..1532/2847..2849 and remain untouched. Inventory found no further declared-row expressions in this source beyond these three repaired fallback stores and the nine repaired transpose reads. Existing explicit byte copies and typed axis3x3/4x3 accesses remain unchanged.

The prior Android baseline already contained BO1_DestructiblePrefetch and eight wrapper callsites. Its SHA256 is C80D8EA31B4EDAF41FCAFAD0D448A341A64A9C273E944FE859755F35BB14D513, captured verbatim in build-android-audit/native_destructible_tag_row/destructible-before.cpp. That baseline had3876 CRLF and10 bareLF line terminators, the latter within the existing prefetch helper insertion. This edit normalizes exactly those10 line terminators to CRLF without changing their tokens. Final source is CP1252/CRLF and passed exact encoded-byte readback.

Verification under build-android-audit/native_destructible_tag_row:

- Final actual Android ARM64/API28 whole TU compile exits0 with production flags and UBSan/non-recovery enabled: destructible-native.rsp and native-tu-final.log. Existing114 warnings retained. An initial native compile after only the fallback mapping predates the authorized bounded expansion; native-tu.log remains separate evidence.
- Final normal Windows x86 and x64 actual whole TUs each compile once, exit0: x86/build-result.json and x64/build-result.json, with exact command.txt and compile.log. No native-selection flag is used for these builds.
- windows-source-proof.json confirms that selecting final Windows branches is exactly the captured prior Android baseline after the10 newline-only normalizations. Restoring only the prior prefetch helper/callsite adaptation then yields byte-for-byte the read-only BlackOps64 source after common CRLF normalization. Thus the inherited Windows baseline and this change are distinguished; neither protected oracle files nor their index/branch were changed.
- No mirror fixture or repeated numerical suite was run. Earlier canonical com_math asymmetric matrix/canary evidence supports those actual shared mathematical owners, not execution of this newly mapped local Vec3RotateTranspose helper. The literal row mapping is established by the source/caller proof; production full426 supplies integration evidence separately.

Reproduce the native whole TU with:

```powershell
& D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe '@build-android-audit/native_destructible_tag_row/destructible-native.rsp'
```

Frozen destructible.cpp SHA256: DD201038FA81DECDE350F2AAE818DF25690D8302747624AE20DC0A6221AA93F0. This report is explicit UTF8, written to a temporary file and verified before publication. No engine/device test, asset/library transfer, central CMake/ledger mutation, commit, reset, clean or unrelated source edit occurred.
