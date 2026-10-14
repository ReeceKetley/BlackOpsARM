# Native canonical matrix row access

Root's production full420 reached DynEntSv_LinkModel during G_InitGame and failed UnitQuatToAxis's first-row access `(*axis)[3]`. The caller owns a real float[3][3], but dereferencing float(*)[3] yields only its first three-float row. Contiguous object bytes do not make that larger row-array subscript valid.

`src/universal/com_math.cpp` now uses a translation-unit-local typed accessor deducing the actual row width and returning the correct const/lvalue reference at rows[index/Columns][index%Columns]. Index is evaluated once. Widths2/3/4 cover real knot, matrix, plane and bounds owners. Windows's macro retains the original first-row expression. Quaternion formulas/signs, casts, arithmetic/evaluation order, stores, dimensions and memory sizes remain unchanged. No flat float pointer, packing, padding or assertion weakening was added.

The equivalent family spans34 actual functions: graph knots, box/sphere planes;3x3/4x3/4x4 identity/multiply/transposes/inverses/transforms; unit quaternion/axis conversions; projections/view matrices; rotated bounds; and color matrices. The initial mechanical inventory mapped779 typed-row expressions, excluding15 legitimate accesses through IntersectPlanes's independent float** plane array. Six GetRotatedBounds byte-offset selections were additionally mapped to actual2x3 bound rows: offsets are exactly0 or12 bytes, hence row choice0/1 with preserved component. Five of those selections contained initial flattened-row expressions, so those nested sites were folded into the complete bound-selection expressions rather than double-counted. MatrixInverse44's final i=0..15 normalization loop required two more actual typed-row accesses despite different `(dst)[0][i]` syntax; Windows retains its exact original loop statement. Final source contains776 typed-row scalar-expression sites and6 bound-selection sites:782 mapped scalar accesses across34 functions.

Dynamic indices remain original bounded loops: knots use2*knotIndex and adjacent components for valid canonical knot counts; planes use4*planeIndex+component for planeIndex<numPlanes; rotated bounds use axisIndex0..2 and row offsets0/12; inverse44 uses i0..15. Other changed matrix scalar accesses use canonical constant indices0..8/11/15. Existing operand/alias/assertion preconditions remain in place.

Verification:

- Actual NDK whole source compiles exit0: `build-android-audit/native_registry_closure/com_math-rows-native.log`.
- Normal Windows x86/x64 passes against root's final inline header: `build-android-audit/takeover_math_matrix_rows_windows2/{x86,x64}/build-result.json`. The earlier windows1 pass predates that header refinement.
- `math-matrix-windows-source-proof.log` restores the canonical Windows expression spelling and selects its inverse-loop branch, then compares comment/whitespace-normalized body tokens against read-only BlackOps64:34 changed functions match, zero mismatches. This is a source equivalence proof, not an oracle executable run. Root separately verifies its two inline header bodies against the same reference.
- `tools/port/tests/native_math_matrix_row_tests.cpp` links the actual compiled com_math object, with only hard-failing assertion and real-format diagnostic seams. No matrix/rotation/provider algorithm is substituted. HONOR UBSan final `math-matrix-row-honor.log` passes ten quaternion sign cases (identity and known axes/general rotation), canonical forward agreement,3x3/4x3/4x4 multiply/transposes/inverses, independent asymmetric/noncommuting expected matrices, two-plane inside/outside decisions, mixed positive/negative-axis rotated bounds,3x2 knot interpolation and output canaries. Direct header transforms of asymmetricA*{2,-1,3} yield{17,21,2}; transpose yields{9,11,4}.

The fixture exposed two additional actual misses, retained separately:

1. `math-matrix-row-honor-failed-header1.log`: MatrixInverseOrthogonal43 reached inline MatrixTransformVector in com_math.h, which still flattened its first row. Root repaired18 reads across that helper and MatrixTransposeTransformVector using exact native row/column indexing; its header is separately owned/frozen.
2. `math-matrix-row-honor-failed-inverse44-2.log`: MatrixInverse44 final `(dst)[0][i]` loop failed at i4. This owned source loop was repaired without dropping that test.

These are fixture failures, distinct from production full420's UnitQuatToAxis failure. Final success does not certify every projection/color API, actual DynEntity placement, engine initialization or Zombies frames. Root's next production421 run supplies integration evidence.

Reproduce the final real-function fixture with saved response files:

```powershell
& D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe '@build-android-audit/native_registry_closure/com_math-rows-native.rsp'
& D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe '@build-android-audit/native_registry_closure/math-matrix-row-tests.rsp'
& D:/Android/Sdk/platform-tools/adb.exe -s AKWJ9X5116G04618 push build-android-audit/native_registry_closure/math-matrix-row-tests /data/local/tmp/blackopsarm-math-matrix-row-tests
& D:/Android/Sdk/platform-tools/adb.exe -s AKWJ9X5116G04618 shell 'chmod 755 /data/local/tmp/blackopsarm-math-matrix-row-tests && /data/local/tmp/blackopsarm-math-matrix-row-tests'
```

Frozen com_math.cpp SHA256:4CBF38F84A2E4C83F47F8EEC5835195BBF3D7503F67BFFEC5D4F10B17C01E6A9. Root's shared com_math.h frozen SHA256:521641AA4899497A15339C5E56274682EE35B899CDBC77EEEEB63A853D5472D1. CP1252/CRLF preserved. No central CMake/ledger edits, asset transfers or production run by this batch. Existing source integration only.
