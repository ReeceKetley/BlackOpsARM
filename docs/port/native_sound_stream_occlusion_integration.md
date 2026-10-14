# Sound streaming and native occlusion ownership

2026-10-13. Full Com_Init remains incomplete.

Selected original snd_stream/snd_debug. Native string comparison, pointer-width
window-return atomics and file error declarations use existing real services.
Full builds exclude inherited unavailable Snd_StreamUpdate where the original
stream owner is now selected. No replacement buffers, reads or successful status.

Native occlusion worker owns the original initially-unsignaled auto-reset event,
real role11/TLS startup, callback, retirement/join and reuse only after completed
join. Canonical event set/wait and quit set/query bodies moved outside the CPU-
query guard, preserving one state owner. Thread admission adds only role11 and
its proved-joined reset. Native sound Fini wakes the worker and joins retirement
before clearing trace/sound state; it does not infer quiescence from a delay.

Source audit: original Sys_QueryWin32QuitEvent returns true when signaled; native
render owner creates it initially false. SND_LosOcclusionThreadMain originally
loops while query is true, so Android now continues while it is false. Windows
branch remains unchanged. This fixes the source-level shutdown-condition inversion;
the actual sound callback has not run, so full occlusion algorithm behavior is
not runtime certified. Real quit-event owner must initialize before callback use.

## Evidence

- HONOR UBSan finite callback verifies real role11/TLS, initial/consumed event,
  payload publication, callback retirement, join and two reuse cycles.
  occlusion-worker-build2.log/run1.log in native_registry_closure.
- The focused executable compiles actual thread/event/hunk/worker and canonical
  TLS/cmd/threads sources with existing selector flags. No diagnostic endpoints
  or substitute engine implementations. It does not execute SND_Init or the
  original occlusion loop. Original full-library build1 failure preserved; its
  unrelated unresolved game graph is not bypassed in the production entry.
- Original Windows stream/debug and sound-occlusion/thread TUs pass x86/x64 in
  sound_stream_windows1 and sound_occlusion_windows2. Final quit-body relocation
  passes threads on both ABIs in occlusion_quit_windows1. Initial occlusion
  Windows1 missing forward declaration preserved and corrected.
- Full-engine-link88..92 retain compile/duplicate/intermediate failures. Latest92
  compiles and fails linking actual remaining cinematic/device, game/client/
  script/physics, renderer/input/network/recovery owners.
- Only the small worker test binary transferred; assets and DXVK/Vulkan unchanged.

Original streaming IO, active audio/occlusion callback, actual sound shutdown,
quit sequencing and full startup remain unverified. The finite worker gate proves
the platform ownership mechanism only. Continue real dependency closure from92.
