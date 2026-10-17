#pragma once
namespace bo1::android {
// Selected once by SDL argv before any engine worker starts.
inline bool autoStartKino{};
// Launcher start (--map=<name>): map loaded once the frontend is active, after
// the launcher's settings (<user root>/launcher.cfg) are applied. Empty: none.
inline char autoStartMap[64]{};
enum class NativeEngineEntry { DedicatedConsole, SDLActivity };
// Single real boot/frame owner for the CLI and matching SDK SDL entry.
// SDL argv: SDK program ("app_process"), existing absolute game/user roots,
// --rendered. Returns only for rejected prerequisites; real quit is terminal.
int RunNativeEngine(NativeEngineEntry entry, int argc, char **argv);
}
