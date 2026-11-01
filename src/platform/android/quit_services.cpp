#include "quit_services.h"
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
namespace { std::atomic<bool> requested{}; }
void Sys_SetQuitRequested(){requested.store(true,std::memory_order_release);}
bool Sys_IsQuitRequested(){return requested.load(std::memory_order_acquire);}
[[noreturn]] void Sys_QuitWithLostDevice(int lostMilliseconds){
    // Preserve the reference's terminal escape from a blocked renderer shutdown.
    // Avoid engine/stdio locks and atexit handlers owned by the waiting main thread.
    char message[256];
    const int length=std::snprintf(message,sizeof(message),
        "Quit requested while the graphics device is lost (%d ms): terminating blocked renderer shutdown.\n",
        lostMilliseconds);
    if(length>0){
        std::size_t remaining=static_cast<std::size_t>(length)<sizeof(message)?length:sizeof(message)-1;
        const char *cursor=message;
        while(remaining){
            const auto written=write(STDERR_FILENO,cursor,remaining);
            if(written<0 && errno==EINTR)continue;
            if(written<=0)break;
            cursor+=written;remaining-=written;
        }
    }
    std::_Exit(0);
}
