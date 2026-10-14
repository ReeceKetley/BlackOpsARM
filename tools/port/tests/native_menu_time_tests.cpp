#include <ui/l_precomp.h>
#include <ui/l_memory.h>
#include <platform/android/legacy_crt.h>
#include <platform/android/zone_heap.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cstdint>
#include <climits>
#include <initializer_list>
extern int numtokens;
static void require(bool condition, const char *message) {
    if (!condition) { std::fprintf(stderr, "FAIL %s\n", message); std::abort(); }
}
int main() {
    require(::setenv("TZ", "UTC0", 1) == 0, "test-local timezone"); ::tzset();
    char *epoch = bo1::crt::OwnedCtime64(0);
    require(epoch && !std::strcmp(epoch, "Thu Jan  1 00:00:00 1970\n"), "actual epoch format");
    char *later = bo1::crt::OwnedCtime64(86400);
    require(later && !std::strcmp(later, "Fri Jan  2 00:00:00 1970\n"), "next day");
    require(!std::strcmp(epoch, "Thu Jan  1 00:00:00 1970\n"), "independent owned storage");
    std::free(epoch); std::free(later);
    require(!bo1::crt::OwnedCtime64(LLONG_MAX), "unrepresentable year rejected");
    const auto allocations = bo1::android::ZoneAllocationCount();
    void *memory = GetMemory(17);
    require(memory && !(reinterpret_cast<std::uintptr_t>(memory) % alignof(token_s)), "native token alignment");
    std::memset(memory, 0x5a, 17); FreeMemory(static_cast<char *>(memory));
    require(bo1::android::ZoneAllocationCount() == allocations, "tagged allocation cleanup");
    require(!GetMemory(UINT_MAX), "size conversion overflow rejected");
    token_s input{}; input.line = 41; input.type = 4;
    char marker[] = "owner"; input.endwhitespace_p = marker; input.next = &input;
    token_s *cloned = PC_CopyToken(&input);
    require(cloned->next == nullptr && cloned->line == 41 && cloned->endwhitespace_p == marker, "typed token link preserves adjacent fields");
    PC_FreeToken(cloned);
    source_s source{};
    for (int builtin : {3,4}) {
        define_s define{}; define.builtin = builtin;
        token_s *first = nullptr, *last = nullptr;
        require(PC_ExpandBuiltinDefine(&source, &input, &define, &first, &last) == 1 && first && last == first, "original date/time expansion");
        require(first->string[0] == '"' && first->string[std::strlen(first->string) - 1] == '"', "quoted real macro");
        require(first->type == 4 && first->next == nullptr && first->line == 41, "macro token metadata");
        PC_FreeToken(first);
    }
    require(numtokens == 0 && bo1::android::ZoneAllocationCount() == allocations, "all parser resources released");
    std::puts("PASS owned real date/time, aligned menu allocation, typed token copy and original macro lifecycle");
}
