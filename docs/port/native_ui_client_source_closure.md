# Original UI and client-platform source closure

2026-10-13. Full-engine-link136 compiles selected sources but still fails linking;
actual Com_Init/Com_Frame remain incomplete.

Selected complete original cl_main_pc_mp, ui_main, ui_shared, ui_localvars, ui_utils
and ui_atoms. Canonical UI state/accessors/context updates/animation, drawing and
client platform-command registration are retained. No substitute engine bodies,
new unsupported endpoints or forced UI/input/audio/Steam ready values were added.

SD_TruncateAudioDeviceNames now has one declaration in the existing neutral sound
driver API, consumed by UI rather than pulling XAudio2 SDK implementation types.
The actual native audio-device service remains missing. gpad_enabled declaration
moves to the backend-neutral client gamepad header; Windows keeps actual storage
and registration. UI explicitly includes that canonical header and excludes its
unused XInput implementation header on Android. No XInput SDK structures were
faked; actual Android gamepad registration/storage/input is still needed.

Three signed32 gamepad sentinel enums use -1, retaining0xffffffff bits. UI's static
const server-filter table has explicit zero initialization, preserving original
static-zero value semantics accepted by MSVC. Native UI integer formatting uses
the existing real CRT service. CP1252 ui_shared encoding is preserved.

Per-site platform guard audit: consulted arm64_phase1/flag_guard_audit.json (legacy
layout guards); this added WIN32-or-Android guard is an OS policy boundary, not a
native layout/BO1_PORT_X64 change. CL_CDKeyValidate keeps the exact reference policy:
if actual g_steamInitialized is false, retain original offline/ticketless result;
otherwise invoke Steam_UpdateClientAuthTicket. No initialized flag, ticket or
online authentication was invented; enabled Steam remains dependent on its real
service. No global WIN32 or BO1_PORT_X64 define was enabled on Android.

All six native modules compile. Eight whole normal Windows TUs pass x86/x64 in
takeover_ui_client_windows1, including actual XAudio2 and XInput implementations
affected by neutral declarations. No UI/audio/input/network runtime result is
claimed. Logs133..136 preserve SDK/enum/Steam guard, const initialization/CRT/
declaration corrections and final link failure. No probes or device/asset transfers.

Next canonical cg_consolecmds_mp owns the still-missing CL_LocalClient_IsCUIFlagSet
query, reading real clientUIActives flags with original bounds assertion; select
its full source rather than copying the query. Remaining actual UI/frontend/client,
gamer-profile, hunk/LargeLocal/recovery, script parser/compiler/builtins, native
audio/cinematic/game/renderer/network services must be integrated. More retained
dependencies after full source selection are expected and are not a completion
metric. Source compilation does not prove real startup registration or gameplay.
