# Original sound-bank and command source integration

2026-10-13. Full Com_Init remains incomplete; no audio playback claim.

Selected original sound bank/db/globals/utils/log/dvar/public-async/queue and BO1
trace source owners. Actual bank/patch add/remove, table lookup, command dispatch
and diagnostic behavior now compile from their canonical engine implementations.

SND_FIND_ROW uses typed adapters rather than calling incompatible function-pointer
casts. The six existing typed lookup functions retain their inputs/results. Three
reference entries deliberately call RETURN_ZERO32; the adapter invokes that same
accepted reference function and converts its result, rather than inventing a new
successful service or changing table policy. Unsupported table kinds remain
absent exactly as the working Windows reference defines them.

Sound tracing's existing exclusive SRW lock uses actual native mutex ownership
on Android; diagnostics use native snprintf/vsnprintf. Windows retains SRW APIs.
This protects trace bookkeeping; it does not create an audio device or mark the
sound engine initialized.

## Evidence

full-engine-link80/81 compile original sound modules and expose actual remaining
sound-core dependencies. Link82 includes the real trace owner and still fails
on production cinematic, core-sound/driver, client, physics, input/network and
recovery services. Nine whole Windows TUs pass x86/x64 in
takeover_sound_bank_windows1 (eight) and takeover_sound_trace_windows1 (one).
No new tablet runtime probe or transfer; prior tests are not repeated.

Runtime bank/patch publication, metadata lookup, command queue, mixer/device
initialization and playback remain unverified. Keep original startup/critical
section order and fail on missing behavior. No new unavailable endpoints added.

## Cinematic platform boundary

Read-only source inspection confirms r_cinematic.cpp uses DirectSound and Bink,
including DirectSound creation/cooperative mode and Bink sound-system context.
Its original headless branch avoids opening a sound device. It has not been
selected/ported in this step; ForceRelinquishIO remains a real unresolved owner.
An Android implementation must preserve genuine state/IO lifecycle and provide
a compatible decoder/audio path when playback is requested. Windows DLLs and
32-bit pointer casts cannot be assumed valid native ARM implementations.
DXVK/Vulkan remains unchanged; these are separate codec/audio/platform contracts.

Continue from link82 with real sound core/device and cinematic IO owners, plus
the remaining retained physics/client/platform graph. Do not claim readiness
from zero-initialized globals or successful source compilation.
