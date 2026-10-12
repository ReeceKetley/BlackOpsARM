#include "com_hash.h"
#include <cctype>
#include <cstdint>
#include <cstring>

int __cdecl Com_HashString(const char *fname, int len)
{
    static_assert(sizeof(int)==sizeof(std::uint32_t));
    if (!fname)
        return 0;
    std::uint32_t hash=5381;
    for (int i=0; fname[i] && (!len || i<len); ++i)
        hash=static_cast<std::uint32_t>(tolower(fname[i]))+33u*hash;
    // Preserve Windows signed result bits without signed-overflow arithmetic.
    int result;
    std::memcpy(&result,&hash,sizeof(result));
    return result;
}
