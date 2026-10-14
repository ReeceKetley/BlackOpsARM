# Native script packed field indices

Full418 progressed through genuine script postload into Scr_AddFieldsForRadiant -> FastFile -> Scr_AddFieldsForFile; UBSan stopped at the uint16 index write after a variable-length name, at a valid odd address. Actual field records are name+NUL, canonical uint16 index, byte type; the final byte is an empty-name sentinel. The index is a scalar, not a Narrow32 pointer.

Native cscr_variable.cpp now Store<uint16_t> at the existing writer position and Load<uint16_t> at the matching sole consumer Scr_FindField. Exactly2 access changes, frozen shared bytecode memcpy helper included only Android. Writer allocation len+4, old-sentinel rewind, byte copy/name/type/end marker and reader len+3 advancement are unchanged. No alignment padding, record format or parser/VM algorithm change. Original Windows statements remain in else branches.

Semantic audit traced every fieldBuffer use and producer/consumer rather than only cast forms: both loadobj/fastfile producers initialize the sentinel, both call the same AddFieldsForFile body; client/game/pathnode/vehicle callers consume only the index/type from Scr_FindField. No other packed scalar access was found in this field-buffer boundary. The buffer belongs to the retained program HunkUser; temporary source/high-hunk retirement is separate. Existing reference stale fieldBuffer after program-hunk destruction was noted by the read-only auditor; no observed postshutdown read or unrelated lifecycle change is claimed.

Native wholeTU and normal Windows wholeTU x86/x64 exit0. Two affected Windows function bodies match read-only oracle ignoring whitespace. Focused HONOR UBSan honor-run1 exit0:16 actual consumer/scalar/canary observations. Actual canonical Scr_FindField and I_stricmp traverse isolated literal records with odd index addresses, mixed lengths, high-bit/max indices, casefold, types, missing/empty lookup, terminal marker and canaries. Shared production scalar helper update is byte-exact and consumed by the actual reader. This does not claim actual parser/SL/DB execution in the fixture; real AddFieldsForFile production integration remains root full419 proof. No copied parser or fake asset/variable providers.

Evidence: build-android-audit/native_script_field_buffer/native-whole1.log, windows-x86.log/windows-x64.log, windows-oracle-bodies.json, fixture-build1.log, honor-run1.log, source-sha256.txt. Reproduce with tools/port/tests/build-native-script-field-buffer.ps1; fixture source native_script_field_buffer_tests.cpp.

Frozen SHA256: afe15e5591e5eb883057176cb08efc5f7f65e09f14548a4c9cd7195a894b341a

