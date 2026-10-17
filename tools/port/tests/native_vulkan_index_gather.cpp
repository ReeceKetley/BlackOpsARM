#include "gfx_vulkan/index_gather.h"
#include <cstdio>
#include <limits>
#include <stdexcept>
using namespace bo1::vulkan;
namespace {unsigned checks;void Check(bool ok){++checks;if(!ok)throw std::runtime_error("index gather invariant");}}
int main(){try {
    const std::uint16_t a[]{7,3,4},b[]{65535,0,100},c[]{40,41,42};
    const IndexSpan spans[]{{a,3},{b,3},{c,3}};
    std::vector<std::uint16_t> output{999};unsigned vertices=99;
    Check(GatherIndices(spans,3,9,output,vertices));
    Check(output==std::vector<std::uint16_t>({7,3,4,65535,0,100,40,41,42})&&vertices==65536);
    const auto original=output;const unsigned before=vertices;
    Check(!GatherIndices(spans,3,8,output,vertices));
    Check(output==original&&vertices==before);
    Check(!GatherIndices(nullptr,3,9,output,vertices));
    Check(!GatherIndices(spans,0,9,output,vertices));
    Check(!GatherIndices(spans,3,0,output,vertices));
    const IndexSpan missing[]{{a,3},{nullptr,3}};
    Check(!GatherIndices(missing,2,9,output,vertices)&&output==original&&vertices==before);
    const IndexSpan empty[]{{a,3},{b,0}};
    Check(!GatherIndices(empty,2,9,output,vertices)&&output==original&&vertices==before);
    const IndexSpan overflow[]{{a,3},{b,std::numeric_limits<std::size_t>::max()}};
    Check(!GatherIndices(overflow,2,9,output,vertices)&&output==original&&vertices==before);
    Check(GatherIndices(spans+2,1,3,output,vertices)&&output==std::vector<std::uint16_t>({40,41,42})&&vertices==43);
    Check(GatherIndices(spans,1,3,output,vertices)&&vertices==8); // no stale upper bound
    std::printf("native_index_gather=PASS checks=%u exact_capacity=1 transactional_validation=1 ordered_spans=1\n",checks);return 0;
}catch(const std::exception& e){std::fprintf(stderr,"native_index_gather=FAIL %s check=%u\n",e.what(),checks);return 1;}}
