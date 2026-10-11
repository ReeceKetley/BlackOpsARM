// TASK-0011 isolated allocation boundary. No DB_InitStreams or asset readers.
#include <platform/fastfile_zone.h>
#include <platform/fastfile_inflate.h>
#include <database/db_memory.h>
#include <platform/bo1_native_width.h>
#include <zlib/zlib.h>
#include <universal/physicalmemory.h>
#include <universal/mem_userhunk.h>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <algorithm>
#include <cmath>
#include <mutex>
#include <new>
#if defined(__ANDROID__)
#include <platform/android/zone_memory.h>
#include <platform/android/hunk_services.h>
#include <platform/android/virtual_memory.h>
#include <time.h>
#include <unistd.h>
#else
#include <Windows.h>
#include <psapi.h>
#endif
using namespace bo1::fastfile;
static void require(bool ok, const char *why) { if (!ok) { std::fprintf(stderr,"FAIL %s\n",why); std::exit(1); } }
static std::uint64_t now() {
#if defined(__ANDROID__)
    timespec t{}; require(clock_gettime(CLOCK_MONOTONIC,&t)==0,"clock");
    return std::uint64_t(t.tv_sec)*1000000000ull+t.tv_nsec;
#else
    LARGE_INTEGER t{},f{}; QueryPerformanceCounter(&t); QueryPerformanceFrequency(&f);
    return std::uint64_t((long double)t.QuadPart*1000000000.0L/f.QuadPart);
#endif
}
struct Memory { long long rss=-1,hwm=-1,vm=-1,peak=-1,privateBytes=-1,peakPrivate=-1; };
static Memory memory() {
    Memory m{};
#if defined(__ANDROID__)
    std::ifstream f("/proc/self/status"); std::string line;
    while (std::getline(f,line)) {
        std::istringstream in(line); std::string key; long long kb;
        if (!(in>>key>>kb)) continue;
        if (key=="VmRSS:") m.rss=kb*1024;
        if (key=="VmHWM:") m.hwm=kb*1024;
        if (key=="VmSize:") m.vm=kb*1024;
        if (key=="VmPeak:") m.peak=kb*1024;
    }
#else
    PROCESS_MEMORY_COUNTERS_EX p{}; p.cb=sizeof(p);
    if (GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&p),sizeof(p))) {
        m.rss=p.WorkingSetSize; m.hwm=p.PeakWorkingSetSize;
        m.privateBytes=p.PrivateUsage; m.peakPrivate=p.PeakPagefileUsage;
    }
