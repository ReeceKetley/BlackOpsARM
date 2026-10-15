#include "pipeline_key.h"
#include "texture_bindings.h"
#include <array>
#include <cstdio>
#include <stdexcept>
#include <chrono>
#include <limits>
using namespace bo1::vulkan;
static unsigned checks;
void Check(bool ok){++checks;if(!ok)throw std::runtime_error("key/sampler invariant failed");}
int main(){try {
 std::string error;std::array<unsigned char,80> a{},b{};for(unsigned i=0;i<a.size();++i)a[i]=b[i]=unsigned(i*73);
 for(unsigned offset=0;offset<8;++offset)for(unsigned size=0;size<=64;++size){
  ProgramDescription d;d.vertexBytecode=a.data()+offset;d.vertexBytes=size;d.pixelBytecode=b.data()+offset;d.pixelBytes=size;
  PipelineKey k;Check(PipelineKey::Create(d,k,error));
  std::array<unsigned char,80> copy=a;d.vertexBytecode=copy.data()+offset;
  PipelineKey same;Check(PipelineKey::Create(d,same,error)&&same==k&&same.Hash()==k.Hash());
  if(size){copy[offset+size-1]^=1;PipelineKey other;Check(PipelineKey::Create(d,other,error)&&other!=k);}
  d.vertexBytes=size+1;PipelineKey length;Check(PipelineKey::Create(d,length,error)&&length!=k);
 }
 ProgramDescription d;PipelineKey original;Check(PipelineKey::Create(d,original,error));
 d.vertexUbyteMask=4;PipelineKey changed;Check(PipelineKey::Create(d,changed,error)&&changed!=original);
 d={};d.pixelDepthSamplerMask=8;Check(PipelineKey::Create(d,changed,error)&&changed!=original);
 d={};d.alphaTest.reference=-0.0f;Check(PipelineKey::Create(d,changed,error)&&changed!=original);
 d={};d.vertexBytes=1;Check(!PipelineKey::Create(d,changed,error));
 unsigned char ownerA{},ownerB{};auto first=reinterpret_cast<const GfxCmdBufState*>(&ownerA),second=reinterpret_cast<const GfxCmdBufState*>(&ownerB);
 const unsigned decoded=(3u<<20)|(3u<<22)|(3u<<24)|(2u<<8)|(2u<<12)|(2u<<16);
 Check(texture_bindings::Update(first,15,decoded,.5f,error));auto before=texture_bindings::Acquire(first);
 Check(before&&!before->slots[15].image&&!before->slots[15].sampler&&before->slots[15].decoded==decoded&&before->slots[15].bias==.5f);
 texture_bindings::Clone(first,second);Check(texture_bindings::Acquire(second)==before);
 Check(texture_bindings::Update(first,15,decoded,-.5f,error));Check(before->slots[15].bias==.5f);
 Check(!texture_bindings::Update(first,16,decoded,0,error));Check(!texture_bindings::Update(first,14,0,0,error));
 Check(!texture_bindings::Update(first,14,decoded,std::numeric_limits<float>::infinity(),error));
 Check(!texture_bindings::Acquire(first)->slots[14].image&&!texture_bindings::Acquire(first)->slots[14].sampler);
 texture_bindings::Disable(first,15);Check(!texture_bindings::Acquire(first)->slots[15].image);Check(texture_bindings::Acquire(second)->slots[15].bias==.5f);
 texture_bindings::Release(first);texture_bindings::Release(second);
 std::array<unsigned char,8192> code{};for(unsigned i=0;i<code.size();++i)code[i]=unsigned(i*17);d={};d.vertexBytecode=code.data()+1;d.vertexBytes=2045;d.pixelBytecode=code.data()+3;d.pixelBytes=4093;
 auto start=std::chrono::steady_clock::now();std::size_t hash=0;
 for(unsigned i=0;i<10000;++i){d.subpass=i&1;PipelineKey key;Check(PipelineKey::Create(d,key,error));hash^=key.Hash();}
 const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
 std::printf("native_key_sampler_cpu=pass checks=%u key_iterations=10000 key_ms=%.3f hash=%zu no_gpu_fixture=1\n",checks,ms,hash);return 0;
 }catch(const std::exception& e){std::printf("FAIL %s checks=%u\n",e.what(),checks);return 1;}}
