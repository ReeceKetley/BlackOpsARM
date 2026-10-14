#include <qcommon/sv_msg_write.h>
#include <qcommon/msg.h>
#include <platform/android/thread_services.h>
#include <platform/critical_section_services.h>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>

namespace {
unsigned checks;
void check(bool condition, const char *name) {
    if (!condition) { std::fprintf(stderr,"FAIL %s\n", name); std::abort(); }
    ++checks;
}
struct Tail {
    size_t page;
    unsigned char *mapping;
    unsigned char *data;
    explicit Tail(unsigned size) {
        page = static_cast<size_t>(sysconf(_SC_PAGESIZE));
        mapping = static_cast<unsigned char *>(mmap(nullptr, page*2, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0));
        check(mapping != MAP_FAILED, "owned guard mapping");
        check(mprotect(mapping+page, page, PROT_NONE) == 0, "guard page installed");
        std::memset(mapping, 0xA5, page);
        data = mapping+page-size;
    }
    ~Tail() { munmap(mapping,page*2); }
};
NetField makeField(int offset, int size, int bits) {
    return {"fixture", offset, size, bits, 0u, "fixture", "0"};
}
msg_t message(unsigned char *data, unsigned size) {
    msg_t msg{}; msg.data=data; msg.maxsize=size; return msg;
}
void wire(msg_t &msg, const unsigned char *expected, unsigned size) {
    check(!msg.overflowed && msg.cursize==static_cast<int>(size), "wire length");
    check(std::memcmp(msg.data,expected,size)==0, "independent wire bytes");
}
void narrowTails(unsigned size, int fromValue, int toValue) {
    Tail from(size), to(size), copied(size);
    std::memcpy(from.data,&fromValue,size);
    std::memcpy(to.data,&toValue,size);
    const NetField field=makeField(0,size,8*size);
    const int *fromF=reinterpret_cast<const int *>(from.data);
    const int *toF=reinterpret_cast<const int *>(to.data);
    check(MSG_GetField(fromF,size)==fromValue, "signed narrow field load");
    check(!MSG_ValuesAreEqual(0,field.bits,size,fromF,toF), "changed narrow tail");
    check(MSG_ValuesAreEqual(0,field.bits,size,fromF,fromF), "same narrow tail");
    MSG_CopyFieldOver(&field,reinterpret_cast<char *>(to.data),reinterpret_cast<char *>(copied.data),0);
    check(std::memcmp(copied.data,to.data,size)==0 && copied.data[-1]==0xA5, "exact narrow copyback tail");
    SnapshotInfo_s snapshot{};
    unsigned char storage[32]{};
    msg_t msg=message(storage,sizeof(storage));
    check(MSG_WriteDeltaField(&snapshot,&msg,0,from.data,from.data,&field,2,false,true,0,1,true)==0 && msg.cursize==0, "unchanged tail early return");
    check(MSG_WriteDeltaField(&snapshot,&msg,0,from.data,to.data,&field,2,false,true,0,1,true)==1, "changed narrow diagnostic and skipped encoding");
    const unsigned char expectedChanged[]={6,3,0};
    wire(msg,expectedChanged,1+size);
    std::memset(storage,0,sizeof(storage)); msg=message(storage,sizeof(storage));
    check(MSG_WriteDeltaField(&snapshot,&msg,0,from.data,from.data,&field,2,true,true,0,1,true)==1, "force unchanged narrow tail");
    const unsigned char expectedForced[]={6,0,0};
    wire(msg,expectedForced,1+size);
    check(from.data[-1]==0xA5 && to.data[-1]==0xA5, "narrow input canaries");
}
void packedScalars() {
    for (unsigned offset=1;offset<=2;++offset) {
        alignas(8) unsigned char source[16], target[16];
        std::memset(source,0xA5,sizeof(source)); std::memset(target,0xA5,sizeof(target));
        const int value=std::bit_cast<int>(0xDEADBEEFu);
        std::memcpy(source+offset,&value,4);
        check(MSG_GetField(reinterpret_cast<const int *>(source+offset),4)==value, "packed fixed32 load");
        const NetField field=makeField(offset,4,-88);
        MSG_CopyFieldOver(&field,reinterpret_cast<char *>(source),reinterpret_cast<char *>(target),0);
        check(std::memcmp(target+offset,source+offset,4)==0 && target[offset-1]==0xA5 && target[offset+4]==0xA5, "packed fixed32 copyback canaries");
    }
    alignas(8) unsigned char source[16], target[16], copied[16];
    std::memset(source,0xA5,sizeof(source)); std::memset(target,0xA5,sizeof(target)); std::memset(copied,0xA5,sizeof(copied));
    const short shortA=-32768, shortB=-32767;
    std::memcpy(source+1,&shortA,2); std::memcpy(target+1,&shortB,2);
    check(MSG_GetField(reinterpret_cast<const int *>(source+1),2)==-32768, "offset-one signed short load");
    check(MSG_ValuesAreEqual(0,-76,2,reinterpret_cast<const int *>(source+1),reinterpret_cast<const int *>(target+1)), "short quantization same lowest tenth");
    const short shortC=0; std::memcpy(target+1,&shortC,2);
    check(!MSG_ValuesAreEqual(0,-76,2,reinterpret_cast<const int *>(source+1),reinterpret_cast<const int *>(target+1)), "short quantization different half range");
    const NetField shortField=makeField(1,2,-76);
    MSG_CopyFieldOver(&shortField,reinterpret_cast<char *>(source),reinterpret_cast<char *>(copied),0);
    check(std::memcmp(copied+1,source+1,2)==0 && copied[0]==0xA5 && copied[3]==0xA5, "offset-one short copyback canaries");
}
void entityComposite() {
    static_assert(sizeof(entityState_s)==224);
    static_assert(offsetof(entityState_s,clientLinkInfo)==218);
    entityState_s from{}, to{}, copied{};
    to.clientLinkInfo.parentEnt=static_cast<short>(-1234);
    to.clientLinkInfo.tagIndex=0xB7;
    to.clientLinkInfo.flags=3;
    const NetField field=makeField(218,4,26);
    const int *fromF=reinterpret_cast<const int *>(reinterpret_cast<const unsigned char *>(&from)+218);
    const int *toF=reinterpret_cast<const int *>(reinterpret_cast<const unsigned char *>(&to)+218);
    check(!MSG_ValuesAreEqual(0,26,4,fromF,toF), "real entity packed comparison");
    check(std::bit_cast<std::uint32_t>(MSG_GetField(toF,4))==0x03B7FB2Eu, "real parent16 tag8 flags8 bits");
    MSG_CopyFieldOver(&field,reinterpret_cast<char *>(&to),reinterpret_cast<char *>(&copied),0);
    check(std::memcmp(&copied.clientLinkInfo,&to.clientLinkInfo,4)==0 && copied.animtreeIndex==0, "real entity copyback preserves next byte");
    unsigned char storage[32]{};
    msg_t msg=message(storage,sizeof(storage)); SnapshotInfo_s snapshot{};
    check(MSG_WriteDeltaField_Internal(&snapshot,&msg,0,reinterpret_cast<const unsigned char *>(&from),reinterpret_cast<const unsigned char *>(&to),&field,0,fromF,toF)==1, "real packed entity producer");
    // Presence1 plus low two XOR bits10, then bytes of the remaining 24 bits.
    const unsigned char expected[]={5,0xCB,0xFE,0xED};
    wire(msg,expected,sizeof(expected));
}
void floatCases() {
    struct Case { int bits; float a,b; bool equal; };
    const Case cases[]={
        {-104,1.1f,1.2f,true}, {-104,-1.1f,-1.2f,true}, {-104,1.1f,1.6f,false},
        {-105,90.0f,90.01f,true}, {-105,90.0f,91.0f,false},
        {-86,1.1f,1.14f,true}, {-86,1.1f,1.2f,false}
    };
    alignas(8) unsigned char a[16],b[16];
    std::memset(a,0xA5,sizeof(a)); std::memset(b,0xA5,sizeof(b));
    for (const Case &c:cases) {
        std::memcpy(a+2,&c.a,4); std::memcpy(b+2,&c.b,4);
        check(MSG_ValuesAreEqual(0,c.bits,4,reinterpret_cast<const int *>(a+2),reinterpret_cast<const int *>(b+2))==c.equal, "independent packed float quantization");
    }
    const std::uint32_t rawA=0x7FC01234u, rawB=0x7FC01235u;
    std::memcpy(a+2,&rawA,4); std::memcpy(b+2,&rawB,4);
    check(MSG_ValuesAreEqual(0,-88,4,reinterpret_cast<const int *>(a+2),reinterpret_cast<const int *>(a+2)), "same NaN raw bits equal before quantization");
    check(!MSG_ValuesAreEqual(0,-88,4,reinterpret_cast<const int *>(a+2),reinterpret_cast<const int *>(b+2)), "different NaN raw bits unequal");
    const NetField rawField=makeField(2,4,-88); SnapshotInfo_s snapshot{};
    unsigned char storage[32]{}; msg_t msg=message(storage,sizeof(storage));
    MSG_WriteDeltaField_Internal(&snapshot,&msg,0,a,b,&rawField,0,reinterpret_cast<const int *>(a+2),reinterpret_cast<const int *>(b+2));
    const unsigned char expectedRaw[]={1,0,0,0}; wire(msg,expectedRaw,4);
    const float zero=0.0f, ninety=90.0f;
    std::memcpy(a+2,&zero,4); std::memcpy(b+2,&ninety,4);
    const NetField angleField=makeField(2,4,-105);
    std::memset(storage,0,sizeof(storage)); msg=message(storage,sizeof(storage));
    MSG_WriteDeltaField_Internal(&snapshot,&msg,0,a,b,&angleField,0,reinterpret_cast<const int *>(a+2),reinterpret_cast<const int *>(b+2));
    const unsigned char expectedAngle[]={0,1}; wire(msg,expectedAngle,2);
    const std::uint32_t negativeZero=0x80000000u;
    std::memcpy(b+2,&negativeZero,4);
    const NetField zeroField=makeField(2,4,-99);
    std::memset(storage,0,sizeof(storage)); msg=message(storage,sizeof(storage));
    MSG_WriteDeltaField_Internal(&snapshot,&msg,0,a,b,&zeroField,0,reinterpret_cast<const int *>(a+2),reinterpret_cast<const int *>(b+2));
    const unsigned char expectedNegativeZero[]={3,0,0,0,0x80}; wire(msg,expectedNegativeZero,5);
    check(a[1]==0xA5 && a[6]==0xA5 && b[1]==0xA5 && b[6]==0xA5, "float input canaries");
}
}
int main() {
    Sys_InitMainThread(); Sys_InitializeCriticalSections();
    narrowTails(1,-127,-126);
    narrowTails(2,-32767,-32766);
    packedScalars(); entityComposite(); floatCases();
    std::printf("PASS %u actual packed scalar/producer/tail/copyback checks\n",checks);
    return 0;
}
