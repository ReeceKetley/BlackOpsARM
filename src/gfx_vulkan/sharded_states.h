#pragma once
#include "pointer_hash.h"
#include <array>
#include <cstdint>
#include <mutex>
#include <unordered_map>
namespace bo1::vulkan {
// Per-command-state registry split into independently locked shards, so
// threads recording with different command states (parallel scene recording)
// do not serialize on one lock. Each key always maps to the same shard; an
// operation on one key takes only that shard's lock. Copy-on-write owners
// keep their rule: a value shared by several keys has use_count > 1.
template<class Key,class Value>
class ShardedStates {
public:
    struct Shard {std::mutex mutex;std::unordered_map<Key,Value,PointerHash> map;};
    Shard& For(Key key) {
        const auto value=std::uint64_t(reinterpret_cast<std::uintptr_t>(key))*0x9e3779b97f4a7c15ull;
        return shards_[std::size_t(value>>59)];
    }
private:
    std::array<Shard,32> shards_;
};
}
