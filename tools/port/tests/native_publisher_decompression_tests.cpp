#include <live/live_storage_pub.h>
#include <server/sv_autoplaylist.h>
#include <zlib/zlib.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <csignal>
#include <sys/wait.h>
#include <unistd.h>
#include <array>
static char message[1024];
// Isolated logging/assertion boundaries. Production uses canonical engine owners.
void Com_PrintError(int, const char *fmt, ...) {
    va_list args; va_start(args, fmt);
    std::vsnprintf(message, sizeof(message), fmt, args); va_end(args);
}
bool Assert_MyHandler(const char *, int, int, const char *, ...) { std::abort(); }
static void Check(bool ok) { if (!ok) std::abort(); }
int main() {
    std::array<unsigned char, 32> payload{};
    for(unsigned int i=0;i<payload.size();++i) payload[i]=static_cast<unsigned char>(i*19);
    std::array<unsigned char, 256> compressed{};
    z_stream stream{};
    Check(deflateInit_(&stream, 6, "1.2.3", sizeof(stream)) == Z_OK);
    stream.next_in = payload.data(); stream.avail_in = payload.size();
    stream.next_out = compressed.data(); stream.avail_out = compressed.size();
    Check(deflate(&stream, Z_FINISH) == Z_STREAM_END);
    const auto compressedSize = stream.total_out;
    Check(deflateEnd(&stream) == Z_OK);
    std::array<unsigned char, 34> output; output.fill(0xA5);
    Check(LiveStorage_DecompressBuffer(compressed.data(), compressedSize, output.data()+1, 32));
    Check(!std::memcmp(output.data()+1,payload.data(),32));
    Check(output.front()==0xA5 && output.back()==0xA5);
    for(bool truncated : {false,true}) {
        const pid_t child=::fork(); Check(child>=0);
        if(!child) {
            LiveStorage_DecompressBuffer(compressed.data(), truncated ? compressedSize-1 : compressedSize,
                output.data()+1, truncated ? 32 : 8);
            std::_Exit(91);
        }
        int status; Check(::waitpid(child,&status,0)==child);
        Check(WIFSIGNALED(status) && WTERMSIG(status)==SIGABRT);
    }
    SV_GroupError("code=%d literal=100%%",7);
    Check(!std::strcmp(message,"GROUPS: code=7 literal=100%\n"));
    std::array<char,300> longText; longText.fill('x'); longText.back()=0;
    SV_GroupError("%s",longText.data());
    Check(std::strlen(message)==8+127+1);
    std::puts("PASS actual modified-zlib publisher decompression/canaries; retained assertion on truncated/small output; bounded group-error varargs");
}
