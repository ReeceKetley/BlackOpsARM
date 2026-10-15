#pragma once
#include <vulkan/vulkan.h>
#include <cstdint>
#include <vector>

namespace bo1::vulkan {
// Conservative Vulkan 1.1 render-pass compatibility identity. Keep attachment
// indices/counts and dependencies exact; this may miss legal reuse but cannot
// merge incompatible topology. Only load/store operations and image layouts
// are omitted. Unknown extension chains (including multiview) disable reuse.
// https://docs.vulkan.org/spec/latest/chapters/renderpass.html#renderpass-compatibility
inline bool RenderPassCompatibility(const VkRenderPassCreateInfo& info,std::vector<std::uint32_t>& words) {
    words.clear();
    if(info.sType!=VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO||info.pNext
        ||(info.attachmentCount&&!info.pAttachments)||!info.subpassCount||!info.pSubpasses
        ||(info.dependencyCount&&!info.pDependencies))return false;
    words={info.flags,info.attachmentCount,info.subpassCount,info.dependencyCount};
    for(unsigned i=0;i<info.attachmentCount;++i) {
        const auto& a=info.pAttachments[i];
        words.insert(words.end(),{a.flags,unsigned(a.format),unsigned(a.samples)});
    }
    const auto references=[&](const VkAttachmentReference* refs,unsigned count) {
        if(count&&!refs)return false;
        for(unsigned i=0;i<count;++i) {
            if(refs[i].attachment!=VK_ATTACHMENT_UNUSED&&refs[i].attachment>=info.attachmentCount)return false;
            words.push_back(refs[i].attachment);
        }
        return true;
    };
    for(unsigned i=0;i<info.subpassCount;++i) {
        const auto& s=info.pSubpasses[i];
        words.insert(words.end(),{s.flags,unsigned(s.pipelineBindPoint),s.inputAttachmentCount,s.colorAttachmentCount,
            unsigned(s.pResolveAttachments!=nullptr),unsigned(s.pDepthStencilAttachment!=nullptr),s.preserveAttachmentCount});
        if(!references(s.pInputAttachments,s.inputAttachmentCount)||!references(s.pColorAttachments,s.colorAttachmentCount)
            ||(s.pResolveAttachments&&!references(s.pResolveAttachments,s.colorAttachmentCount))
            ||(s.pDepthStencilAttachment&&!references(s.pDepthStencilAttachment,1))
            ||(s.preserveAttachmentCount&&!s.pPreserveAttachments)){words.clear();return false;}
        for(unsigned j=0;j<s.preserveAttachmentCount;++j) {
            if(s.pPreserveAttachments[j]>=info.attachmentCount){words.clear();return false;}
            words.push_back(s.pPreserveAttachments[j]);
        }
    }
    for(unsigned i=0;i<info.dependencyCount;++i) {
        const auto& d=info.pDependencies[i];
        words.insert(words.end(),{d.srcSubpass,d.dstSubpass,d.srcStageMask,d.dstStageMask,
            d.srcAccessMask,d.dstAccessMask,d.dependencyFlags});
    }
    return true;
}
}
