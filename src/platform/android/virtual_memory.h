#pragma once
#include <cstddef>
namespace bo1::android {
std::size_t VirtualPageSize();
// Diagnostic count for ownership tests; no address/low-VA policy is imposed.
std::size_t VirtualReservationCount();
}
