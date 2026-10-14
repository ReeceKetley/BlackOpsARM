#include "stdio_services.h"
#include "file_services.h"
#include <cstring>
#include <cstdlib>
#include <mutex>
#include <set>

namespace {
std::mutex streamMutex;
std::set<FILE *> textStreams;
}
FILE *bo1::android::OpenStdio(const char *path, const char *mode) {
    const auto nativePath=ResolveEngineFilePath(path);
    if(nativePath.empty())return nullptr;
    FILE *stream = fopen(nativePath.c_str(), mode);
    if (stream && strchr(mode, 't')) {
        try {
            std::lock_guard<std::mutex> guard(streamMutex);
            textStreams.insert(stream);
        } catch (...) { fclose(stream); throw; }
    }
    return stream;
}
unsigned int bo1::android::ReadStdio(void *destination, unsigned int bytes, FILE *stream) {
    std::lock_guard<std::mutex> guard(streamMutex);
    if (!textStreams.count(stream)) return fread(destination, 1, bytes, stream);
    // Match Windows text-mode reads: CRLF -> LF; Ctrl-Z marks EOF.
    auto *output = static_cast<unsigned char *>(destination);
    unsigned int count = 0;
    while (count < bytes) {
        int ch = fgetc(stream);
        if (ch == EOF) break;
        if (ch == 26) { ungetc(ch, stream); break; }
        if (ch == '\r') {
            int next = fgetc(stream);
            if (next == '\n') ch = '\n';
            else if (next != EOF) ungetc(next, stream);
        }
        output[count++] = static_cast<unsigned char>(ch);
    }
    return count;
}
void bo1::android::CloseStdio(FILE *stream) {
    std::lock_guard<std::mutex> guard(streamMutex);
    textStreams.erase(stream);
    fclose(stream);
}
unsigned int bo1::android::WriteStdio(const void *source,unsigned int bytes,FILE *stream) {
    std::lock_guard<std::mutex> guard(streamMutex);
    if(!textStreams.count(stream))return fwrite(source,1,bytes,stream);
    const auto *input=static_cast<const unsigned char *>(source);
    unsigned count=0;
    for(;count<bytes;++count) {
        if(input[count]=='\n' && fputc('\r',stream)==EOF)break;
        if(fputc(input[count],stream)==EOF)break;
    }
    return count; // Number of caller bytes, not translated physical bytes.
}
