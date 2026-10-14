#pragma once
// Native terminal allocation failure. Matches the canonical declaration/ABI.
// Uses no engine/stdio locks or heap allocation, and never publishes clean exit.
void Sys_OutOfMemErrorInternal(const char *filename,int line);
#include <cstdint>
namespace bo1::android {
// Actual failure-only caller quantities; deficitKnown=false reports unavailable.
[[noreturn]] void ReportAllocationFailure(const char *filename,int line,const char *operation,const char *allocationName,std::int64_t requestedBytes,std::int64_t deficitBytes,bool deficitKnown);
}
