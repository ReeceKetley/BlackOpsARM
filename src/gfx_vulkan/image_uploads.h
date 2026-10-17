#pragma once
#include "image_assets.h"
namespace bo1::vulkan::image_uploads {
void Queue(std::shared_ptr<const image_assets::Record>);
// Submit snapshots on the renderer queue; retain owners until real completion.
struct FlushProfile {
    bool sampled{},success{};
    std::uint64_t sequence{},totalNs{},retireNs{},validationNs{},stagingNs{},packNs{},beginNs{},recordNs{},submitNs{},stagedBytes{},boxBytes{};
    unsigned images{},batches{},stagingAllocations{},stagingReuses{},regionCount{},boxFullFallbacks{};
};
// Render-thread last flush; unsampled calls perform no phase clock reads.
FlushProfile ReadFlushProfile();
bool Flush(std::string&,bool timed=false);
// Ready for subsequent GPU consumption on that same queue, not CPU readback.
bool Ready(const std::shared_ptr<const image_assets::Record>&);
// Caller has retired GPU users; pending CPU snapshots may then be discarded.
bool Clear(std::string&);
}
