#include <physics/phys_local.h>
#include <qcommon/bitarray.h>
#include <thread>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cfenv>
#if defined(__ANDROID__)
#include <platform/android/snap_float.h>
#else
#include <xmmintrin.h>
#endif
bool Assert_MyHandler(const char *file,int line,int,const char *,...) {
    std::fprintf(stderr,"ASSERT %s:%d\n",file,line); std::abort();
}
void check(bool value,const char *name) {
    if(!value){std::fprintf(stderr,"FAIL %s\n",name);std::exit(1);}
    std::printf("PASS %s\n",name);
}
struct Node { Node *m_next_link; unsigned id; };
int main(){
    const std::uint32_t inputs[]={0u,0x80000000u,0x3f000000u,0xbf000000u,
        0x3fc00000u,0xbfc00000u,0x40200000u,0xc0200000u,0x4effffffu,
        0x4f000000u,0xcf000000u,0xcf000001u,0x7f800000u,0xff800000u,
        0x7fc00000u,0x7f800001u,1u,0x80000001u};
    for(int mode=0;mode<4;++mode){
#if defined(__ANDROID__)
        const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
        if(fesetround(modes[mode]))std::abort();
#else
        _mm_setcsr((_mm_getcsr()&~0x6000u)|(static_cast<unsigned>(mode)<<13));
#endif
        for(auto bits:inputs){
            float value;std::memcpy(&value,&bits,4);
#if defined(__ANDROID__)
            feclearexcept(FE_ALL_EXCEPT);
            int result=bo1::android::SnapFloatToInt32(value);
            int invalid=!!fetestexcept(FE_INVALID),inexact=!!fetestexcept(FE_INEXACT);
#else
            _mm_setcsr(_mm_getcsr()&~0x3fu);
            int result=_mm_cvtss_si32(_mm_set_ss(value));
            int invalid=!!(_mm_getcsr()&1u),inexact=!!(_mm_getcsr()&0x20u);
#endif
            std::printf("ROUND %d %08x %d %d %d\n",mode,bits,result,invalid,inexact);
        }
    }
    bitarray<160> bits(0u,31u,32u,95u,159u,~0u);
    check(bits.testBit(0)&&bits.testBit(31)&&bits.testBit(32)&&bits.testBit(95)
          &&bits.testBit(159)&&!bits.testBit(158),"native variadic bit positions");
    minspec_mutex mutex{};int value=0;
    auto writer=[&]{for(int i=0;i<2500;++i){mutex.Lock();++value;mutex.Unlock();}};
    std::thread a(writer),b(writer);a.join();b.join();
    check(value==5000&&mutex.m_token==0,"physics mutex exclusion and fixed32 state");
    minspec_read_write_mutex rw;int shared=0;std::atomic<bool> done{};
    std::atomic<bool> valid{true};
    std::thread reader([&]{int previous=0;while(!done){rw.ReadLock();int current=shared;
        if(current<previous)valid=false;previous=current;rw.ReadUnlock();}});
    for(int i=0;i<1000;++i){rw.WriteLock();++shared;rw.WriteUnlock();}
    done=true;reader.join();
    check(valid&&shared==1000&&rw.m_count==1,"physics reader writer publication");
    phys_link_list<Node> list;Node nodes[128]{};
    auto add=[&](unsigned start){for(unsigned i=start;i<start+64;++i){nodes[i].id=i;list.add_mt(&nodes[i]);}};
    std::thread first(add,0),second(add,64);first.join();second.join();
    bool seen[128]{};unsigned count=0;bool ok=true;
    for(Node *p=list.m_first;p;p=p->m_next_link){if(count>=128||p->id>=128||seen[p->id]){ok=false;break;}
        seen[p->id]=true;++count;}
    check(ok&&count==128&&list.m_alloc_count==128,"concurrent native pointer chain after producers join");
}
