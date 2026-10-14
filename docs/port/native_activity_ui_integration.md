# Genuine native Activity/UI adapters

2026-10-13. New src/platform/android/ui_platform.cpp/.h and new Java
android/app/src/main/java/com/blackopsarm/dev/NativeUi.java only. Existing Windows,
Activity/bootstrap, controller, input/console, metadata and protected sources are
unchanged. Root owns integration of the frozen header/output/input hooks.

Select ui_platform.cpp using existing real SDL2/native services linkage and
headers. No extra success provider, HWND, Activity or capability setter is added.
Actual owners: IN_SetCursorPos, Sys_GetClipboardData, Sys_ShowConsole, Sys_OpenURL,
Sys_UpdateHotkeyBlock, plus bo1::android::ConfirmConfigurationChange and the frozen
console/URL adapter API. Actual exported/undefined manifests and SHA256/bytes are
in build-android-audit/native_registry_closure/ui-platform-native-{defined,
undefined}.txt and native-ui-frozen-hashes.json. Header stayed frozen after its
initial publication; no existing consumed header was edited.

## Real dependencies/actions

Before any unsafe SDL Android JNI getter, the service requires the actual matching
SDLJavaSetupComplete publication. The actual JNI environment, VM, Activity type
and finishing/destroyed state are checked. App NativeUi resolves through the real
Activity's classloader; attached native-thread FindClass is used only for bootstrap
Java/Android classes. Native console callbacks RegisterNatives against that real
class, with exact JNI method descriptors. JNI exceptions owned by the operation
are described/cleared and local frames retire before engine logging/allocation.
Existing pending JNI exceptions cause refusal rather than being swallowed.
Native text uses actual UTF-8 bytes and Java String byte-array conversion.

URL uses actual ACTION_VIEW and requires actual startActivity acknowledgement;
missing handler, Activity/dispatch failure or cancellation returns false. Sys_OpenURL
queues canonical quit text only after acknowledged launch when doexit is set.
Headless never launches. The accepted Windows reference's Sys_OpenURL is #if0;
root explicitly authorized this actual native intended-body functionality. Windows
remains unchanged. No remote website/Steam/service readiness is inferred from
startActivity returning.

Confirmation uses a real Yes/No AlertDialog. True means actual positive user click
and completed dismissal. No, cancel, destruction, unavailable Activity/UI or wrong
waiting thread means false. It refuses blocking on the Android UI looper, where
waiting would prevent the dialog from running. Workers dispatch show on the UI
looper and wait for actual dismissal; there is no invented positive response.
Absent JNI/Activity context emits native refusal warning. Root metadata algorithms
may use this frozen bool API, with their canonical headless false guard.

Clipboard uses the real Activity ClipboardManager and primary clip's coerceToText.
Missing/private/unreadable data returns null; empty actual text can remain empty.
UTF-8 is capped at1MiB; failures do not manufacture an empty clipboard payload.
JNI copy/local frames retire before the real tag11 Z_Malloc copy. The returned
payload belongs to the canonical caller, which frees tag11. The original ignored
strtok-return first-token policy for newline/CR/backspace is preserved, including
leading separators, using thread-safe strtok_r. A normal null allocation result
frees the intermediate owned heap copy and returns null. Headless lacks the native
Activity clipboard and returns null; no Windows desktop clipboard is synthesized.
Initialized nonlocal allocator-error recovery is not proven by this component.

Cursor requests run on the actual installed SDL owner through renderer_window.
They require actual Java/video, tracked live window, nonclosing drawable and actual
SDL mouse focus. Native pixel coordinates convert through actual drawable/logical
sizes; out-of-window positions refuse. Actual SDL_WarpMouseInWindow is followed by
actual SDL_GetMouseState position verification. This is per-window logical SDL
cursor behavior, never a claim of Android/global desktop cursor movement. Headless
never warps. Actual rendered input translator/video/bootstrap remain prerequisites.

Android does not provide the Windows global system-hotkey hook. Sys_UpdateHotkeyBlock
observes actual SDL window focus/closing state and canonical fullscreen/client
state; when the original policy requests global interception it reports actual
platform inability once. It never publishes a fabricated global block or claims
system Home/navigation interception. The Windows hook behavior remains unchanged.

## Actual console and exact integration hooks