#endif
    return m;
}
static std::string memJson(const Memory &m) {
    std::ostringstream o; o<<"{";
    const char *keys[]={"resident_bytes","process_lifetime_resident_hwm_bytes","VmSize_bytes","VmPeak_bytes","windows_private_commit_bytes","windows_peak_pagefile_bytes"};
    long long vals[]={m.rss,m.hwm,m.vm,m.peak,m.privateBytes,m.peakPrivate};
    for(int i=0;i<6;++i) { if(i)o<<","; o<<'"'<<keys[i]<<"\":"; if(vals[i]<0)o<<"null"; else o<<vals[i]; }
    o<<"}"; return o.str();
}
struct Sampler {
    std::atomic<bool> done{false}; std::atomic<long long> peak{0}; std::mutex epoch; std::thread worker;
    Sampler():worker([this]{while(!done.load()){ {std::lock_guard<std::mutex> guard(epoch);auto m=memory();if(m.rss>peak.load())peak=m.rss;} std::this_thread::sleep_for(std::chrono::milliseconds(1));}}){}
    void reset(long long baseline){std::lock_guard<std::mutex> guard(epoch);peak=baseline;}
    void include(long long value){std::lock_guard<std::mutex> guard(epoch);if(value>peak.load())peak=value;}
    ~Sampler(){done=true;worker.join();}
};
#if defined(__ANDROID__)
static bool failRegistration;
bool Bo1_TestZoneRegistrationFailure(){return failRegistration;}
#endif
static std::size_t page() {
#if defined(__ANDROID__)
    return bo1::android::VirtualPageSize();
#else
    SYSTEM_INFO s{}; GetSystemInfo(&s); return s.dwPageSize;
#endif
}
static double clockResolution() {
#if defined(__ANDROID__)
    timespec r{};require(clock_getres(CLOCK_MONOTONIC,&r)==0,"clock resolution");return double(r.tv_sec)*1e9+r.tv_nsec;
#else
    LARGE_INTEGER f{};QueryPerformanceFrequency(&f);return 1e9/double(f.QuadPart);
#endif
}
static ZoneLimits limits(unsigned type=0) {
    std::uint64_t pool=0x14800000u;
#if BO1_NATIVE_64BIT
    pool+=pool/2;
#endif
    return {pool,INT32_MAX-8192u,4096,type,0};
}
static void startup() {
#if defined(__ANDROID__)
    bo1::android::RegisterHunkMainThread();
#endif
}
struct Counts { std::uint64_t owners{},count{},bytes{},reservations{},low{},high{}; };
static Counts counts() {
#if defined(__ANDROID__)
    return {bo1::android::ZoneMemoryCount(),bo1::android::ZoneMemoryAllocationCount(),
        bo1::android::ZoneMemoryRequestedBytes(),bo1::android::VirtualReservationCount(),0,0};
#else
    return {g_mem.prim[0].allocListCount+g_mem.prim[1].allocListCount,0,0,g_mem.buf?1u:0u,g_mem.prim[0].pos,g_mem.prim[1].pos};
#endif
}
static bool equalCounts(const Counts&a,const Counts&b) { return a.owners==b.owners&&a.count==b.count&&a.bytes==b.bytes&&a.reservations==b.reservations&&a.low==b.low&&a.high==b.high; }
static std::string countJson(const Counts &c) { std::ostringstream o; o<<"{\"owners\":"<<c.owners;
#if defined(__ANDROID__)
    o<<",\"native_allocation_count\":"<<c.count<<",\"native_requested_bytes\":"<<c.bytes;
#else
    o<<",\"native_allocation_count\":null,\"native_requested_bytes\":null";
#endif
    o<<",\"reservations\":"<<c.reservations<<",\"windows_low_position\":"<<c.low<<",\"windows_high_position\":"<<c.high<<"}"; return o.str(); }
