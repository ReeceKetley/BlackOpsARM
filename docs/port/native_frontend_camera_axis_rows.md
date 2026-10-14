# Native rotated collision matrix rows and camera coordinate copies

SDK462/stage34, engine PID19980, BuildId19342a28dfd274116368b69985604a94686d3335 reaches CA_ACTIVE at88636ms, then reports cg_camera.cpp3140:39 index3 beyond const float[3]. This is an actual engine failure. The final log follows SetClientViewAngle(0,-60,0), but the matching crash identifies SERVER tid20125, RotatePoint→CM_TransformedBoxTraceRotated→CM_TransformedBoxTrace→CM_TransformedBoxTraceExternal→SV_EntityContact→Player_GetUseList→Player_UpdateCursorHints→ClientEndFrame→G_RunFrame. The source file name does not make this a camera-driver crash.

Raw evidence remains in build-android-audit/native_registry_closure/sdk-native-entry462-launch1-only.log and sdk-native-launch462-crash1.txt. The crash buffer contains older crashes too; the PID19980/BuildId19342a28… section is the matching failure.

## Ownership and bounded family

SV_EntityContact passes the real entity runtime origin and angles into the external transformed trace. CM_TransformedBoxTrace owns float matrix[3][3]; for nonzero angles, the canonical AnglesToAxis producer writes all nine typed coordinates, then passes the owned matrix to CM_TransformedBoxTraceRotated. That caller owns float start_l[3], end_l[3] and transpose[3][3]. RotatePoint rotates the first two vectors in place. If collision reduces results->fraction, TransposeMatrix fills the owned transpose and RotatePoint rotates the returned trace normal. CM_TransformedBoxSightTrace independently owns another matrix[3][3] and calls the same RotatePoint after the same producer. Producer, callers, signatures, layouts and normal-return condition stay unchanged. owner-producer-caller.txt preserves bounded excerpts.

RotatePoint now reads mat[0..2][0..2] on native64 non-Windows. All nine replacement offsets match the original flattened offsets: (row*3+column)*sizeof(float). Its three cached point components, assignments, explicit float casts, multiplication/addition grouping and order remain exact. No quaternion RotatePoint change.

The immediate writer/reader family in cm_trace is TransposeMatrix: native transpose[i][j]=matrix[j][i] replaces the flattened row indexing. The two original three-iteration loops and assignment order remain exact. This closes the same rotated-collision return path before the next APK build; actual execution of that conditional return path is still unclaimed.

The bounded cg_camera coordinate audit finds six eight-byte copies through _QWORD pointers: gunner viewPos from tagMtx row3; vehicle tagMtx row0 from bodyMtx row0; desiredCamPos from tagMtx row3; viewAxis row0 from barrelMtx row0; third-person newLookDir from axis row0; movie-camera plane from trace.normal. Their float3/float3-row owners guarantee float alignment, which can be four modulo eight. Native memcpy copies exactly two floats, preserving bits; the original following third-float statement remains outside each guard, unchanged. Typed vehicle matrices, coordinate loops, existing whole-axis memcpy calls and the nine-float gunner scratch/casts remain untouched.

## Verification and frozen sources

- Six whole-TU compiles pass: both sources with production NDK30 ARM64/API28 flags including O0/debug/instrumentation/UBSan-no-recovery, and MSVC Windows x86/x64.
- Sixteen preprocess runs pass. Thirteen selected native bodies prove only the coordinate/copy substitutions, including seven unchanged overload/caller bodies. Fifty-two Windows body comparisons against both immediate-before source and read-only BlackOps64 pass. Assertion-location normalization changes only compiler-generated file/line tokens; explicit assertions remain unchanged.
- Both source inverses are byte-exact. CP1252 high bytes, absent BOM, camera CRLF and cm_trace's17 pre-existing bare LF endings are preserved. Existing native cm_trace branch/prefetch guards and all earlier camera/ADS/killcam fixes remain intact; cg_view_mp is untouched.
- The source-extracted host fixture passes6656 checks:256 varied finite matrix/point cases give byte-identical rotation and transpose results against the Windows canonical bodies, every row/component address matches its original byte offset, and all six exact-copy wrappers preserve64 pairs of signed-zero/finite/infinity/NaN/subnormal/arbitrary-bit patterns, the untouched third float and source bytes. Owners are deliberately four modulo eight. This is a focused host proof, not Android gameplay execution.

Frozen cg_camera.cpp SHA256: `08C2A33FF74A760EEDB7B7405E20751E97F136D8D83F9141756DCD7929E51E85`.

Frozen cm_trace.cpp SHA256: `D07F2E1B0D3F4D8FB27F10FA3DB8F41F209321FA355FD108DE0D074A7BC1B159`.

Sources released to root for SDK463/stage35 after compile/body checks. No master build, device, provider, asset, config, Ledger or STATE mutation by this owner. Runtime verification belongs to the next actual APK observation; visible3D/playable Zombies remains unproved by these checks.

Evidence: build-android-audit/native_frontend_camera_axis_rows1/{source-manifest.json,check-results.json,coordinate-results.json,coordinate-run.log,cg_camera.diff,cm_trace.diff}, per-TU compiler commands/logs/preprocessed bodies, source backups and fixture source. check-results-attempt1.json preserves a verification-harness failure: macro expansion changed _QWORD to uint64 and the substitution initially touched the parameter declaration. The comparison harness was corrected, saved compiles reused under exact command equality, and all checks pass. Production sources were unchanged during that correction.
