#pragma once
#include <ui/keycodes.h>

namespace bo1::android::pad {
inline bool IsMenuDirection(int key)
{
    return (key >= K_DPAD_UP && key <= K_DPAD_RIGHT) ||
           (key >= K_APAD_UP && key <= K_APAD_RIGHT);
}

// PC menu assets consume arrow keys, while the input owner retains pad keys.
inline int MenuKey(int key)
{
    switch (key) {
    case K_DPAD_UP: case K_APAD_UP: return K_UPARROW;
    case K_DPAD_DOWN: case K_APAD_DOWN: return K_DOWNARROW;
    case K_DPAD_LEFT: case K_APAD_LEFT: return K_LEFTARROW;
    case K_DPAD_RIGHT: case K_APAD_RIGHT: return K_RIGHTARROW;
    default: return key;
    }
}

// -1 no event, 0 release, 1 initial press, 2 held repeat. Use separate press
// and release thresholds so drift around the boundary cannot chatter.
inline int MenuAxisEvent(float directedValue, bool wasDown, bool menuActive,
                         float threshold, float hysteresis)
{
    const bool down = menuActive && directedValue > threshold + (wasDown ? -hysteresis : hysteresis);
    return down ? (wasDown ? 2 : 1) : (wasDown ? 0 : -1);
}
}
