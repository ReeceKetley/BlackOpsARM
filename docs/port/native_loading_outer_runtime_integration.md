# Original loading keepalive and native outer runtime

Original Sys_LoadingKeepAlive is selected in the full native system core. It
calls the actual Monkey keepalive owner and drains the actual shared event queue.
Native discarded event payloads are retired with the same tag11 allocator used
by the queue/caller contract; the Windows body is preserved. FULL diagnostic
removed only where this genuine owner replaces it; OFF historical diagnostic
retains its original isolation selection.

Native outer entry formerly called one frame and returned, leaving workers
alive at process teardown. It now polls actual Platform_InputFrame and calls
original Com_Frame continuously only after actual Com_Init returns. Real quit
owns teardown; an unexpected returned terminal quit aborts rather than claiming
clean shutdown. The initialization-only watchdog defaults300s (configurable
BO1_ENGINE_WATCHDOG_SECONDS 1..3600); it observes actual initializer return and
cannot kill continuing gameplay after that return. No fake engine state/readiness
is set. Main input producer remains the genuine preinstalled console/SDL owner.

Whole native main ARM64 compilation passes with exact production flags. Normal
Windows twelve whole TUs (gameplay/effects/table dependencies + win_main) both
ABIs pass. Latest linker382 selected these owners but still lacks genuine
remaining dependencies. Link383 was caught while Steam agent files were mutable;
it is not accepted frozen-source evidence and remains preserved.

No full engine link or Com_Init return, initialized loading keepalive/Monkey,
continuous frame/gameplay or terminal recovery/shutdown runtime proof. Native
Sys_Init/input/platform binding is now integrated through actual native services.
The combined scoped system fixture passes, including the corrected headless
gamepad restart lifecycle; its filesystem provider is isolated from production.
See native_system_core_initialization_integration.md. Full-engine-link389 compiles
these owners and still fails genuine linker closure; no full startup is claimed.
