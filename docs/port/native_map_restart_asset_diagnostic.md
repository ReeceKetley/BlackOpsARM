# Native map restart asset ownership diagnostic

Full428 completes real Kino initialization, 1,300 server/script frames and mapped
shutdown. A separate same-process `map_restart` run completes 500 frames on its
first map, then exits 139 while marking dependencies after map-zone default
replacement. The second game initialization does not complete.

The retained matching ELF has BuildId
`2cb28886800560924e4d8ad488dd8fdc49c50f8a`. Its symbolized stack is retained in
`build-android-audit/native_registry_closure/full428-restart-symbolized-stack.txt`.
The actual Android crash reports memcpy's source as `0x5f6d735f6c5f636d`, eight
ASCII bytes being used as a pointer. The canonical path is
`DB_GetXAssetHeaderName` → `DB_GetXAsset` → `Mark_XModelAsset` →
`Mark_XModelPtrArray(16)` → `Mark_WeaponDef`'s `worldModel` traversal →
`DB_FreeUnusedResources`. `DB_DDLGetName` copies exactly the native-width payload
and name pointers; changing that getter would conceal the invalid model reference.

The actual native `Load_WeaponDef` uses `DB_ExpandLoad_WeaponDef` for its 2,056-byte
disk record. Inline model arrays use `DB_ExpandLoad_Ptr32Array` and widen four-byte
disk slots to native slots. Non-inline gun/world array offsets use
`DB_ConvertOffsetToPointer` and `DB_StreamMapLookup`. These owners and the original
16-element contract match the read-only Windows reference. The crash stack alone
cannot prove whether offset mapping was already wrong when loading an unused
weapon or a later operation overwrote a valid reference.

The next production candidate therefore contains a bounded diagnostic in the two
actual owners, `src/database/db_load.cpp` and `src/database/db_registry.cpp`.
Immediately after canonical gun/world array loading it checks model slot values
against exact XModel pool entry addresses. It logs discrepancies, the original
baked array offset, current stream segments and actual native zone/block span.
Before each requested zone unload and after replacement it checks registered
weapon arrays again and prints a boundary count. There is no frame-loop scan.

These checks only observe ownership. Pool membership during loading is diagnostic
information, not a readiness or validity gate: in-flight/unlinked models may
legitimately precede canonical publication. External spans are reported without
the diagnostic reading their contents. Canonical loading, marking, default
replacement, assertions and failures remain unchanged; the original crash may
still occur. This candidate does not claim a repair or successful restart.

The final frozen source manifest is
`build-android-audit/native_registry_closure/asset-recovery-diagnostic1/source-manifest-final.json`:

- `db_registry.cpp`: `EAE342F5B1A0B63452857F719EA4082548556E0EE71E5EE421B3869EDED599B1`
- `db_load.cpp`: `B77B99FC04B9608F38D39BA11AE3D0EE3A5384759E70D60A4EF8EDC7AC738AFF`

Both final whole translation units compile using actual production NDK30/API28
commands, with debug, PIC, UBSan/no recovery and engine function instrumentation
retained. Isolated outputs are under the evidence directory's `final` folder;
the full428 ELF was not overwritten. Normal Windows x86/x64 comparisons also
pass for both translation units with the established `/permissive /DWIN32=1`
configuration. All diagnostic additions are Android-only. The initial command
parser failure and missing diagnostic include failure are preserved, followed by
the corrected final checks. No additional fixture duplicates the engine logic.

Root owns candidate429 linking and one real targeted restart run. The next action
is to compare the earliest `ASSET_RECOVERY_MODEL`/`ARRAY` records with
`ASSET_RECOVERY_BOUNDARY`, then repair the proven producer or retirement contract.
No assets, libraries, protected reference files or tablet state were changed by
this diagnostic batch.

The subsequent full429 run established that this corruption already exists
during initial `common_zombie` loading. The resulting representation repair and
its bounded actual-object fixture are documented in
[native_weapon_shared_zero_pointer_array.md](native_weapon_shared_zero_pointer_array.md).
Full430 production restart validation remains pending at this report update.
