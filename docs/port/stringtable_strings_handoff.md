# Initialized StringTable strings checkpoint —12 October2026

TASK-0020 passes HONOR and Windows x86/x64:129 cases/12 genuine cycles each,
including optimized ARM with assertions/UBSan. Actual raw loading initializes
398/1545 inline strings; the actual converter writes2/49 owned cell fields after
earlier-start/NUL/owner/bounds/exact-map checks. Source ends9607/62490 before
indices. Header/index tag and TEMP lifetime, provenance invalidation and logical
cleanup are verified. All tests ended, failures/scratch preserved, no push.

Read `arm64_stringtable_strings.md` for commands/evidence/limits and
`arm64_stringtable_strings_changes.md` for every pending path. Git/STATE identify
the later verified checkpoint. The source bodies and protected Windows/DXVK/root/
older evidence remain unchanged. No new latency/RSS claim, APK or graphics.

Next TASK-0021 acceptance: actual fixed2-byte index reads/commits,400/1594 entries
from source9607/62490 to10407/65678, unchanged original order/values, disk alignment2
and native8, all word/byte maps, signed range/permutation, invalid/truncated/bounds
fixtures and repeated detach/release on3 ABIs. Stop before lookup/registration.

Important revised plan: genuine indices are valid permutations but are not
ordinary signed/unsigned sorted hashes. Preserve them; sorting is diagnostic,
not a reason to reject or rewrite original assets. The source lookup model
predicts24/400 and796/1594 misses but no native lookup ran. Next isolate the actual
overflow-safe Windows-equivalent hash, then run actual lookup/reference tests
before any behavior change. Pure initialization guards trust caller liveness/
proof creation; suffix/otherblock offsets and full engine/graphics remain open.
Continue the authorized objective under DEC-0029/0026; WARN-0020.
