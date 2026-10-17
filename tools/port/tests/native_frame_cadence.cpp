#include "gfx_vulkan/frame_cadence.h"
#include <cstdio>
#include <cstdlib>
using bo1::vulkan::FrameCadence;
unsigned checks;void Check(bool value){++checks;if(!value)std::abort();}
int main(){
    FrameCadence cadence;std::uint64_t time=1000000000;
    Check(!cadence.Observe(time));
    for(unsigned i=1;i<25;++i)Check(!cadence.Observe(time+=40000000));
    auto window=cadence.Observe(time+=40000000);Check(bool(window));
    Check(window->frames==25&&window->seconds==1&&window->fps==25);
    Check(window->p50==40&&window->p95==40&&window->p99==40&&window->maximum==40);
    for(unsigned i=0;i<20;++i)Check(!cadence.Observe(time+=40000000));
    window=cadence.Observe(time+=200000000);Check(bool(window));
    Check(window->frames==21&&window->fps==21&&window->p50==40&&window->p95==40&&window->p99==200&&window->maximum==200);
    Check(!cadence.Observe(time-1));time-=1;Check(!cadence.Observe(time));
    window=cadence.Observe(time+=2000000000);Check(bool(window)&&window->frames==1&&window->fps==0.5&&window->maximum==2000);
    FrameCadence bounded;time=1;Check(!bounded.Observe(time));
    for(unsigned i=0;i<1023;++i)Check(!bounded.Observe(time+=1000));
    window=bounded.Observe(time+=1000);Check(bool(window)&&window->frames==1024);
    std::printf("native_frame_cadence=PASS checks=%u every_interval=1 slow_frame=1 reset=1 bounded=1\n",checks);
}
