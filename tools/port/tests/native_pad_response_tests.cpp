#include <platform/android/pad_response.h>
#include <platform/android/pad_menu.h>
#include <cassert>
#include <cmath>
#include <cstdio>

using namespace bo1::android::pad;
static bool Near(float a, float b) { return std::fabs(a - b) < 2e-6f; }

int main()
{
    float x, y;
    RightStick(0, 0, 0, 0.01f, x, y);
    assert(x == 0 && y == 0);
    RightStick(2000, -1000, 0.07f, 0.01f, x, y);
    assert(x == 0 && y == 0);
    RightStick(2400, 0, 0.07f, 0.01f, x, y);
    assert(x > 0 && x < 0.005f && y == 0);
    RightStick(-32768, 32767, 0.07f, 0.01f, x, y);
    assert(Near(x*x + y*y, 1) && x < 0 && y > 0);
    RightStick(32767, 0, 0.5f, 0.8f, x, y);
    assert(x == 1 && y == 0); // Overlapping deadzones must remain finite.
    for (int mode = 1; mode <= 2; ++mode) {
        float previous = -1;
        for (int i = 0; i <= 10000; ++i) {
            const float t = i / 10000.0f;
            x = t; y = 0;
            ShapeLook(x, y, mode);
            assert(x >= previous && x >= 0 && x <= 1 && y == 0);
            assert(x - previous < 0.001f || i == 0);
            if (mode == 1) assert(Near(x, t));
            previous = x;
            float dx = -0.6f*t, dy = 0.8f*t;
            ShapeLook(dx, dy, mode);
            assert(Near(dx, -0.6f*x) && Near(dy, 0.8f*x));
        }
        x = 1; y = -1;
        ShapeLook(x, y, mode);
        assert(Near(x*x + y*y, 1) && Near(x, -y));
    }
    assert(AdsScale(0, 0.5f) == 1);
    assert(AdsScale(0.5f, 0.5f) == 0.75f);
    assert(AdsScale(1, 0.5f) == 0.5f);
    for (int i = 0; i <= 100; ++i) assert(AdsScale(i/100.0f, 1) == 1);
    // ADS must soften small corrections without changing full-stick speed,
    // reversing direction, overshooting, or altering Linear response.
    for (int step = 0; step <= 10; ++step) {
        float previous = -1;
        for (int i = 0; i <= 10000; ++i) {
            const float t = i/10000.0f;
            float hip=t, zero=0;
            ShapeLook(hip, zero, 2);
            x=t; y=0;
            ShapeLook(x, y, 2, step/10.0f);
            assert(x >= previous && x >= 0 && x <= 1 && x <= hip + 2e-6f);
            previous=x;
            if (step == 10) assert(Near(x,t*t));
        }
        assert(previous == 1);
    }
    x=0.1f; y=0; ShapeLook(x,y,2,1);
    assert(Near(x,0.01f));
    x=0.1f; y=0; ShapeLook(x,y,1,1);
    assert(Near(x,0.1f));
    assert(MenuAxisEvent(0.49f,false,true,0.4f,0.1f)==-1);
    assert(MenuAxisEvent(0.51f,false,true,0.4f,0.1f)==1);
    assert(MenuAxisEvent(0.4f,true,true,0.4f,0.1f)==2);
    assert(MenuAxisEvent(0.29f,true,true,0.4f,0.1f)==0);
    assert(MenuAxisEvent(1,true,false,0.4f,0.1f)==0);
    assert(MenuAxisEvent(1,false,false,0.4f,0.1f)==-1);
    for (int i=0;i<4;++i) {
        assert(MenuKey(K_DPAD_UP+i)==K_UPARROW+i);
        assert(MenuKey(K_APAD_UP+i)==K_UPARROW+i);
        assert(IsMenuDirection(K_DPAD_UP+i) && IsMenuDirection(K_APAD_UP+i));
    }
    assert(MenuKey(K_BUTTON_A)==K_BUTTON_A && MenuKey(K_BUTTON_B)==K_BUTTON_B);
    assert(!IsMenuDirection(K_BUTTON_LSTICK));
    puts("PASS: deadzone boundaries, signed extremes, diagonal direction, bounded monotonic continuous curves, ADS transition and neutral multiplier");
    puts("PASS: ADS precision curve, Linear identity, menu hysteresis/release, direction translation and unchanged action keys");
}
