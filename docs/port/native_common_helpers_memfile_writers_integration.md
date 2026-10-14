# Existing common helpers and memory-file writers

2026-10-13. Actual Com_Init/Com_Frame remains incomplete; no tablet execution or asset transfers.

Canonical common.cpp now selects original Field_Clear, Com_AddToString,
Com_GetMaxActorCorpses, Com_GetFirstFreeEntityNum, Com_GetDecimalDelimiter and
Com_LocalizedFloatToString for native boot. Canonical common/stringed headers
provide their types, constants and language state. Original bodies and Windows
behavior are retained, including caller-supplied string bounds, locale policy and
entity-number selection. No synthetic initialization or independent engine copies.

Whole existing game_mp/g_save_mp.cpp is selected by the full database target.
Android MemFile_WriteShort uses memcpy of sizeof(short), preserving its two-byte
little-endian encoding while removing an alignment requirement at odd cache
positions. Windows retains the original store. Original flush boundaries and
cache accounting are unchanged.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link286.log compiles common and g_save_mp.
- full-engine-link287.log compiles final locale selection; full link still fails
  remaining genuine engine/platform dependencies.
- common-helper-memfile-definitions1.log confirms eight real archive definitions.
- takeover_common_helpers_memfile_writers_windows1: two whole normal TUs pass
  x86 and x64, exit0, with source manifests after final changes.

No initialized console/locale/entity/memory-file lifecycle executed. Locale caller
currently passes a 1024-byte buffer and precision2; the original general API still
requires nonzero capacity, valid buffer and initialized loc_language. Cache-flush
compression/decompression and save-image ABI limitations remain as previously
recorded. Compile/archive checks do not prove runtime startup or save correctness.

Info_Print remains unselected: its original unchecked 512/516-byte local copies
need bounds handling before exposing the native owner. Other remaining dependencies
include actual error recovery, input, audio/cinematic and original source owners.
