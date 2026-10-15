# Hybrid startup font capacity

Stage55, 2026-10-15. Full native backend goal incomplete.

SDK570 overflow dump contains valid, distinct zone registrations: six shared font names each retained from SP/MP localized code zones, dev-font variants, then clanFont from common_mp. Read-only BlackOps64 reference also defines16 slots; it was not edited. Hybrid native startup intentionally loads both code-zone sets; overriding fonts must remain available for canonical zone removal, so dropping duplicate names would change ownership semantics.

Added bounded32-event registration metadata diagnostic without scanning assets. SDK571/stage140 proves15 valid startup registrations,16th fonts/clanFont height36/glyphs96,17th actual common_mp fonts/bigDevFont height24/glyphs96, then16-slot exhaustion. No malformed pointer/count/font payload found. A transient default replacement was considered during inspection but the failing registration is a real override; it is not the basis of the fix.

Android capacity is now24, allowing the additional SP startup set alongside the existing16-slot budget. Windows remains16. Hard bound and normal allocation failure remain; no asset is skipped or fabricated. Compiled ARM64 ELF verifies g_poolSize[ASSET_TYPE_FONT]=24 and g_FontPool968bytes (8-byte header +24*40-byte native entries). Extra pool storage320bytes; glyph/texture data and fixed disk/VM formats unchanged. Legacy non-Android/native-guard projection28494tokens matches checkpoint; shared font behavior outside Android unchanged.

SDK572/stage141 engine840737e09419c5569bfcb4678fc2de5da3a3e0e7 PID28364 passes real17th font registration and completes common_mp. No16-slot overflow. It then crashesSIGSEGV in Dvar_FindVar via GetDvarStringValue/EvaluateExpression/menu scripts at bad address01343698. Matched tombstone confirms actual native crash reason5/status11; not an OOM or observation timeout. Shadow colour-technique failure remains the primary renderer feature gap; error/recovery UI now advances beyond the previous freeze. Full teardown correctness remains unproved.

Both ARM64 builds/exact package checks pass. SDK571 terminalfontfatal exit2, SDK572 actual nativecrash confirmed. Evidence native-font-stage55-build1/2.log, launch1/2.log, follow1/2/3.log, pool-proof.json, legacy-proof.json, exit-info.txt, crash572.txt; native_registry_closure/sdk571-font-final-entry.log and sdk572-font-final-entry.log. Next fix runtime expression pointer preservation, then implement real depth shadow/comparison path and full scene/lifecycle/performance. Assets/reference repositories unchanged.
