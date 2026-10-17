#include "gfx_vulkan/material_bindings.h"
#include "gfx_d3d/rb_state.h"
#include "gfx_d3d/r_dvars.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
// Production Capture/selection map/constant_state are linked unchanged. Only
// asset discovery and empty geometry/textures are supplied by this CPU fixture.
using namespace bo1::vulkan;
namespace bo1::vulkan::shader_assets {
void TraceIntegrity(const std::shared_ptr<const Record>& record,const char*) {
    if(record&&record->integrityWatched){std::fputs("FAIL watched shader requires production integrity diagnostics\n",stderr);std::abort();}
}
bool Active(){return true;}
std::shared_ptr<const Record> Acquire(const MaterialVertexShader*){static auto r=std::make_shared<Record>();return r;}
std::shared_ptr<const Record> Acquire(const MaterialPixelShader*){static auto r=std::make_shared<Record>();return r;}
}
namespace bo1::vulkan::vertex_assets {
std::shared_ptr<const Record> Acquire(const MaterialVertexDeclaration*){static auto r=[] {auto r=std::make_shared<Record>();r->variants[0].available=true;return r;}();return r;}
}
namespace bo1::vulkan::geometry_bindings {
std::shared_ptr<const Snapshot> Acquire(const GfxCmdBufPrimState*){static auto r=std::make_shared<Snapshot>();return r;}
}
namespace bo1::vulkan::texture_bindings {
std::shared_ptr<const Snapshot> Acquire(const GfxCmdBufState*){return {};}
}
const dvar_s *r_polygonOffsetBias{},*r_polygonOffsetScale{},*sm_polygonOffsetBias{},*sm_polygonOffsetScale{};
static unsigned checks;
static void Check(bool value){++checks;if(!value)std::abort();}
int main(){
    auto state=std::make_unique<GfxCmdBufState>();std::string error;float first[4]={1,2,3,4},second[4]={5,6,7,8};
    Check(material_bindings::Vertex(state.get(),nullptr,error));Check(material_bindings::Pixel(state.get(),nullptr,error));
    Check(material_bindings::Declaration(state.get(),nullptr,0,error));
    Check(constant_state::Set(state.get(),ShaderStage::Vertex,0,1,first,error));
    const constant_state::Snapshot* address;
    {
        material_bindings::Draw captured;Check(material_bindings::Capture(state.get(),captured,error));address=captured.constants.get();
        Check(constant_state::Set(state.get(),ShaderStage::Vertex,0,1,second,error));
        Check(captured.constants->floats[0][0][0]==1&&constant_state::Acquire(state.get()).get()!=address);
    }
    {auto current=constant_state::Acquire(state.get());address=current.get();}
    Check(constant_state::Set(state.get(),ShaderStage::Vertex,0,1,first,error));Check(constant_state::Acquire(state.get()).get()==address);
    dvar_s bias{},slope{};bias.current.value=4;slope.current.value=2;r_polygonOffsetBias=&bias;r_polygonOffsetScale=&slope;
    state->activeStateBits[1]=0x10;material_bindings::Draw captured;
    Check(material_bindings::Capture(state.get(),captured,error));const auto oldBias=captured.polygonBias;
    bias.current.value=8;Check(material_bindings::Capture(state.get(),captured,error)&&captured.polygonBias!=oldBias);
    slope.current.value=std::numeric_limits<float>::infinity();Check(!material_bindings::Capture(state.get(),captured,error));
    slope.current.value=2;r_polygonOffsetBias=nullptr;Check(!material_bindings::Capture(state.get(),captured,error));
    state->activeStateBits[1]=0;Check(material_bindings::Capture(state.get(),captured,error)&&captured.polygonBias==0&&captured.polygonSlope==0);
    Check(!material_bindings::Capture(nullptr,captured,error)&&!captured.constants);
    material_bindings::Release(state.get());constant_state::Release(state.get());
    std::printf("native_material_capture=PASS checks=%u stack_capture_cow_retirement=1 bias_changed_nonfinite_missing_reset=1\n",checks);
}
