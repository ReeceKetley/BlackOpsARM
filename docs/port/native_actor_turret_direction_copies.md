# Native actor and turret direction copies

This bounded follow-up repairs three nominal wide-copy alignment contracts in canonical `src/game/actor_senses.cpp` and `src/game/turret.cpp`. It follows the actual full425 tag-matrix row failure but does not claim that these three copy sites have executed or failed in production. Root's actual full426 run remains the integration authority.

All sources are real local float[4][3] tag matrices populated synchronously by `G_DObjGetWorldTagMatrix` on a successful tag lookup. The copied source is row0, a float[3] region with only float alignment required. Actor destinations are actual `ActorCachedInfo::dir[3]` float arrays (`game/actor.h`) in live actor eye/muzzle cache members. The turret destination is actual `weaponParms::gunForward[3]` (`bgame/bg_weapons.h`) in the live caller-owned weapon parameters. No source pointer is retained; no allocation, padding, lifetime or cache-publication change is introduced. No destination has a fourth lane.

Under `defined(__ANDROID__) && BO1_NATIVE_64BIT`, guarded `<cstring>` and three `std::memcpy` calls replace only the original wide load/store operations:

- `Actor_UpdateEyeInformation`: exactly8 bytes copy XY. Original `Vec2Normalize` then assigns dir[2]=0, unchanged. Copying12 here would expand the original operation unnecessarily, so the third component is not copied.
- `Actor_GetMuzzleInfo`: exactly12 bytes copy the three row0 components, replacing the original8+4-byte pair. Cache time/position writes, failure return and optional forward result flow are unchanged.
- `Turret_FillWeaponParms`: exactly12 bytes copy the three row0 components into gunForward, replacing the original8+4-byte pair. Activator/client branching, angle-vector construction, muzzle position and player-path calculations are unchanged.

Original Windows expressions remain verbatim in `#else` branches. A bounded inventory finds exactly two `_QWORD` copies in actor_senses and one in turret, with no other wide scalar memory-copy family or flattened first-row constant-offset accesses in these files. Existing turret copies of complete `col_context_t` objects are unrelated typed object copies and remain unchanged. All assertions, target/tag lookup, normalization, weapon parameters, masks, trace algorithms and callbacks remain unchanged. No unrelated casts, handles, shared declarations, CMake or runtime services change.

Verification under `build-android-audit/native_actor_turret_copies`:

- `scope.json`, both `.before` snapshots and `.diff` files: CP1252/CRLF exact readback, copy lengths8/12/12, and complete Windows source projection byte-for-byte equal to both before snapshots and read-only BlackOps64 oracle files.
- Per-source `*-native-command.json` and `*-native-tu.log`: exact production ARM64 whole-TU commands with UBSan/no recovery, outputs redirected only to evidence objects; both exit0 once.
- Normal Windows `x86` and `x64`: both whole TUs compile once per architecture, native selector absent, all four exit0. Commands, logs and build-result JSON retain existing warnings.
- `compile-results.json`: both final hashes unchanged throughout the six builds. `git diff --check` passes for both sources.

Before/oracle SHA256 values are `481A31B4F3BAAFD3C349F9FCD1F75C4A4D3973335EC862D2493D6088FEA215AA` for actor_senses and `B1DC9686C6585D3E89175E1EEC19D59EA0E87436CB117923D6E9B9A425B55DFA` for turret.

Frozen source SHA256:

- actor_senses.cpp: `C87D8C33623F3D8A549F19DF945C4198ED8E7B4EEC027E2ED859E593DAC71DD0`.
- turret.cpp: `B6AD12AD64B5738C9ADE2A7F2154AAF6021A2D6338CB8B436BE84B96F6DAAF3A`.

No synthetic fixture was added for pure byte-preserving copies. No engine/device run, asset/library transfer, Ledger mutation, commit or push was performed by this owner. Previous batches remain frozen. Reproduction helpers under `build-android-audit/native_registry_closure` are `repair_actor_turret_copies.py` and `build_actor_turret_copies.py`; the repair helper preserves original bytes and refuses an existing evidence directory. Report and JSON use UTF-8; canonical source remains CP1252/CRLF.
