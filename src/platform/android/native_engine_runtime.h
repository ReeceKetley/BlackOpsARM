#pragma once
namespace bo1::android {
enum class NativeEngineEntry { DedicatedConsole, SDLActivity };
// Single real boot/frame owner for the CLI and matching SDK SDL entry.
// SDL argv: SDK program ("app_process"), existing absolute game/user roots,
// --rendered. Returns only for rejected prerequisites; real quit is terminal.
int RunNativeEngine(NativeEngineEntry entry, int argc, char **argv);
}
