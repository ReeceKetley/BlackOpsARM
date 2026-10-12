// Standalone CRT service research. No engine, FastFile allocation or game bytes.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>
#include <vector>

static std::vector<std::int32_t> hashes;
static std::uint32_t columns;
static int retailSignedCompare(const void* a, const void* b) {
    const auto left = *static_cast<const std::int16_t*>(a);
    const auto right = *static_cast<const std::int16_t*>(b);
    if (hashes[left] != hashes[right]) return hashes[left] < hashes[right] ? -1 : 1;
    return left % static_cast<int>(columns) - right % static_cast<int>(columns);
}
static std::uint32_t word32(const std::vector<unsigned char>& b, std::size_t offset) {
    return std::uint32_t(b[offset]) | std::uint32_t(b[offset+1]) << 8 |
           std::uint32_t(b[offset+2]) << 16 | std::uint32_t(b[offset+3]) << 24;
}
int main(int argc, char** argv) {
    if (argc != 2) { std::fprintf(stderr,"usage: crt_sort INPUT.numeric\n"); return 2; }
    std::ifstream input(argv[1],std::ios::binary);
    if (!input) { std::fprintf(stderr,"unable to read numeric metadata\n"); return 2; }
    const std::vector<unsigned char> bytes{std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
    if (bytes.size() < 12 || std::memcmp(bytes.data(),"STQP",4)) return 3;
    columns = word32(bytes,4); const auto count = word32(bytes,8);
    if (!columns || !count || columns > count || count % columns || count > 32768 || bytes.size() != 12 + 6*std::size_t(count)) return 3;
    hashes.resize(count); std::vector<std::int16_t> indices(count); std::vector<bool> seen(count);
    for (std::uint32_t i=0;i<count;++i) { const auto bits=word32(bytes,12+4*i);std::memcpy(&hashes[i],&bits,4); }
    for (std::uint32_t i=0;i<count;++i) {
        const auto offset=12+4*count+2*i; const auto value=unsigned(bytes[offset])|unsigned(bytes[offset+1])<<8;
        if (value >= count || seen[value]) return 3;seen[value]=true;indices[i]=static_cast<std::int16_t>(value);
    }
    const auto address = reinterpret_cast<std::uintptr_t>(indices.data());
    std::qsort(indices.data(),indices.size(),sizeof(indices[0]),retailSignedCompare);
    for (std::size_t i=1;i<indices.size();++i) if (retailSignedCompare(&indices[i-1],&indices[i]) > 0) return 4;
    std::printf("{\"schema\":\"BlackOpsARM.CRT-sort-probe.v1\",\"pointer_bytes\":%zu,\"column_count\":%u,\"cell_count\":%u,\"array_address\":%llu,\"array_above_u32\":%s,\"element_bytes\":2,\"signed_hash_compare\":true,\"equal_key_returns_zero\":true,\"actual_std_qsort\":true,\"sorted_indices\":[",sizeof(void*),columns,count,static_cast<unsigned long long>(address),address>std::numeric_limits<std::uint32_t>::max()?"true":"false");
    for (std::size_t i=0;i<indices.size();++i) std::printf("%s%d",i?",":"",static_cast<int>(indices[i]));
    std::printf("]}\n");return 0;
}
