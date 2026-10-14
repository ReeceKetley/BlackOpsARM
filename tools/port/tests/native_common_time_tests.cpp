#include <universal/q_shared.h>
#include <universal/com_shared.h>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <thread>
#include <atomic>

int main() {
    if (::setenv("TZ", "UTC0", 1)) return 1;
    ::tzset();
    const qtime_s epoch{0,0,0,1,0,70,4,0,0};
    const qtime_s leap{0,0,0,29,1,100,2,59,0};
    const qtime_s limit{7,14,3,19,0,138,2,18,0};
    qtime_s value{};
    Com_GetQTime(0, &value, false);
    if (std::memcmp(&value,&epoch,sizeof(value))) return 2;
    Com_GetQTime(951782400, &value, false);
    if (std::memcmp(&value,&leap,sizeof(value))) return 3;
    Com_GetQTime(INT32_MAX, &value, false);
    if (std::memcmp(&value,&limit,sizeof(value))) return 4;
    Com_GetQTime(951782400, &value, true);
    if (std::memcmp(&value,&leap,sizeof(value))) return 5;
    errno=0;
    Com_GetQTime(-1,&value,false);
    if (errno!=EINVAL || std::memcmp(&value,&leap,sizeof(value))) return 6;
    errno=0;
    Com_GetQTime(INT32_MIN,&value,true);
    if (errno!=EINVAL || std::memcmp(&value,&leap,sizeof(value))) return 7;
    Com_GetQTime(0,nullptr,true);
    if (::setenv("TZ", "EST5", 1)) return 8;
    ::tzset();
    const qtime_s offset{0,0,19,28,1,100,1,58,0};
    Com_GetQTime(951782400,&value,true);
    if (std::memcmp(&value,&offset,sizeof(value))) return 9;
    std::atomic<bool> good{true};
    std::thread workers[4];
    for (auto &worker:workers) worker=std::thread([&] {
        for(int i=0;i<1024;++i) {
            qtime_s utc{},local{};
            Com_GetQTime(951782400,&utc,false);
            Com_GetQTime(951782400,&local,true);
            if (std::memcmp(&utc,&leap,sizeof(utc)) ||
                std::memcmp(&local,&offset,sizeof(local))) good=false;
        }
    });
    for(auto &worker:workers) worker.join();
    if (!good) return 10;
    const auto before=std::time(nullptr);
    const int actual=Com_RealTime(&value,false);
    const auto after=std::time(nullptr);
    if(actual<before || actual>after) return 11;
    std::printf("actual common time: UTC/leap/int32-limit/local-offset/invalid/8192 concurrent conversions/live clock passed\n");
}
