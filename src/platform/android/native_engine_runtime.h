#pragma once
namespace bo1::android {
// Selected once by SDL argv before any engine worker starts.
inline bool autoStartKino{};
enum class NativeEngineEntry { DedicatedConsole, SDLActivity };
// Single real boot/frame owner for the CLI and matching SDK SDL entry.
// SDL argv: SDK program ("app_process"), existing absolute game/user roots,
// --rendered. Returns only for rejected prerequisites; real quit is terminal.
int RunNativeEngine(NativeEngineEntry entry, int argc, char **argv);
}
