#pragma once
#include <qcommon/common.h>
#include <cstdio>
#include <cstdlib>

namespace bo1::android {
// Preserve the secure formatter's real destination capacity on the native ABI.
// A diagnostic record must not continue with a partial or failed format.
template <typename... Arguments>
int FormatChecked(char *buffer,std::size_t capacity,const char *format,Arguments... arguments) {
    if (!buffer || !capacity || !format) {
        Com_Error(ERR_FATAL,"Native bounded formatting requires a valid destination and format");
        std::abort();
    }
    const int written=std::snprintf(buffer,capacity,format,arguments...);
    if (written<0 || static_cast<std::size_t>(written)>=capacity) {
        buffer[0]=0;
        Com_Error(ERR_FATAL,"Native formatting exceeded its actual destination capacity");
        std::abort();
    }
    return written;
}
template <std::size_t Capacity,typename... Arguments>
int FormatChecked(char (&buffer)[Capacity],const char *format,Arguments... arguments) {
    return FormatChecked(buffer,Capacity,format,arguments...);
}
}
