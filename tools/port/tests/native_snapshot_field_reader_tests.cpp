#include <qcommon/sv_msg_write.h>
#include <qcommon/msg.h>
#include <platform/android/thread_services.h>
#include <platform/critical_section_services.h>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <sys/mman.h>
#include <unistd.h>

namespace {
unsigned checks;
void check(bool ok,const char *name) {
    if (!ok) { std::fprintf(stderr,"FAIL %s\n",name); std::abort(); }
    ++checks;
}
struct Tail {
    size_t page;
    unsigned char *mapping,*data;
    explicit Tail(unsigned size) {
        page=static_cast<size_t>(sysconf(_SC_PAGESIZE));
        mapping=static_cast<unsigned char *>(mmap(nullptr,page*2,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));
        check(mapping!=MAP_FAILED,"owned mapping");
        check(mprotect(mapping+page,page,PROT_NONE)==0,"tail guard");
        std::memset(mapping,0xA5,page);data=mapping+page-size;
    }
    ~Tail() { munmap(mapping,page*2); }
};
NetField field(int offset,int size,int bits) { return {"reader",offset,size,bits,0u,"reader","0"}; }
msg_t message(unsigned char *data,unsigned size) { msg_t m{};m.data=data;m.maxsize=size;return m; }
void encode(msg_t &m,const NetField &f,const void *a,const void *b,const int *baseline=nullptr) {
    SnapshotInfo_s snapshot{};
    const auto *from=static_cast<const unsigned char *>(a),*to=static_cast<const unsigned char *>(b);
    check(MSG_WriteDeltaField_Internal(&snapshot,&m,1000,from,to,&f,0,
        baseline?baseline:reinterpret_cast<const int *>(from+f.offset),
        reinterpret_cast<const int *>(to+f.offset))==1,"canonical producer");
    check(!m.overflowed,"producer bounded");
}
void decode(msg_t &m,const NetField &f,void *from,void *to,bool noXor=false) {
    MSG_BeginReading(&m);
    MSG_ReadDeltaField(&m,1000,static_cast<char *>(from),static_cast<char *>(to),&f,0,noXor);
    check(!m.overflowed,"canonical reader bounded");
}
void expectedPlain(msg_t &m,int bits,std::uint32_t a,std::uint32_t b) {
    unsigned char expected[8]{};
    const unsigned width=bits<0?-bits:bits;
    const unsigned remainder=width&7;
    const std::uint32_t mask=width==32?UINT32_MAX:(std::uint32_t{1}<<width)-1;
    const std::uint32_t x=(a^b)&mask;
    expected[0]=1|((x&((1u<<remainder)-1))<<1);
    unsigned size=1;
    for (unsigned j=remainder;j<width;j+=8) expected[size++]=static_cast<unsigned char>(x>>j);
    check(m.cursize==static_cast<int>(size)&&std::memcmp(m.data,expected,size)==0,"independent signed/unsigned wire bytes");
}
void genericPacked() {
    struct Case { int size,bits,a,b; };
    const Case cases[]={{1,-8,-127,-126},{2,-16,-32767,-32766},{4,26,0,0x03B7FB2E},
        {4,31,0x01234567,0x76543210},{4,-31,-12345,-1234},
        {4,32,std::bit_cast<int>(0x89ABCDEFu),std::bit_cast<int>(0xFEDCBA98u)},
        {4,-32,std::bit_cast<int>(0x89ABCDEFu),std::bit_cast<int>(0xFEDCBA98u)}};
    for (unsigned offset: {1u,2u}) for (const Case &c:cases) {
        alignas(8) unsigned char a[16],b[16],out[16];unsigned char wire[32]{};
        std::memset(a,0xA5,16);std::memset(b,0xA5,16);std::memset(out,0xA5,16);
        std::memcpy(a+offset,&c.a,c.size);std::memcpy(b+offset,&c.b,c.size);
        const NetField f=field(offset,c.size,c.bits);msg_t m=message(wire,sizeof(wire));
        encode(m,f,a,b);expectedPlain(m,c.bits,c.a,c.b);decode(m,f,a,out);
        check(std::memcmp(out+offset,b+offset,c.size)==0,"packed plain field value");
        check(out[offset-1]==0xA5&&out[offset+c.size]==0xA5,"plain exact-width canaries");
        std::memset(wire,0,32);m=message(wire,32);m.cursize=1;decode(m,f,a,out);
        const unsigned char zero[4]{};
        check(std::memcmp(out+offset,zero,c.size)==0,"default zero branch own bytes");
        check(out[offset-1]==0xA5&&out[offset+c.size]==0xA5,"zero branch canaries");
    }
    for (unsigned size: {1u,2u}) {
        Tail a(size),b(size),out(size);const int av=size==1?-127:-32767,bv=av+1;
        std::memcpy(a.data,&av,size);std::memcpy(b.data,&bv,size);
        const NetField f=field(0,size,-8*static_cast<int>(size));unsigned char storage[32]{};msg_t m=message(storage,32);
        encode(m,f,a.data,b.data);expectedPlain(m,f.bits,av,bv);decode(m,f,a.data,out.data);
        check(std::memcmp(out.data,b.data,size)==0&&out.data[-1]==0xA5,"reader narrow PROT_NONE tail");
        std::memset(storage,0,32);m=message(storage,32);m.cursize=1;decode(m,f,a.data,out.data);
        const unsigned char zero[2]{};check(std::memcmp(out.data,zero,size)==0&&out.data[-1]==0xA5,"zero narrow PROT_NONE tail");
    }
}
void actualEntity() {
    static_assert(sizeof(entityState_s)==224&&offsetof(entityState_s,clientLinkInfo)==218);
    const NetField *f=nullptr;
    for (const auto &candidate:entityStateFields) if (!std::strcmp(candidate.name,"clientLinkInfo")) f=&candidate;
    check(f&&f->offset==218&&f->size==4&&f->bits==26,"actual canonical NetField metadata");
    entityState_s a{},b{},out{};b.clientLinkInfo.parentEnt=-1234;b.clientLinkInfo.tagIndex=0xB7;b.clientLinkInfo.flags=3;
    out.animtreeIndex=0xCD;unsigned char storage[32]{};msg_t m=message(storage,32);
    encode(m,*f,&a,&b);const unsigned char expected[]={5,0xCB,0xFE,0xED};
    check(m.cursize==4&&std::memcmp(storage,expected,4)==0,"real composite26-bit wire");
    decode(m,*f,&a,&out);
    check(std::memcmp(&out.clientLinkInfo,&b.clientLinkInfo,4)==0&&out.animtreeIndex==0xCD,"real entity reader span218..221");
    // Decode that same packet with a nonzero source but noXor selected: zeroVal
    // is the original canonical baseline, not a replacement field result.
    a.clientLinkInfo.parentEnt=123;a.clientLinkInfo.tagIndex=77;a.clientLinkInfo.flags=1;
    decode(m,*f,&a,&out,true);
    check(std::memcmp(&out.clientLinkInfo,&b.clientLinkInfo,4)==0&&out.animtreeIndex==0xCD,"canonical noXor baseline");
}
void specialPacked() {
    struct Case { int size,bits;std::uint32_t a,b,expected; };
    const Case cases[]={
        {4,-63,0,0xDEADBEEFu,0xDEADBEEFu},{4,-88,0x7FC01234u,0x7FC01235u,0x7FC01235u},
        {4,-98,0x10,0x14,0x14},{4,-97,0,975,975},{4,-95,0,1200,1200},
        {1,-94,0,7,7},{2,-94,0,7,7},{4,-94,0,7,7},
        {1,-93,0,7,7},{2,-93,0,7,7},{4,-93,0,7,7},
        {2,-96,0,1022,1022},{4,-96,0,1022,1022},
        {1,-59,11,12,12},{2,-59,11,12,12},{4,-59,11,12,12},
        {1,-60,static_cast<unsigned>(-2),3,3},{2,-60,static_cast<unsigned>(-2),3,3},{4,-60,static_cast<unsigned>(-2),3,3},
        {1,-58,22,7,7},{2,-58,22,7,7},{4,-58,22,7,7},
        {4,0,0,0x80000000u,0x80000000u},{4,0,0x3F800000u,0x40200000u,0x40200000u},
        {4,-89,0x3F800000u,0x40200000u,0x40200000u},{4,-99,0,0x80000000u,0x80000000u},
        {4,-99,0x3F800000u,0x40000000u,0x40000000u},{4,-81,0,0x3F000000u,0x3F000000u},
        {4,-80,0,0x3F000000u,0x3F000000u},{4,-77,0,0x3F000000u,0x3F000000u},
        {2,-76,0,0,0},{2,-75,0,0,0},{2,-74,0,0,0},
        {4,-73,0,0x3F000000u,0x3F000000u},{4,-72,0,0x3F000000u,0x3F000000u},
        {4,-106,0,0x3F000000u,0x3F000000u},{4,-107,0,0x3F000000u,0x3F000000u},
        {4,-105,0,0x42B40000u,0x42B40000u},{4,-87,0,0x42B40000u,0x42B40000u},
        {4,-100,0,0x42B40000u,0x42B40000u},{4,-104,0,0x3F800000u,0x3F800000u},
        {4,-103,0,0x3F800000u,0x3F800000u},{4,-86,0,0x3FC00000u,0x3FC00000u},
        {4,-85,0xFF332211u,0x00332211u,0x00332211u}
    };
    for (const Case &c:cases) {
        alignas(8) unsigned char a[16],b[16],out[16];unsigned char storage[32]{};
        const unsigned offset=c.bits==-85?4:1; // Actual HUD color union owner is4-aligned.
        std::memset(a,0xA5,16);std::memset(b,0xA5,16);std::memset(out,0xA5,16);
        std::memcpy(a+offset,&c.a,c.size);std::memcpy(b+offset,&c.b,c.size);
        const NetField f=field(offset,c.size,c.bits);msg_t m=message(storage,32);
        std::fprintf(stdout,"case bits%d size%d\n",c.bits,c.size);std::fflush(stdout);
        encode(m,f,a,b);decode(m,f,a,out);
        check(std::memcmp(out+offset,&c.expected,c.size)==0,"special independent decoded bits");
        check(out[offset-1]==0xA5&&out[offset+c.size]==0xA5,"special exact-width canaries");
    }
}
}
int main() {
    Sys_InitMainThread();Sys_InitializeCriticalSections();
    genericPacked();actualEntity();specialPacked();
    std::printf("PASS %u actual packed NetField reader/producer/width/tail checks\n",checks);
    return 0;
}