struct Context {
    const char *name="checked-genuine-zone"; unsigned type{}; int failIndex=-1;
    bool failBegin=false; bool misalign=false; bool throwing=false; bool measure=false;
    unsigned successful{}; std::uint64_t createNs{},allocNs{},releaseNs{},allocStart{};
    Memory afterCreate{},afterAlloc{};
};
static bool begin(void *v,const ZonePlan &p) {
    auto &c=*static_cast<Context*>(v); if(c.failBegin)return false;
#if defined(__ANDROID__)
    if (bo1::android::ZoneMemoryHasOpenOwner() || bo1::android::ZoneMemoryContains(c.name,c.type)) return false;
#else
    if (g_mem.prim[c.type].allocName) return false;
    for (const auto &prim : g_mem.prim) for(unsigned i=0;i<prim.allocListCount;++i)
        if (prim.allocList[i].name && !std::strcmp(prim.allocList[i].name,c.name)) return false;
#endif
    const auto t=now();
#if defined(__ANDROID__)
    bo1::android::ZoneMemoryBegin(c.name,c.type,static_cast<int>(p.firstFitCapacity));
#else
    PMem_Init(); PMem_BeginAlloc(c.name,c.type,TRACK_FASTFILE);
#endif
    c.createNs=now()-t; if(c.measure)c.afterCreate=memory(); c.allocStart=now(); return true;
}
static unsigned char *allocate(void *v,unsigned n,unsigned i) {
    auto &c=*static_cast<Context*>(v);
    if (int(i)==c.failIndex) { if(c.throwing)throw 42; return nullptr; }
    const unsigned type=(i==3||i==6)?2u:(i==0?0u:1u);
    auto *p=DB_MemAlloc(n,type,c.type,c.name,0);
    if(p)++c.successful;
    return p&&c.misalign?p+1:p;
}
static void end(void *v) {
    auto &c=*static_cast<Context*>(v); c.allocNs=now()-c.allocStart;
    if(c.measure)c.afterAlloc=memory();
#if defined(__ANDROID__)
    bo1::android::ZoneMemoryEnd();
#else
    PMem_EndAlloc(c.name,c.type);
#endif
}
static void release(void *v) {
    auto &c=*static_cast<Context*>(v); const auto t=now();
#if defined(__ANDROID__)
    bo1::android::ZoneMemoryRelease(c.name,c.type);
#else
    // Fixture-only: freeTot is a signed cumulative diagnostic, not allocation
    // ownership. Avoid overflowing it during repeated >100MB reference frees.
    extern int freeTot; freeTot=0;
    PMem_Free(c.name);
#endif
    c.releaseNs=now()-t;
}
static ZoneBackend backend(Context&c){return {&c,begin,allocate,end,release};}
static bool empty(const XBlock (&b)[7]){for(auto &x:b)if(x.data||x.size)return false;return true;}
static std::string planJson(const ZonePlan&p){std::ostringstream o;
    o<<"{\"decoded_bytes\":"<<p.decodedBytes<<",\"XFile_size\":"<<p.file.size<<",\"externalSize\":"<<p.file.externalSize
    <<",\"logical_bytes\":"<<p.logicalBytes<<",\"backend_request_bytes\":"<<p.requestBytes<<",\"first_fit_capacity_bytes\":"<<p.firstFitCapacity
    <<",\"windows_low_consumption_bytes\":"<<p.pmemLowBytes<<",\"windows_high_consumption_bytes\":"<<p.pmemHighBytes<<",\"alignment\":4096,\"blocks\":[";
    for(unsigned i=0;i<7;++i){if(i)o<<",";o<<"{\"index\":"<<i<<",\"disk_bytes\":"<<p.file.blockSize[i]<<",\"logical_bytes\":"<<p.logical[i]
    <<",\"request_bytes\":"<<p.request[i]<<",\"factor100\":"<<p.factor100[i]<<",\"additive_expansion_bytes\":"<<(BO1_NATIVE_64BIT&&p.logical[i]?65536:0)<<"}";}
    o<<"]}";return o.str();}
