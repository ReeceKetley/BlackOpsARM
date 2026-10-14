#include <cstdio>
#include <cstdlib>
#include <thread>
#include <vector>
#include <cstdint>
void check(bool result,const char *name) {
    if(!result){std::fprintf(stderr,"FAIL %s\n",name);std::exit(1);}
    std::printf("PASS %s\n",name);
}
int main() {
    alignas(4) volatile unsigned value=0;
    check(_InterlockedExchangeAdd(&value,0xe0000000u)==0 && value==0xe0000000u,"unsigned status bits preserve exact 32-bit addition");
    check(static_cast<unsigned>(_InterlockedExchangeAdd(&value,0x20000000u))==0xe0000000u && value==0,"32-bit modular carry and prior value");
    check(_InterlockedCompareExchange(&value,7u,1u)==0 && value==0,"failed compare-exchange returns observed bits");
    check(_InterlockedCompareExchange(&value,7u,0u)==0 && value==7,"successful compare-exchange publishes exact value");
    check(_InterlockedExchange(&value,0xffffffffu)==7 && value==0xffffffffu,"unsigned exchange preserves all high bits");
    check(_InterlockedIncrement(&value)==0 && value==0,"increment wraps at four-byte boundary");
    check(static_cast<unsigned>(_InterlockedDecrement(&value))==0xffffffffu,"decrement wraps at four-byte boundary");
    _InterlockedExchange(&value,0u);
    std::vector<std::thread> workers;
    for(int i=0;i<8;++i)workers.emplace_back([&] {for(int n=0;n<10000;++n)_InterlockedIncrement(&value);});
    for(auto &worker:workers)worker.join();
    check(value==80000,"eight real concurrent writers retain every update");
}
