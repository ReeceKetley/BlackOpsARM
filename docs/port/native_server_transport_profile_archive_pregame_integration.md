# Original server transport, profile, archive and pregame integration

2026-10-13. Genuine Com_Init/Com_Frame remains incomplete. No device/asset transfers.

Full database now selects six whole existing sources: sv_net_chan_mp,
sv_snapshot_profile_mp, sv_main_pc_mp, sv_live_stats, sv_bot_mp, sv_archive_mp.
Existing pregame.cpp full implementation is selected in full entry-state target
through BO1_NATIVE_PREGAME_IMPLEMENTATION; OFF retains prior diagnostic selection.
Real transmission/fragment bookkeeping, profiling, license/ranked policy, reserved
slots, bot, archive and pregame state algorithms connected. No fake online session,
license, readiness or successful packet send introduced.

Android sv_main_pc optional whitelist reader uses actual FILE rather than MS _iobuf,
checks fgets result before counting a password, bounds reads by actual pass array
and six-slot capacity, and closes the file. Original relative reservedslots.txt
policy retained; native application user-root placement remains an integration
concern. Windows reader unchanged; no claim of whitelist runtime verification.

Native archive adaptation in its canonical owner:

- SnapshotInfo is fully value-initialized, archived=true, avoiding original 12-byte
  clear/packed writes across native pointer/boolean layout.
- Entity state copies use typed assignment, client mask stays actual integer,
  flags/broadcast/bounds/origin/type reads use canonical fields.
- Visibility byte221 is clientLinkInfo.flags, verified by active static offsetof
  assertion. Visibility expression/operator precedence retained; no bmodel substitute.
- Baselines use typed svEntities[number].baseline; actual array/number assertions
  precede indexing. No old 360-byte pointer stride or narrowed native pointer.
- Last entity number is an int rather than float-bit alias/incorrect varargs input.

Original Windows branch retained. Fixed32 entity/network fields not widened, message
codec/zone/assertion gates remain. No independent archive algorithm, dummy writes,
allocator changes or assertion relaxation.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link305 records source selection/_iobuf
  failure; link306..307 intermediate adaptations; link308 compiles final source
  and fails genuine remaining full-engine links.
- server-transport-profile-archive-pregame-definitions1.log confirms real transmit,
  license/profile/bot/archive/whitelist owners and full pregame owners.
- takeover_server_transport_profile_archive_pregame_windows1: seven normal whole
  TUs pass x86/x64 exit0 with final source manifests.

No actual native socket transmission, initialized bot/session/whitelist, pregame
transition or archive delta/full encoding ran. Native archive field fixes are
compile/layout evidence, not complete codec/runtime proof; ring lifetime, caller
initialization, snapshot publication, compressed/save/network envelopes and actual
server state require execution once dependencies link. Live stats fixed data-slot
contracts are preserved but unexecuted. Full Com_Init/Com_Frame remains incomplete.
