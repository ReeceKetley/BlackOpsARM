# Original UI registration and native menu-token ownership

2026-10-13. Full-engine-link139 compiles selected sources but still fails to link.
Actual Com_Init/Com_Frame and complete UI/menu initialization remain incomplete.

Selected eight complete original cg_consolecmds_mp, ui_main_pc, ui_shared_obj,
ui_feeders, l_precomp, l_memory, ui_main_mp and ui_gametype_variants_mp. These
supply actual client UI-flag query, active-menu and command handling, raw menu
parser/allocator, feeders and gametype policy owners. No substitute source or
new diagnostic endpoint was added. Dvars/menu/client readiness remain dependent
on real startup state, not source selection.

Existing native CRT formatter is used by preprocessor/parser. Recent-player
FILETIME is a scoped fixed8-byte/two32-bit format with a per-header alias to the
actual Windows SDK type there; native metadata layout is size8/alignment4.
No native timestamp storage/OS service is claimed from that declaration.

Actual date/time macro code frees its ctime result. Windows _ctime64 returns
static CRT storage; that reference corner remains unchanged and is not claimed
runtime-validated. Android explicitly obtains an independently heap-owned copy
from real localtime/asctime services. Year range is checked before26-byte text
formatting; parser failures release its copied token and publish no successful
macro. The owned helper is not advertised as a full MSVC _ctime64 replacement.

Original l_memory's4-byte prefix misaligns ARM64 token_s, including native long
double/pointers. Native prefix uses max_align_t alignment, retaining the same
cookie immediately before user data and the original tagged Z_Malloc/Z_Free
ownership. Overflow into the signed32 allocator size is rejected. Windows prefix
and free path remain unchanged. PC_CopyToken now clears native typed next instead
of a hard-coded32-bit word index that can overwrite another field. Disk/VM widths
and Windows load algorithms were not changed; CP1252 source bytes preserved.

HONOR UBSan menu-time-build1/run1 executes actual production PC_CopyToken,
PC_FreeToken, PC_ExpandBuiltinDefine, l_memory, native zone allocator and owned
time helper. It verifies real UTC epoch/next-day format in test-local timezone,
independent/freeable time strings, unrepresentable-year failure, allocation
alignment/tagged cleanup/size overflow, copied line/whitespace preservation/next,
original quoted date/time macro tokens and all token/zone allocations released.
It uses the existing fail-on-use assertion/error endpoints; a production assertion
or error would fail, never pass the fixture. No menu/file parsing or full UI init
claim. Only this test binary was pushed; game assets were reused.

Nine whole normal Windows TUs pass x86/x64 in takeover_ui_registration_windows1,
including original live_meetplayer as a header/layout comparison. Logs137..139
preserve missing CRT/time/timestamp dependencies and final link evidence. Other
packed parser fields and full preprocessing/loading/recovery still require actual
execution before broader native readiness claims.

Next actual global hunk/LargeLocal, screen/event/input, audio/cinematic, remaining
client/UI/script/game/renderer/network owners must be integrated from link139.
Do not replace real ownership/allocation/device services with ready flags. Existing
scoped passing probes need no repetition absent related changes. Full production
startup, frame execution and asset loading remain the objective.
