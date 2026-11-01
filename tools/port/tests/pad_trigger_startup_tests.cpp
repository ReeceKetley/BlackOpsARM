#include "platform/android/pad_trigger_startup.h"
#include <cassert>
#include <cstdio>
int main() {
    bo1::android::TriggerStartup left, right;
    assert(left.Filter(-32768,0)==0); // SDL synthetic release must not arm.
    for(int frame=0;frame<100;++frame) {
        assert(left.Filter(0,16383)==0); // stick movement reports untouched triggers
        assert(right.Filter(0,16383)==0);
    }
    assert(left.Filter(-20000,6383)==6383); // first real partial press preserved
    assert(right.Filter(0,16383)==0); // independent trigger state
    assert(left.Filter(0,16383)==16383); // midpoint is valid after movement
    assert(left.Filter(32767,32767)==32767);
    assert(left.Filter(-32767,0)==0); // release
    assert(right.Filter(-32767,0)==0); // released first hardware sample
    assert(right.Filter(0,16383)==16383);
    left={}; // detach/reconnect resets startup
    assert(left.Filter(-32768,0)==0 && left.Filter(0,16383)==0);
    assert(left.Filter(32767,32767)==32767); // fully held at attachment
    puts("PASS trigger startup placeholders, independent axes, first press, full range, release and reconnect");
}