static XFile readHeader(const char *path) { XFile f{};std::ifstream in(path,std::ios::binary);in.read(reinterpret_cast<char*>(&f),sizeof(f));require(in.gcount()==sizeof(f)&&in.peek()==EOF,"exact validated header artifact");return f; }
static void decode(const char *path,const char *out) {
    Sampler sample; const auto before=memory(); Bo1InflatedFastFile decoded; const auto t=now();
    const auto r=Bo1_InflateFastFilePath(path,Bo1InflateLimits{},&decoded);const auto ns=now()-t;
    require(r==Bo1InflateResult::Ok,"genuine wrapped decode");const auto after=memory();
    const auto capacity=decoded.decoded.capacity(); const auto bytes=decoded.decoded.size();
    const auto checksum=adler32(1,decoded.decoded.data(),static_cast<unsigned>(bytes));
    const XFile header=decoded.file;require(!std::memcmp(&header,decoded.decoded.data(),sizeof(header)),"decoded field provenance");
    std::ofstream file(out,std::ios::binary|std::ios::trunc); file.write(reinterpret_cast<const char*>(&header),sizeof(header));file.close();require(bool(file),"header output");
    ZonePlan plan;require(PlanZoneHeader(&header,sizeof(header),bytes,limits(),page(),plan)==ZoneResult::Ok,"decoded plan");
    std::vector<std::uint8_t>().swap(decoded.decoded);const auto cleanup=memory();
    std::ostringstream hex;hex<<std::hex;for(unsigned i=0;i<sizeof(XFile);++i){ auto b=reinterpret_cast<const unsigned char*>(&header)[i];hex.width(2);hex.fill('0');hex<<unsigned(b);}
    std::printf("{\"mode\":\"decode_only_fresh_process\",\"decode_ns\":%llu,\"decoded_adler32\":%lu,\"vector_final_capacity_bytes\":%zu,\"file_bytes\":%llu,\"compressed_bytes\":%llu,\"padding_bytes\":%llu,\"header_hex\":\"%s\",\"plan\":%s,\"before\":%s,\"after_decode\":%s,\"after_cleanup\":%s,\"sampled_peak_resident_bytes\":%lld,\"sample_period_ms\":1}\n",
        (unsigned long long)ns,checksum,capacity,(unsigned long long)decoded.fileBytes,(unsigned long long)decoded.compressedBytes,(unsigned long long)decoded.paddingBytes,hex.str().c_str(),planJson(plan).c_str(),memJson(before).c_str(),memJson(after).c_str(),memJson(cleanup).c_str(),sample.peak.load());
}
static void fixtures() {
    startup();
#if defined(_WIN32)
    PMem_Init();
#endif
    const auto base=counts(); unsigned tests=0;
    XFile f{};f.size=40;f.blockSize[0]=36;f.blockSize[4]=4;
    auto rejected=[&](const char *name,const XFile&h,std::size_t n,std::uint64_t decoded,ZoneLimits l,ZoneResult expected){ZonePlan p;auto r=PlanZoneHeader(&h,n,decoded,l,page(),p);require(r==expected,name);require(p.requestBytes==0&&equalCounts(base,counts()),"rejection preallocation");++tests;std::printf("{\"fixture\":\"%s\",\"status\":\"%s\",\"no_allocation\":true}\n",name,ZoneResultName(r));};
    for(unsigned n=0;n<36;++n)rejected("truncated_header",f,n,76,limits(),ZoneResult::ShortHeader);
    ZonePlan p{};require(PlanZoneHeader(nullptr,36,76,limits(),page(),p)==ZoneResult::ShortHeader,"null header");++tests;
    unsigned char unaligned[37];std::memcpy(unaligned+1,&f,36);require(PlanZoneHeader(unaligned+1,36,76,limits(),page(),p)==ZoneResult::Ok,"unaligned header input");++tests;
    rejected("decoded_length_mismatch",f,36,75,limits(),ZoneResult::SizeMismatch);
    for(unsigned a:{0u,3u,8192u}){auto l=limits();l.alignment=a;rejected("alignment",f,36,76,l,ZoneResult::Alignment);}
    auto l=limits();l.allocType=2;rejected("alloc_type",f,36,76,l,ZoneResult::Unsupported);
    l=limits();l.flags=0x40000000;rejected("mixed_Z_physical_flags",f,36,76,l,ZoneResult::Unsupported);
    XFile bad=f;bad.blockSize[0]=UINT32_MAX;rejected("uint32_block_overflow",bad,36,76,limits(),ZoneResult::BlockOverflow);
    bad=f;bad.blockSize[0]=INT32_MAX;rejected("signed_backend_bound",bad,36,76,limits(),ZoneResult::BlockOverflow);
    bad=f;for(auto&b:bad.blockSize)b=0x30000000;rejected("aggregate_crosses_32bit_capacity",bad,36,76,limits(),ZoneResult::AggregateLimit);
    l=limits();l.capacityBytes=1024;rejected("reservation_budget",f,36,76,l,ZoneResult::AggregateLimit);
    std::uint64_t sum=9;require(!ZoneCheckedAdd(UINT64_MAX,1,sum)&&sum==9,"uint64 checked helper overflow");++tests;
    for(unsigned n:{0xfffffu,0x100000u,0x1ffffffu,0x2000000u}){auto h=f;h.blockSize[0]=n;require(PlanZoneHeader(&h,36,76,limits(),page(),p)==ZoneResult::Ok,"factor boundary");Context c;XBlock b[7]{};begin(&c,p);DB_AllocXBlocks(h.blockSize,c.name,b,0,0);end(&c);for(unsigned i=0;i<7;++i)require(b[i].size==p.logical[i],"actual production expansion parity");DB_FreeXBlocks(b,0);release(&c);require(equalCounts(base,counts()),"production parity cleanup");++tests;}
    require(PlanZoneHeader(&f,36,76,limits(),page(),p)==ZoneResult::Ok,"fixture plan");
    for(int variant=0;variant<4;++variant){auto tampered=p;if(variant==0)++tampered.logical[0];if(variant==1)++tampered.request[0];if(variant==2)++tampered.firstFitCapacity;if(variant==3)++tampered.requestBytes;Context c;XBlock b[7]{};unsigned failed;require(AllocateZonePlan(tampered,page(),backend(c),b,failed)==ZoneResult::InconsistentPlan,"tampered derived request");require(empty(b)&&equalCounts(base,counts()),"tamper cleanup");++tests;}
    Context c;XBlock occupied[7]{};occupied[2].size=1;unsigned failed;require(AllocateZonePlan(p,page(),backend(c),occupied,failed)==ZoneResult::OccupiedOutput&&occupied[2].size==1,"occupied output preserved");++tests;
    c.failBegin=true;XBlock b[7]{};require(AllocateZonePlan(p,page(),backend(c),b,failed)==ZoneResult::BeginFailed,"controlled begin failure");require(empty(b)&&equalCounts(base,counts()),"begin failure cleanup");++tests;
#if defined(__ANDROID__)
    failRegistration=true;Context registration;
    try{AllocateZonePlan(p,page(),backend(registration),b,failed);require(false,"expected registration failure");}catch(const std::bad_alloc&){}
    failRegistration=false;require(empty(b)&&equalCounts(base,counts())&&!bo1::android::ZoneMemoryHasOpenOwner(),"registration failure cleanup");++tests;
    std::puts("{\"fixture\":\"post_reservation_owner_registration_bad_alloc\",\"restored\":true}");
#endif
    Context existing;require(begin(&existing,p),"existing owner");end(&existing);const auto unrelated=counts();Context duplicate;
    require(AllocateZonePlan(p,page(),backend(duplicate),b,failed)==ZoneResult::BeginFailed&&equalCounts(unrelated,counts()),"duplicate preserves owner");
    release(&existing);require(equalCounts(base,counts()),"existing owner release");++tests;
    for(int i:{0,4}){Context fail;fail.failIndex=i;require(AllocateZonePlan(p,page(),backend(fail),b,failed)==ZoneResult::AllocationFailed&&failed==unsigned(i),"controlled partial failure");require(empty(b)&&equalCounts(base,counts()),"partial ownership cleanup");++tests;std::printf("{\"fixture\":\"controlled_partial_failure\",\"failed_block\":%d,\"real_allocations_before_failure\":%u,\"restored\":true}\n",i,fail.successful);}
    Context wrong;wrong.misalign=true;require(AllocateZonePlan(p,page(),backend(wrong),b,failed)==ZoneResult::Alignment&&empty(b)&&equalCounts(base,counts()),"backend alignment cleanup");++tests;
    Context exception;exception.failIndex=4;exception.throwing=true;try{AllocateZonePlan(p,page(),backend(exception),b,failed);require(false,"expected injected exception");}catch(int n){require(n==42,"exception identity");}require(empty(b)&&equalCounts(base,counts()),"exception cleanup");++tests;
    XFile zero{};require(PlanZoneHeader(&zero,36,36,limits(),page(),p)==ZoneResult::Ok,"all zero plan");c={};require(AllocateZonePlan(p,page(),backend(c),b,failed)==ZoneResult::Ok&&empty(b)&&equalCounts(base,counts()),"zero no owner");++tests;
    // Genuine-independent sparse fixture covers every physical/nonphysical slot.
    f={};for(auto&x:f.blockSize)x=1;require(PlanZoneHeader(&f,36,36,limits(1),page(),p)==ZoneResult::Ok,"all slot type plan");c={};c.type=1;require(AllocateZonePlan(p,page(),backend(c),b,failed)==ZoneResult::Ok,"high owner all slots");for(auto&x:b)require(x.data&&x.size,"all slots owned");DB_FreeXBlocks(b,0);release(&c);require(empty(b)&&equalCounts(base,counts()),"high owner cleanup");++tests;
    std::printf("{\"fixtures_passed\":%u,\"baseline\":%s,\"final\":%s}\n",tests,countJson(base).c_str(),countJson(counts()).c_str());
}
static void validateGenuine(const char *path) {
    const auto f=readHeader(path);startup();
#if defined(_WIN32)
    PMem_Init();
#endif
    const auto baseline=counts();ZonePlan p;require(PlanZoneHeader(&f,36,std::uint64_t(f.size)+36,limits(),page(),p)==ZoneResult::Ok,"genuine failure plan");
    for(unsigned i=0;i<7;++i){if(!p.logical[i])continue;Context c;c.failIndex=int(i);XBlock b[7]{};unsigned failed;
        require(AllocateZonePlan(p,page(),backend(c),b,failed)==ZoneResult::AllocationFailed&&failed==i,"genuine controlled failure");
        require(empty(b)&&equalCounts(baseline,counts()),"genuine failure cleanup");
        std::printf("{\"fixture\":\"genuine_partial_failure\",\"failed_block\":%u,\"real_prior_allocations\":%u,\"restored\":true}\n",i,c.successful);
    }
    for(unsigned type:{0u,1u}){Context c;c.type=type;ZonePlan q;require(PlanZoneHeader(&f,36,std::uint64_t(f.size)+36,limits(type),page(),q)==ZoneResult::Ok,"genuine reference plan");
        XBlock blocks[7]{};begin(&c,q);auto header=f;DB_AllocXBlocks(header.blockSize,c.name,blocks,type,0);end(&c);
        for(unsigned i=0;i<7;++i)require(blocks[i].size==q.logical[i]&&bool(blocks[i].data)==bool(q.logical[i]),"genuine actual DB_AllocXBlocks parity");
        DB_FreeXBlocks(blocks,0);release(&c);require(empty(blocks)&&equalCounts(baseline,counts()),"production genuine release");
    }
    std::printf("{\"genuine_production_parity\":true,\"alloc_types\":[0,1],\"baseline\":%s,\"final\":%s}\n",countJson(baseline).c_str(),countJson(counts()).c_str());
}
static void backendFailure() {
    startup();Context c;ZonePlan p{};p.firstFitCapacity=65536;begin(&c,p);
    require(DB_MemAlloc(1,0,0,c.name,0)!=nullptr,"real allocation before fatal failure");
#if defined(__ANDROID__)
    DB_MemAlloc(65536,0,0,c.name,0);
#else
    DB_MemAlloc(g_mem.size,0,0,c.name,0);
#endif
    require(false,"backend failure must terminate");
}
static void allocation(const char *path,unsigned iterations,unsigned warmups,unsigned type) {
    require(iterations>0&&iterations<=100&&warmups<=10,"sample count");const XFile f=readHeader(path);ZonePlan p;
    require(PlanZoneHeader(&f,36,std::uint64_t(f.size)+36,limits(type),page(),p)==ZoneResult::Ok,"genuine plan");startup();
    Sampler sampler;std::vector<std::string> samples;samples.reserve(iterations);
    for(unsigned run=0;run<warmups+iterations;++run){Context c;c.type=type;c.measure=true;XBlock b[7]{};unsigned failed;
        const auto before=memory();const auto entry=counts();sampler.reset(before.rss);
        require(AllocateZonePlan(p,page(),backend(c),b,failed)==ZoneResult::Ok,"genuine checked allocation");
        const auto owned=counts();std::uint64_t touchedPages=0;
        for(unsigned i=0;i<7;++i){require(b[i].size==p.logical[i]&&bool(b[i].data)==bool(p.logical[i]),"actual block sizes");if(!b[i].data)continue;
            require(reinterpret_cast<std::uintptr_t>(b[i].data)%4096==0,"actual alignment");
#if defined(__ANDROID__)
            const auto start=reinterpret_cast<std::uintptr_t>(b[i].data);touchedPages+=((start+p.request[i]+page()-1)/page()-start/page())*page();
            require(b[i].data[0]==0&&b[i].data[p.request[i]-1]==0,"native payload zeroing");
#endif
        }
        sampler.include(c.afterCreate.rss);sampler.include(c.afterAlloc.rss);
        DB_FreeXBlocks(b,0);require(empty(b),"descriptor reset");release(&c);const auto cleanup=memory();const auto final=counts();sampler.include(cleanup.rss);
#if defined(_WIN32)
        require(final.owners==entry.owners&&final.low==entry.low&&final.high==(entry.reservations?entry.high:g_mem.size),"PMem logical rewind");
        require(owned.low==(type?entry.low:p.pmemLowBytes)&&owned.high==(type?g_mem.size-p.pmemHighBytes:g_mem.size),"production cursor plan parity");
#else
        require(equalCounts(entry,final),"native counts/reservations restored");require(owned.count-entry.count==c.successful&&owned.bytes-entry.bytes==p.requestBytes,"actual owner accounting");
#endif
        if(run<warmups)continue;
        std::ostringstream s;s<<"{\"iteration\":"<<(run-warmups)<<",\"create_ns\":"<<c.createNs<<",\"allocate_ns\":"<<c.allocNs<<",\"release_ns\":"<<c.releaseNs<<",\"total_phase_ns\":"<<(c.createNs+c.allocNs+c.releaseNs)
            <<",\"before\":"<<memJson(before)<<",\"after_create\":"<<memJson(c.afterCreate)<<",\"after_allocate\":"<<memJson(c.afterAlloc)<<",\"after_cleanup\":"<<memJson(cleanup)<<",\"sampled_peak_resident_bytes\":"<<sampler.peak.load()
            <<",\"entry_counters\":"<<countJson(entry)<<",\"owned_counters\":"<<countJson(owned)<<",\"final_counters\":"<<countJson(final)<<",\"payload_zeroed_bytes\":";
#if defined(__ANDROID__)
        s<<p.requestBytes;
#else
        s<<0;
#endif
        s<<",\"payload_touched_page_bytes\":"<<touchedPages<<",\"actual_block_count\":"<<c.successful
            <<",\"logical_ownership_restored\":true,\"raw_counters_match\":"<<(equalCounts(entry,final)?"true":"false")
            <<",\"pool_initialized_and_retained\":"<<(!entry.reservations&&final.reservations?"true":"false")<<"}";samples.push_back(s.str());
    }
    std::printf("{\"mode\":\"allocation_only_validated_header\",\"native_pointer_bytes\":%zu,\"page_bytes\":%zu,\"iterations\":%u,\"excluded_warmups\":%u,\"alloc_type\":%u,\"clock_resolution_ns\":%.9g,\"clock\":\"%s\",\"plan\":%s,\"samples\":[",sizeof(void*),page(),iterations,warmups,type,clockResolution(),
#if defined(__ANDROID__)
        "CLOCK_MONOTONIC",
#else
        "QueryPerformanceCounter",
#endif
        planJson(p).c_str());for(unsigned i=0;i<samples.size();++i)std::printf("%s%s",i?",":"",samples[i].c_str());std::puts("]}");
}
int main(int argc,char**argv) {
    if(argc==2&&!std::strcmp(argv[1],"fixtures")){fixtures();return 0;}
    if(argc==4&&!std::strcmp(argv[1],"decode")){decode(argv[2],argv[3]);return 0;}
    if(argc==3&&!std::strcmp(argv[1],"validate")){validateGenuine(argv[2]);return 0;}
    if(argc==2&&!std::strcmp(argv[1],"backend-failure")){backendFailure();return 1;}
    if(argc>=5&&!std::strcmp(argv[1],"alloc")){allocation(argv[2],unsigned(std::strtoul(argv[3],nullptr,10)),unsigned(std::strtoul(argv[4],nullptr,10)),argc==6?unsigned(std::strtoul(argv[5],nullptr,10)):0);return 0;}
    std::fputs("usage: checked_zone fixtures | decode asset.ff header.bin | alloc header.bin iterations warmups [type]\n",stderr);return 1;
}
