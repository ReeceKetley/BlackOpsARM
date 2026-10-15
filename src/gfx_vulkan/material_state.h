#pragma once
#include "graphics_program.h"
namespace bo1::vulkan {
// Decode canonical T5 state words into an actual native pipeline description.
// Does not publish partial output on unsupported state/capability requirements.
bool ApplyMaterialState(unsigned word0,unsigned word1,ProgramDescription&,std::string&);
}
