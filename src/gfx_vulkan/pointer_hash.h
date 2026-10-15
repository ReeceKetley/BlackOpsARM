#pragma once
#include <cstddef>
#include <cstdint>
namespace bo1::vulkan {
// libc++ hashes pointer keys with CityHash; these registries are looked up on
// every draw. Addresses are already unique and libc++ buckets by a prime
// modulus, so a cheap mix of the address is sufficient.
struct PointerHash {
    std::size_t operator()(const void* pointer)const noexcept {
        const auto value=reinterpret_cast<std::uintptr_t>(pointer);
        return std::size_t(value^(value>>17));
    }
};
}
