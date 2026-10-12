#include <platform/native_zone_file.h>
#include <database/db_stream.h>
#include <universal/physicalmemory.h>
#include <zlib/zlib.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <array>
#if defined(__ANDROID__)
#include <platform/android/hunk_services.h>
#include <platform/android/virtual_memory.h>
#include <platform/android/zone_memory.h>
bool Bo1_TestZoneRegistrationFailure() { return false; }
#else
#include <Windows.h>
#endif
using namespace bo1::fastfile;
static void require(bool value, const char *why) { if (!value) { std::fprintf(stderr,"FAIL %s\n",why); std::exit(1); } }
int main(int argc, char **argv) {
    require(argc == 2,"real retail file path required");
#if defined(__ANDROID__)
    bo1::android::RegisterHunkMainThread();
    const auto page = bo1::android::VirtualPageSize();
#else
    PMem_Init(); SYSTEM_INFO info{}; GetSystemInfo(&info); const auto page = info.dwPageSize;
#endif
    XBlock detached[7]{}; DB_InitStreams(detached);
    ZoneLimits limits{64u*1024u*1024u, INT32_MAX-8192u,4096,0,0};
    Bo1InflateLimits inflate; inflate.maxDecodedBytes = 1024*1024;
    NativeZoneFile zone;
#if defined(_WIN32) && BO1_NATIVE_64BIT
    PMem_BeginAlloc("native-zone-existing-owner1",0,TRACK_FASTFILE);
    PMem_EndAlloc("native-zone-existing-owner1",0);
    PMem_BeginAlloc("native-zone-existing-owner2",0,TRACK_FASTFILE);
    PMem_EndAlloc("native-zone-existing-owner2",0);
    require(zone.Open(argv[1],"native-production-first-zone",inflate,limits,page,8*1024*1024)==NativeZoneResult::Ok,"third native PMem owner allocated");
    require(g_mem.prim[0].allocListCount==3,"three native owners retained");
    DB_InitStreams(zone.Blocks());
    require(zone.Close()==NativeZoneResult::StreamsAttached,"third owner obeys stream lifetime");
    DB_InitStreams(detached);
    require(zone.Close()==NativeZoneResult::Ok&&g_mem.prim[0].allocListCount==2&&!g_mem.prim[0].pos,"third owner removed without releasing earlier owners");
    PMem_Free("native-zone-existing-owner2"); PMem_Free("native-zone-existing-owner1");
    std::array<std::array<char,32>,31> retainedNames{};
    for(unsigned type=0;type<2;++type) {
        for(unsigned i=0;i<31;++i) {
            std::snprintf(retainedNames[i].data(),retainedNames[i].size(),"retained-owner-%u-%u",type,i);
            PMem_BeginAlloc(retainedNames[i].data(),type,TRACK_FASTFILE);
            PMem_EndAlloc(retainedNames[i].data(),type);
        }
        limits.allocType=type;
        require(zone.Open(argv[1],"native-production-first-zone",inflate,limits,page,8*1024*1024)==NativeZoneResult::Ok,"32nd native zone owner supported");
        NativeZoneFile extra;
        require(extra.Open(argv[1],"native-over-capacity",inflate,limits,page,8*1024*1024)==NativeZoneResult::AllocationFailed
            && extra.AllocationStatus()==ZoneResult::BeginFailed,"33rd owner refused before PMem mutation");
        require(!extra.IsOpen()&&extra.Decoded().decoded.empty()&&g_mem.prim[type].allocListCount==32,"capacity refusal preserves all owners");
        require(zone.Close()==NativeZoneResult::Ok&&g_mem.prim[type].allocListCount==31,"32nd owner removed independently");
        for(unsigned i=31;i;--i) PMem_Free(retainedNames[i-1].data());
        require(!g_mem.prim[0].allocListCount&&!g_mem.prim[1].allocListCount&&!g_mem.prim[0].pos&&g_mem.prim[1].pos==g_mem.size,"maximum owner stack restores both baselines");
    }
#endif
    require(zone.Open(argv[1],"native-production-first-zone",inflate,limits,page,65535)==NativeZoneResult::DecodeFailed,"budget rejected before codec allocation");
    require(!zone.IsOpen()&&zone.Decoded().decoded.empty(),"failed decode leaves no owner or output");
    for(unsigned cycle=0;cycle<4;++cycle) {
        limits.allocType=cycle%2;
        require(zone.Open(argv[1],"native-production-first-zone",inflate,limits,page,8*1024*1024)==NativeZoneResult::Ok,"full codec and actual named allocation");
        require(zone.Decoded().decoded.size()==150108&&zone.Decoded().paddingBytes==27,"genuine complete first startup zone");
        require(zone.Decoded().peakWorkingBytes<=8*1024*1024&&zone.Decoded().peakWorkingBytes>65536,"codec peak recorded under budget");
        require(zone.Open(argv[1],"other",inflate,limits,page,8*1024*1024)==NativeZoneResult::Occupied,"live owner cannot reopen");
        DB_InitStreams(zone.Blocks());
        require(zone.Close()==NativeZoneResult::StreamsAttached,"attached production streams retain owner");
        XBlock copied[7]; std::memcpy(copied,zone.Blocks(),sizeof(copied)); DB_InitStreams(copied);
        require(zone.Close()==NativeZoneResult::StreamsAttached,"copied stream descriptors retain owner");
        std::array<unsigned char,4096> bytes{}; bytes.fill(0xa5);
        require(!zone.Read(bytes.data(),bytes.size(),4097)&&!zone.Position()&&bytes[0]==0xa5,"short capacity leaves source and destination untouched");
        uLong checksum=1;
        while(zone.Remaining()) { const auto n=static_cast<int>((std::min)(zone.Remaining(),bytes.size())); require(zone.Read(bytes.data(),bytes.size(),n),"owned full decoded sequential reads"); checksum=adler32(checksum,bytes.data(),n); }
        require(zone.Position()==150108&&!zone.Read(bytes.data(),bytes.size(),1),"exact complete source consumption");
        std::printf("{\"native_zone_file\":{\"cycle\":%u,\"decoded\":%zu,\"adler32\":%lu,\"codec_peak\":%llu,\"owner_capacity\":%llu,\"padding\":%llu}}\n",cycle,zone.Position(),static_cast<unsigned long>(checksum),static_cast<unsigned long long>(zone.Decoded().peakWorkingBytes),static_cast<unsigned long long>(zone.Plan().firstFitCapacity),static_cast<unsigned long long>(zone.Decoded().paddingBytes));
        DB_InitStreams(detached); require(zone.Close()==NativeZoneResult::Ok&&!zone.IsOpen()&&!zone.Remaining(),"detached actual free and owner release");
#if defined(__ANDROID__)
        require(!bo1::android::ZoneMemoryCount()&&!bo1::android::ZoneMemoryRequestedBytes(),"Android actual ownership baseline");
#else
        require(!g_mem.prim[0].allocListCount&&!g_mem.prim[1].allocListCount&&!g_mem.prim[0].pos&&g_mem.prim[1].pos==g_mem.size,"Windows actual PMem baseline");
#endif
    }
    return 0;
}