AppendAndroidConsoleOutput retains the actual last128KiB of Platform_Print output.
Root should call it from the real output provider; it has no logging/JNI/core
callbacks while its own output mutex is held. Java Console is a real Activity
AlertDialog showing those bytes, refreshed through registered JNI, with command
entry and close controls. Lifecycle destruction retires observer/dialog/refresh
callbacks. No fake console HWND or output is invented.

InstallAndroidConsoleCommandOwner must be called on the actual SDL event owner
AFTER genuine rendered input and engine event-queue initialization. This registers
an actual bounded producer/consumer; it does not stand in for input initialization.
PumpAndroidConsoleCommandsIfOwner belongs in that actual Platform_InputFrame.
RetireAndroidConsoleCommandOwner belongs before input/SDL owner retirement.
Headless continues its existing real stdin/tagged queue and never shows this GUI.

The actual GUI command producer remains disabled until this installation. Java
shows unavailable owner and disables Send/input in that state. Registered JNI
submission validates1..1023 UTF-8 bytes, forbids embedded NUL/line-control framing,
and admits at most32 pending lines. It refuses overflow rather than dropping an
accepted line. Only its actual input owner drains to fresh tag11 payloads and
canonical Sys_QueEvent(SE_CONSOLE), outside the producer mutex. Accepted means
queued, not executed; Java labels that distinction. Retirement discards pending
lines and rejects new submission. No new locks survive across engine allocator,
event, logging or error callbacks. Native explicit fatal callbacks abort if they
unexpectedly return. Library/native callbacks must outlive an active Java console;
app bootstrap/library retirement remains an actual integration dependency.

Java UI dispatch uses a per-request phase/monitor and actual latch acknowledgement.
A timed-out pending action is cancelled before it can run later. A started short
UI operation is observed to completion rather than reporting a false timeout then
launching/showing after return. Confirmation waits never run on the UI looper.
This has not certified active-render modal waits/presentation/recovery.

## Proportionate evidence

- ui-platform-native-final1.log/command.json: actual full Android toolchain,
  production instrumentation and UBSan source compilation PASS.
- native-ui-javac2.log (argv retained as native-ui-javac1-command.json): real
  Android API35 jar, Java release17, Xlint all, clean compilation PASS.
- native-ui-java-signatures.txt: actual compiled method descriptors, including
  the three registered private native callbacks.
- native-ui-fixture-build1.log/command.json and run1.log: HONOR
  AKWJ9X5116G04618, UBSan with no recovery PASS.

Durable tools/port/native_ui_fixture.cpp consumes the actual adapter source only
for its private bounded helpers, links actual renderer/window/controller service
sources and the existing matching SDL2 library. Isolated headless-policy,
logger/zone/event/quit providers expose CPU contracts, not engine readiness.
Actual SDLJavaSetupComplete remains false in this shell. Unsafe JNI getters are
wrapped to abort if touched; actual service refusals do not touch them.

Proof: headless and absent-Java context produce no clipboard, Yes, console launch,
URL quit or cursor request; no phantom clipboard allocation or Cbuf quit. Real
output bytes/tail capacity, canonical tag11 clipboard first-token/leading-separator
policy and normal allocation-failure intermediate-copy free. Actual SDL event
owner plus bounded command queue:32 FIFO payloads, overflow/invalid framing reject,
allocator reentry adds two in order, four producers add80 once (114 total), only
owner drains, retirement cancels pending and denies late admission. Three fatal
children exercise duplicate installation, wrong-thread retirement and allocator
failure; reentrant error callbacks take the same service locks without deadlock,
then SIGABRT prevents fake continuation. No actual VM/Activity/GPU/window/input
readiness is mocked or certified.

Only the new2,140,600-byte fixture binary was transferred; existing SDL2 was reused:

    adb -s AKWJ9X5116G04618 shell 'LD_LIBRARY_PATH=/data/local/tmp/blackopsarm-native-engine-entry47-1 /data/local/tmp/native_ui_fixture'

Native audit1's genuine compile failures (wrong pointer cast and canonical client
header dependency) remain preserved. Positive APK dialogs/Yes/No/Activity destruction,
clipboard privacy, real intent launches/handlers, console display/submission, mouse
focus/warp and global-platform capability observations remain UNEXECUTED. Current
APK still lacks real SDL setup and rendered input is incomplete. No APK install,
bootstrap rewrite, asset/library transfer or false UI/Com_Init readiness occurred.
