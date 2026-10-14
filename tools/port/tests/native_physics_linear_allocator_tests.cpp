#include <physics/phys_mem_new.h>
#include <algorithm>
#include <array>
#include <thread>
#include <vector>
#include <cstdio>
#include <cstdlib>
static void check(bool value,const char *reason) {
    if(!value) { std::fprintf(stderr,"FAIL %s\n",reason); std::abort(); }
}
int main() {
    std::vector<char> arena(32769);
    phys_memory_manager manager(arena.data()+1,32768);
    for(auto *entry:manager.m_slot_pool_map.m_hash_table) check(!entry,"all native map pointers initialized");
    auto *first=static_cast<char *>(manager.allocate(5,64));
    check(first && !(reinterpret_cast<uintptr_t>(first)%64),"aligned arbitrary-size allocation");
    std::array<std::array<char *,128>,4> allocated{};
    std::vector<std::thread> workers;
    for(int worker=0;worker<4;++worker) workers.emplace_back([&,worker] {
        for(auto &slot:allocated[worker]) slot=static_cast<char *>(manager.allocate(32,16));
    });
    for(auto &worker:workers) worker.join();
    std::vector<char *> ordered;
    for(auto &batch:allocated) for(auto *slot:batch) {
        check(slot && !(reinterpret_cast<uintptr_t>(slot)%16),"concurrent alignment");
        check(slot>=first+5 && slot+32<=manager.m_buffer_end,"arena bounds"); ordered.push_back(slot);
    }
    std::sort(ordered.begin(),ordered.end());
    for(size_t i=1;i<ordered.size();++i) check(ordered[i]>=ordered[i-1]+32,"non-overlapping concurrent owners");
    char *cursor=manager.m_buffer_cur;
    check(!manager.allocate(16,0) && !manager.allocate(16,3) && manager.m_buffer_cur==cursor,"invalid alignment does not advance cursor");
    check(!manager.allocate(~0u,16) && manager.m_buffer_cur==cursor,"oversized request does not advance cursor");
    const auto remaining=static_cast<unsigned int>(manager.m_buffer_end-manager.m_buffer_cur);
    check(manager.allocate(remaining,1)==cursor && manager.m_buffer_cur==manager.m_buffer_end,"exact remaining capacity");
    check(!manager.allocate(1,1) && manager.m_buffer_cur==manager.m_buffer_end,"exhaustion retains cursor");
    std::puts("PASS original physics linear allocator: native map initialization, 512 concurrent owners, alignment and exhaustion");
}
