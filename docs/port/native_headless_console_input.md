# Real native headless console producer

2026-10-13. Full-engine-link194 compiles input integration but still fails genuine
remaining links. Actual Com_Init/Com_Frame/command processing unexecuted.

console_input owns a duplicated CLOEXEC input descriptor on the installation
thread. Reading uses zero-timeout poll and one-byte reads without changing caller
file flags. Installation requires exclusive stream-reading ownership; descriptor
duplication alone cannot serialize another reader. Engine native entry installs
its STDIN after real main-thread registration, before Com_Init. Bounded4096-byte
per-frame work prevents a continuously writing stream starving frame execution.

Complete lines become actual SE_CONSOLE events with native zone-tag11 allocations,
correct length and terminator. Partial lines remain pending; CRLF emits once;
NUL/oversized lines are explicitly discarded through the next terminator. EOF
flushes a remaining valid partial line once and closes the owned descriptor.
Retirement releases descriptor/buffer state on the owner. No fabricated console
commands or successful read substituted for failures.

Platform_InputFrame's actual headless branch polls that producer. Rendered branch
pumps only the real installed SDL event owner, failing explicitly when absent;
SDL window/consumer installation and keyboard/mouse/touch/controller translation
remain required. This does not certify complete rendered input or provide fake
controller capability. Canonical event queue remains its single original owner.

Evidence:

-194 compiles production console service and real entry installation; reaches
  genuine audio/cinematic/client/render/game linker dependencies. Platform_InputFrame
  now resolves for this service; startup remains unexecuted.
- console-input-build1 compiles actual producer, canonical Win_main queue/native
  headless policy, native critical sections/zone heap/clock/quit/SDL service with
  API28 ARM64 UBSan minimal runtime. Overflow logging fixture and unexpected
  terminal quit failure endpoint are test-only; no engine initialization simulated.
- console-input-run1 on HONOR uses actual OS pipe and Platform_InputFrame. Verifies
  real headless policy, unchanged caller descriptor flags, retained partial input,
  correct owned command payloads, CRLF exact-once delivery, oversized/NUL line
  rejection, EOF partial flush once and allocation baseline after consumer frees.
  Does not run Com_EventLoop, command execution or whole game shutdown.

Only new test binary transferred; existing SDL/runtime/assets reused. Shared
Windows sources unchanged since their passing event/wait whole-source checks;
new input service is Android-specific. Continue SDL input translation/owner/window
installation and actual remaining194 dependencies; execute authentic startup once
the full target links. Avoid empty input polling presented as platform completion.
