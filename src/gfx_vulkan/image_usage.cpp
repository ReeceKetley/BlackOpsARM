#include "image_usage.h"
namespace bo1::vulkan::image_usage {
namespace {thread_local RecordingScope* current{};
thread_local std::uint64_t samplingLookups{};
bool Transfer(VkCommandBuffer,const std::shared_ptr<Texture2D>&,const std::shared_ptr<Texture2D>&,const VkImageCopy*,unsigned,std::string&);
VkImageLayout Final(const Texture2D& image) {
 const auto& d=image.Description();
 if(d.aspects&VK_IMAGE_ASPECT_DEPTH_BIT)return d.usage&VK_IMAGE_USAGE_SAMPLED_BIT?VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
 return d.usage&VK_IMAGE_USAGE_SAMPLED_BIT?VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
}}
RecordingScope::RecordingScope(Tracker& t,VkCommandBuffer c):tracker(t),command(c),previous_(current){current=this;}
RecordingScope::~RecordingScope(){current=previous_;}
const RecordingScope* CurrentScope(){return current;}
BorrowScope::BorrowScope(const RecordingScope* owner):previous_(current){current=const_cast<RecordingScope*>(owner);}
BorrowScope::~BorrowScope(){current=previous_;}
State RecordingScope::Lookup(const std::shared_ptr<Texture2D>& image)const {
 // The caller holds `image`, so a live entry at its address is this image; an
 // entry for a destroyed image at a reused address has expired.
 auto p=pending.find(image.get());if(p!=pending.end()&&!p->second.image.expired())return p->second;
 auto s=tracker.submitted.find(image.get());if(s!=tracker.submitted.end()&&!s->second.image.expired())return s->second;
 State empty;empty.image=image;return empty;
}
bool RecordingScope::Validate(std::string& error)const {
 for(const auto& item:pending)if(item.second.active){error="Native image pass remained active at submit";return false;}return true;
}
void RecordingScope::Commit() {
 for(auto it=tracker.submitted.begin();it!=tracker.submitted.end();)if(it->second.image.expired())it=tracker.submitted.erase(it);else ++it;
 for(const auto& item:pending)if(!item.second.image.expired())tracker.submitted[item.first]=item.second;
}
bool Begin(VkCommandBuffer command,const std::shared_ptr<Texture2D>& color,const std::shared_ptr<Texture2D>& depth,bool loadColor,bool loadDepth,std::string& error) {
 if(!current||current->command!=command){error="Native attachment has no matching image recording scope";return false;}
 for(const auto& image:{color,depth})if(image){auto state=current->Lookup(image);
  if(state.active||((image==color?loadColor:loadDepth)&&(!state.contents||state.layout!=Final(*image)))){error="Native attachment LOAD requires preserved submitted or earlier-pass contents";return false;}}
 for(const auto& image:{color,depth})if(image){auto state=current->Lookup(image);state.active=true;state.contents=false;current->pending[image.get()]=state;}return true;
}
bool End(VkCommandBuffer command,const std::shared_ptr<Texture2D>& color,const std::shared_ptr<Texture2D>& depth,bool preserveDepth,std::string& error) {
 if(!current||current->command!=command){error="Native attachment end has no matching scope";return false;}
 for(const auto& image:{color,depth})if(image&&!current->Lookup(image).active){error="Native attachment end has no active producer";return false;}
 for(const auto& image:{color,depth})if(image){auto state=current->Lookup(image);state.active=false;state.layout=Final(*image);state.contents=image==color||preserveDepth;current->pending[image.get()]=state;}return true;
}
bool CanLoad(const std::shared_ptr<Texture2D>& image){if(!current||!image)return false;auto s=current->Lookup(image);return !s.active&&s.contents&&s.layout==Final(*image);}
bool Known(const std::shared_ptr<Texture2D>& image){if(!current||!image)return false;++samplingLookups;const auto state=current->Lookup(image);return state.active||state.layout!=VK_IMAGE_LAYOUT_UNDEFINED;}
bool Copy(VkCommandBuffer command,const std::shared_ptr<Texture2D>& source,const std::shared_ptr<Texture2D>& destination,std::string& error,VkImageAspectFlags copyAspects) {
 if(!current||current->command!=command||!source||!destination||source->Handle()==destination->Handle()
     ||!source->Handle()||!destination->Handle()||!source->Device()||source->Device()!=destination->Device()) {error="Native image copy needs distinct images and its recording scope";return false;}
 const auto& a=source->Description();const auto& b=destination->Description();
 if(a.aspects!=b.aspects||(a.aspects!=VK_IMAGE_ASPECT_COLOR_BIT&&a.aspects!=VK_IMAGE_ASPECT_DEPTH_BIT&&a.aspects!=(VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT))
     ||a.viewType!=VK_IMAGE_VIEW_TYPE_2D||b.viewType!=VK_IMAGE_VIEW_TYPE_2D||a.mipLevels!=1||b.mipLevels!=1
     ||a.arrayLayers!=1||b.arrayLayers!=1||a.format!=b.format||a.extent.width!=b.extent.width
     ||a.extent.height!=b.extent.height||a.extent.depth!=1||b.extent.depth!=1
     ||!(a.usage&VK_IMAGE_USAGE_TRANSFER_SRC_BIT)||!(b.usage&VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
  error="Native image copy requires matching single-mip 2D colour/depth formats/extents and transfer usage";return false;}
 VkImageCopy copies[2]{};unsigned count=0;
 if(copyAspects&&(copyAspects&~a.aspects)) {error="Native image copy requests an absent aspect";return false;}
 for(const auto aspect:{VK_IMAGE_ASPECT_COLOR_BIT,VK_IMAGE_ASPECT_DEPTH_BIT,VK_IMAGE_ASPECT_STENCIL_BIT})if(a.aspects&aspect&&(!copyAspects||(copyAspects&aspect))) {
  auto& copy=copies[count++];copy.srcSubresource=copy.dstSubresource={VkImageAspectFlags(aspect),0,0,1};copy.extent=a.extent;
 }
 return Transfer(command,source,destination,copies,count,error);
}
bool CopyRegion(VkCommandBuffer command,const std::shared_ptr<Texture2D>& source,VkOffset2D sourceOffset,const std::shared_ptr<Texture2D>& destination,
    VkOffset2D destinationOffset,VkExtent2D extent,VkImageAspectFlags copyAspects,std::string& error) {
 if(!current||current->command!=command||!source||!destination||source->Handle()==destination->Handle()
     ||!source->Handle()||!destination->Handle()||!source->Device()||source->Device()!=destination->Device()) {error="Native image region copy needs distinct images and its recording scope";return false;}
 const auto& a=source->Description();const auto& b=destination->Description();
 const auto inside=[](const VkExtent3D& size,VkOffset2D offset,VkExtent2D rect){
  return offset.x>=0&&offset.y>=0&&rect.width&&rect.height&&unsigned(offset.x)<=size.width&&unsigned(offset.y)<=size.height
   &&rect.width<=size.width-unsigned(offset.x)&&rect.height<=size.height-unsigned(offset.y);};
 if(a.aspects!=b.aspects||a.format!=b.format||!copyAspects||(copyAspects&~a.aspects)
     ||a.viewType!=VK_IMAGE_VIEW_TYPE_2D||b.viewType!=VK_IMAGE_VIEW_TYPE_2D||a.mipLevels!=1||b.mipLevels!=1||a.arrayLayers!=1||b.arrayLayers!=1
     ||!inside(a.extent,sourceOffset,extent)||!inside(b.extent,destinationOffset,extent)
     ||!(a.usage&VK_IMAGE_USAGE_TRANSFER_SRC_BIT)||!(b.usage&VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
  error="Native image region copy requires matching single-mip 2D formats, in-bounds rectangles and transfer usage";return false;}
 VkImageCopy copies[2]{};unsigned count=0;
 for(const auto aspect:{VK_IMAGE_ASPECT_COLOR_BIT,VK_IMAGE_ASPECT_DEPTH_BIT,VK_IMAGE_ASPECT_STENCIL_BIT})if(copyAspects&aspect) {
  auto& copy=copies[count++];copy.srcSubresource=copy.dstSubresource={VkImageAspectFlags(aspect),0,0,1};
  copy.srcOffset={sourceOffset.x,sourceOffset.y,0};copy.dstOffset={destinationOffset.x,destinationOffset.y,0};copy.extent={extent.width,extent.height,1};
 }
 return Transfer(command,source,destination,copies,count,error);
}
namespace {
bool Transfer(VkCommandBuffer command,const std::shared_ptr<Texture2D>& source,const std::shared_ptr<Texture2D>& destination,const VkImageCopy* copies,unsigned count,std::string& error) {
 const auto& a=source->Description();
 auto src=current->Lookup(source),dst=current->Lookup(destination);
 if(src.active||dst.active||!src.contents||src.layout!=Final(*source)
     ||(dst.layout!=VK_IMAGE_LAYOUT_UNDEFINED&&dst.layout!=Final(*destination))) {
  error="Native image copy requires ended, preserved source contents and an inactive destination";return false;}
 const VkPipelineStageFlags producers=VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_VERTEX_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_TRANSFER_BIT;
 const VkAccessFlags previousAccess=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_TRANSFER_READ_BIT|VK_ACCESS_TRANSFER_WRITE_BIT;
 VkImageMemoryBarrier barriers[2]{};
 for(unsigned i=0;i<2;++i){auto& barrier=barriers[i];barrier.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;barrier.subresourceRange={a.aspects,0,1,0,1};}
 barriers[0].image=source->Handle();barriers[0].oldLayout=src.layout;barriers[0].newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;barriers[0].srcAccessMask=previousAccess;barriers[0].dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
 barriers[1].image=destination->Handle();barriers[1].oldLayout=dst.layout;barriers[1].newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barriers[1].srcAccessMask=dst.layout==VK_IMAGE_LAYOUT_UNDEFINED?0:previousAccess;barriers[1].dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
 vkCmdPipelineBarrier(command,producers,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,2,barriers);
 vkCmdCopyImage(command,source->Handle(),VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,destination->Handle(),VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,count,copies);
 barriers[0].oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;barriers[0].newLayout=Final(*source);barriers[0].srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
 barriers[1].oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barriers[1].newLayout=Final(*destination);barriers[1].srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
 for(auto& barrier:barriers)barrier.dstAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT|VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
 vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,producers,0,0,nullptr,0,nullptr,2,barriers);
 src.layout=Final(*source);dst.layout=Final(*destination);dst.contents=true;
 current->pending[source.get()]=src;current->pending[destination.get()]=dst;return true;
}
}
std::uint64_t SamplingLookupCount(){return samplingLookups;}
bool ResolveSampling(const std::shared_ptr<Texture2D>& image,bool requireTracked,VkImageLayout& layout,bool& known,std::string& error) {
 known=false;State state;
 if(current&&image){++samplingLookups;state=current->Lookup(image);known=state.active||state.layout!=VK_IMAGE_LAYOUT_UNDEFINED;}
 if(!known&&!requireTracked){layout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;return true;}
 if(!current||!image){error="Native generated image sampling needs a recording scope";return false;}
 if(state.active||!state.contents||!(image->Description().usage&VK_IMAGE_USAGE_SAMPLED_BIT)||state.layout!=Final(*image)) {
  error="Native sampled image has no preserved ordered contents or is an active attachment active="+std::to_string(state.active)
   +" contents="+std::to_string(state.contents)+" layout="+std::to_string(unsigned(state.layout))+" expected="+std::to_string(unsigned(Final(*image)))
   +" sampled="+std::to_string(bool(image->Description().usage&VK_IMAGE_USAGE_SAMPLED_BIT));return false;}
 layout=state.layout;image->NoteRead();return true;
}
bool Sample(const std::shared_ptr<Texture2D>& image,VkImageLayout& layout,std::string& error){bool known;return ResolveSampling(image,true,layout,known,error);}
}
