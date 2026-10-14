#pragma once

#include <universal/dvar.h>
#include <client/cl_gamepad.h>

#if defined(__ANDROID__) || defined(BO1_NATIVE_GAMEPAD_HOST_TEST)
#include <platform/android/controller_service.h>
// Canonical algorithms consume a transport sample; Android has no XInput ABI.
using _XINPUT_GAMEPAD = bo1::android::ControllerSample;
struct _XINPUT_STATE { _XINPUT_GAMEPAD Gamepad; };
#else
#include <XInput.h>
#endif

struct GamePad // sizeof=0x7C
{                                       // XREF: .data:s_gamePads/r
    bool enabled;
    bool keyboardEnabled;
    __int16 digitals;
    __int16 lastDigitals;
    // padding byte
    // padding byte
    float analogs[2];
    float lastAnalogs[2];
    float sticks[4];
    float lastSticks[4];
    bool stickDown[4][2];
    bool stickDownLast[4][2];
    float lowRumble;
    float highRumble;
    //GamePad::<unnamed_type_feedback> feedback;
    struct //GamePad::<unnamed_type_feedback> // sizeof=0x4
    {                                       // XREF: GamePad/r
#if defined(__ANDROID__) || defined(BO1_NATIVE_GAMEPAD_HOST_TEST)
        struct { unsigned short wLeftMotorSpeed, wRightMotorSpeed; } Rumble;
#else
        _XINPUT_VIBRATION Rumble;
#endif
    } feedback;
#if !defined(__ANDROID__) && !defined(BO1_NATIVE_GAMEPAD_HOST_TEST)
    _XINPUT_CAPABILITIES caps;
    _XINPUT_CAPABILITIES keyboardCaps;
#endif
};

#if defined(__ANDROID__) || defined(BO1_NATIVE_GAMEPAD_HOST_TEST)
static_assert(sizeof(GamePad) == 84);
static_assert(bo1::android::ControllerSlots == 1);
#endif

void GPad_InitAll();
#if defined(__ANDROID__) || defined(BO1_NATIVE_GAMEPAD_HOST_TEST)
void GPad_ShutdownAll();
#endif
void __cdecl GPad_UpdateAll();
int GPad_RefreshAll();
void __cdecl GPad_Check(int portIndex);
void __cdecl GPad_UpdateDigitals(int portIndex, const _XINPUT_GAMEPAD *xpad);
void __cdecl GPad_UpdateAnalogs(int portIndex, const _XINPUT_GAMEPAD *xpad);
void __cdecl GPad_UpdateSticks(int portIndex, const _XINPUT_GAMEPAD *xpad);
void __cdecl GPad_ConvertStickToFloat(__int16 inX, __int16 inY, float *outX, float *outY);
void __cdecl GPad_UpdateSticksDown(GamePad *gPad);
void __cdecl GPad_ResetState(int portIndex);
bool __cdecl GPad_ButtonRequiresUpdates(int portIndex, GamePadButton button);
bool __cdecl GPad_IsButtonReleased(int portIndex, GamePadButton button);
bool __cdecl GPad_IsStickPressed(int portIndex, GamePadStick stick, GamePadStickDir stickDir);
bool __cdecl GPad_GetStickChangedToPressedState(
                int portIndex,
                GamePadStick stick,
                GamePadStickDir stickDir,
                bool pressedState);
bool __cdecl GPad_IsStickReleased(int portIndex, GamePadStick stick, GamePadStickDir stickDir);
