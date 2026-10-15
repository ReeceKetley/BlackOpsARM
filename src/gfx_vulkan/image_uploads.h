#pragma once
#include "image_assets.h"
namespace bo1::vulkan::image_uploads {
void Queue(std::shared_ptr<const image_assets::Record>);
// Submit snapshots on the renderer queue; retain owners until real completion.
bool Flush(std::string&);
// Ready for subsequent GPU consumption on that same queue, not CPU readback.
bool Ready(const std::shared_ptr<const image_assets::Record>&);
// Caller has retired GPU users; pending CPU snapshots may then be discarded.
bool Clear(std::string&);
}
