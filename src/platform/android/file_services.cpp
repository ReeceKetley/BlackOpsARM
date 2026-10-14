#include "file_services.h"
#include "legacy_crt.h"
#include <platform/win32_compat/win32_errors.h>
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <dirent.h>
#include <fcntl.h>
#include <memory>
#include <mutex>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <unordered_map>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
struct File {
    int fd;
    std::uint32_t access, sharing, flags;
    std::string path;
    std::mutex mutex;
    bool closed{};
    unsigned pendingCallbacks{};
    File(int descriptor, std::uint32_t a, std::uint32_t s, std::uint32_t f, std::string p)
        : fd(descriptor), access(a), sharing(s), flags(f), path(std::move(p)) {}
};
std::mutex handlesMutex;
std::unordered_map<void *, std::shared_ptr<File>> handles;
void error(std::uint32_t value) { bo1::win32::SetLastError(value); }
void posixError() {
    const auto value = errno;
    error(value == ENOENT ? 2 : value == EACCES || value == EPERM ? 5
          : value == EBADF ? 6 : value == ENOMEM ? 8 : value == EINVAL ? 87
          : value == ENOSPC ? 112 : value == EEXIST ? 80 : 31);
}
std::string pathFor(const char *path) {
    std::string result(path);
    std::replace(result.begin(), result.end(), '\\', '/');
    // Preserve Windows lookup for the original ASCII game paths, including
    // Common/common and English/english. Do not pick an ambiguous native alias.
    std::string resolved;
    if (!result.empty() && result.front() == '/') resolved = "/";
    else {
        char *cwd = getcwd(nullptr, 0);
        if (!cwd) return result;
        resolved = cwd;
        std::free(cwd);
    }
    std::size_t offset = 0;
    while (offset < result.size()) {
        auto end = result.find('/', offset);
        if (end == std::string::npos) end = result.size();
        auto part = result.substr(offset, end - offset);
        offset = end + 1;
        if (part.empty() || part == ".") continue;
        if (part == "..") {
            const auto slash = resolved.find_last_of('/');
            resolved = slash == 0 ? "/" : resolved.substr(0, slash);
            continue;
        }
        const auto candidate = resolved + (resolved == "/" ? "" : "/") + part;
        struct stat metadata{};
        if (lstat(candidate.c_str(), &metadata)) {
            if (DIR *directory = opendir(resolved.c_str())) {
                std::string match;
                bool ambiguous = false;
                while (auto *entry = readdir(directory)) {
                    if (bo1::crt::Stricmp(entry->d_name, part.c_str()) == 0) {
                        if (!match.empty()) { ambiguous = true; break; }
                        match = entry->d_name;
                    }
                }
                closedir(directory);
                if (ambiguous) { errno = EEXIST; return {}; }
                if (!match.empty()) part = match;
            }
        }
        resolved += (resolved == "/" ? "" : "/") + part;
    }
    char *canonical = realpath(resolved.c_str(), nullptr);
    if (canonical) { resolved = canonical; std::free(canonical); }
    return resolved;
}
std::shared_ptr<File> find(void *handle) {
    std::lock_guard<std::mutex> lock(handlesMutex);
    auto found = handles.find(handle);
    if (found == handles.end()) { error(6); return {}; }
    return found->second;
}
constexpr std::uint32_t readAccess = 0x80000000u, writeAccess = 0x40000000u;
constexpr std::uint32_t overlappedFlag = 0x40000000u, directFlag = 0x20000000u;
}
std::uint32_t GetLastError() { return bo1::win32::GetLastError(); }
std::string bo1::android::ResolveEngineFilePath(const char *path) {
    if(!path || !*path){error(87);return {};}
    return pathFor(path);
}
void Sys_Mkdir(const char *path) {
    const auto nativePath = bo1::android::ResolveEngineFilePath(path);
    if (!nativePath.empty()) {
        if (mkdir(nativePath.c_str(), 0700) == 0) return;
        struct stat info{};
        if (errno == EEXIST && stat(nativePath.c_str(), &info) == 0
            && S_ISDIR(info.st_mode)) return;
    }
    posixError();
    std::fprintf(stderr, "Native Sys_Mkdir failed for %s: %s\n",
        path ? path : "(null)", std::strerror(errno));
    std::abort();
}
int Sys_DirectoryHasContents(const char *path) {
    const auto nativePath = bo1::android::ResolveEngineFilePath(path);
    if (nativePath.empty()) return 0;
    DIR *directory = opendir(nativePath.c_str());
    if (!directory) { posixError(); return 0; }
    int found = 0;
    while (auto *entry = readdir(directory)) {
        const auto name = entry->d_name;
        if (!std::strcmp(name, ".") || !std::strcmp(name, "..")) continue;
        if (!bo1::crt::Stricmp(name, "CVS")) {
            struct stat info{};
            const auto child = nativePath + "/" + name;
            if (!stat(child.c_str(), &info) && S_ISDIR(info.st_mode)) continue;
        }
        found = 1;
        break;
    }
    closedir(directory);
    return found;
}
void *CreateFileA(const char *path, std::uint32_t access, std::uint32_t sharing,
                  void *security, std::uint32_t creation, std::uint32_t flags, void *templateFile) {
    void *const invalid = reinterpret_cast<void *>(static_cast<std::intptr_t>(-1));
    if (!path || !*path || security || templateFile || (access & ~(readAccess | writeAccess))
        || !access || (sharing & ~7u) || creation < 1 || creation > 5) { error(87); return invalid; }
    if (flags & ~(overlappedFlag | directFlag | 0x80u | 0x08000000u)) { error(50); return invalid; }
    const auto name = pathFor(path);
    if (name.empty()) { posixError(); return invalid; }
    std::lock_guard<std::mutex> lock(handlesMutex);
    for (const auto &entry : handles) if (entry.second->path == name) {
        const auto &other = *entry.second;
        if (((access & readAccess) && !(other.sharing & 1))
            || ((access & writeAccess) && !(other.sharing & 2))
            || ((other.access & readAccess) && !(sharing & 1))
            || ((other.access & writeAccess) && !(sharing & 2))) { error(32); return invalid; }
    }
    int mode = (access & writeAccess) ? ((access & readAccess) ? O_RDWR : O_WRONLY) : O_RDONLY;
    mode |= O_CLOEXEC;
    if (creation == 1) mode |= O_CREAT | O_EXCL;
    else if (creation == 2) mode |= O_CREAT | O_TRUNC;
    else if (creation == 4) mode |= O_CREAT;
    else if (creation == 5) mode |= O_TRUNC;
    if (flags & directFlag) mode |= O_DIRECT;
    struct stat prior{};
    const bool existed = stat(name.c_str(), &prior) == 0;
    int fd = open(name.c_str(), mode, 0600);
    if (fd < 0) { posixError(); return invalid; }
    try {
        auto file = std::make_shared<File>(fd, access, sharing, flags, name);
        void *handle = file.get();
        handles.emplace(handle, std::move(file));
        if (creation == 2 || creation == 4) error(existed ? 183 : 0);
        return handle;
    } catch (...) { close(fd); error(8); return invalid; }
}
namespace {
int transfer(void *handle, void *buffer, std::uint32_t bytes, std::uint32_t *count,
             OVERLAPPED *overlapped, bool writing) {
    if (count) *count = 0;
    if ((!buffer && bytes) || !count) { error(87); return 0; }
    auto file = find(handle);
    if (!file) return 0;
    std::lock_guard<std::mutex> lock(file->mutex);
    if (file->closed) { error(6); return 0; }
    if (overlapped || (file->flags & overlappedFlag)) { error(50); return 0; }
    if (!(file->access & (writing ? writeAccess : readAccess))) { error(5); return 0; }
    auto *data = static_cast<unsigned char *>(buffer);
    while (*count < bytes) {
        ssize_t transferred = writing ? write(file->fd, data + *count, bytes - *count)
                                      : read(file->fd, data + *count, bytes - *count);
        if (transferred < 0) { if (errno == EINTR) continue; posixError(); return 0; }
        if (!transferred) { if (writing) { error(112); return 0; } break; }
        *count += static_cast<std::uint32_t>(transferred);
        // A synchronous read may legitimately return fewer bytes than requested.
        if (!writing) break;
    }
    return 1;
}
}
int ReadFile(void *handle, void *buffer, std::uint32_t bytes, std::uint32_t *read, OVERLAPPED *overlapped) {
    return transfer(handle, buffer, bytes, read, overlapped, false);
}
int WriteFile(void *handle, const void *buffer, std::uint32_t bytes, std::uint32_t *written, OVERLAPPED *overlapped) {
    return transfer(handle, const_cast<void *>(buffer), bytes, written, overlapped, true);
}
int CloseHandle(void *handle) {
    std::shared_ptr<File> file;
    {
        std::lock_guard<std::mutex> lock(handlesMutex);
        auto found = handles.find(handle);
        if (found == handles.end()) { error(6); return 0; }
        file = found->second;
        std::lock_guard<std::mutex> fileLock(file->mutex);
        // Cancellation is not implemented yet. Reject close while accepted
        // callbacks are outstanding instead of claiming cancelled IO completed.
        if (file->pendingCallbacks) { error(170); return 0; }
        file->closed = true;
        handles.erase(found);
    }
    std::lock_guard<std::mutex> lock(file->mutex);
    file->closed = true;
    if (close(file->fd)) { posixError(); return 0; }
    return 1;
}
std::uint32_t GetFileSize(void *handle, std::uint32_t *high) {
    auto file = find(handle);
    if (!file) return 0xffffffffu;
    std::lock_guard<std::mutex> lock(file->mutex);
    struct stat metadata{};
    if (file->closed) { error(6); return 0xffffffffu; }
    if (fstat(file->fd, &metadata)) { posixError(); return 0xffffffffu; }
    const auto size = static_cast<std::uint64_t>(metadata.st_size);
    if (high) *high = static_cast<std::uint32_t>(size >> 32);
    if (static_cast<std::uint32_t>(size) == 0xffffffffu) error(0);
    return static_cast<std::uint32_t>(size);
}
std::uint32_t GetFileAttributesA(const char *path) {
    if (!path || !*path) { error(87); return 0xffffffffu; }
    struct stat metadata{};
    const auto name = pathFor(path);
    if (stat(name.c_str(), &metadata)) { posixError(); return 0xffffffffu; }
    return (S_ISDIR(metadata.st_mode) ? 0x10u : 0x80u) | ((metadata.st_mode&S_IWUSR)?0:1u);
}
int SetFileAttributesA(const char *path,std::uint32_t attributes) {
    if(!path || !*path || (attributes&~0x91u)){error(50);return 0;}
    const auto name=pathFor(path);struct stat metadata{};
    if(name.empty() || stat(name.c_str(),&metadata)){posixError();return 0;}
    if((attributes&0x10u) && !S_ISDIR(metadata.st_mode)){error(87);return 0;}
    const auto mode=(attributes&1u)?metadata.st_mode&~S_IWUSR:metadata.st_mode|S_IWUSR;
    if(chmod(name.c_str(),mode)){posixError();return 0;}
    return 1;
}

namespace {
struct AsyncRead;
struct CompletionQueue {
    std::mutex mutex;
    std::condition_variable condition;
    // Intrusive completed list avoids allocation after the IO was accepted.
    std::shared_ptr<AsyncRead> first, last;
};
struct AsyncRead {
    int fd{-1};
    void *buffer{};
    std::uint32_t bytes{}, errorCode{}, transferred{};
    off_t offset{};
    OVERLAPPED *overlapped{};
    BO1_FileCompletion completion{};
    std::shared_ptr<CompletionQueue> owner;
    std::shared_ptr<File> file;
    std::shared_ptr<AsyncRead> next;
    ~AsyncRead() { if (fd >= 0) close(fd); }
};
thread_local std::shared_ptr<CompletionQueue> threadCompletions;
struct Reader {
    std::mutex mutex;
    std::condition_variable condition;
    std::deque<std::shared_ptr<AsyncRead>> requests;
    std::thread thread;
    Reader() : thread([this] { run(); }) {}
    void run() {
        for (;;) {
            std::shared_ptr<AsyncRead> request;
            {
                std::unique_lock<std::mutex> lock(mutex);
                condition.wait(lock, [this] { return !requests.empty(); });
                request = std::move(requests.front());
                requests.pop_front();
            }
            ssize_t result;
            do {
                result = pread(request->fd, request->buffer, request->bytes, request->offset);
            } while (result < 0 && errno == EINTR);
            if (result < 0) {
                posixError();
                request->errorCode = GetLastError();
            } else if (!result && request->bytes) {
                request->errorCode = 38; // ERROR_HANDLE_EOF
            } else {
                request->transferred = static_cast<std::uint32_t>(result);
            }
            request->overlapped->Internal = request->errorCode == 0 ? 0
                : request->errorCode == 38 ? 0xc0000011u : 0xc0000001u;
            request->overlapped->InternalHigh = request->transferred;
            auto owner = request->owner;
            {
                std::lock_guard<std::mutex> lock(owner->mutex);
                if (owner->last) owner->last->next = request;
                else owner->first = request;
                owner->last = std::move(request);
            }
            owner->condition.notify_one();
        }
    }
};
Reader &reader() {
    // Process owned, like the engine's original endless DB worker. No static
    // destructor races outstanding IO. Callers must drain callbacks before
    // releasing their buffers, OVERLAPPED storage or initiating thread shutdown.
    static Reader *instance = new Reader;
    return *instance;
}
}
int ReadFileEx(void *handle, void *buffer, std::uint32_t bytes,
               OVERLAPPED *overlapped, BO1_FileCompletion completion) {
    if ((!buffer && bytes) || !overlapped || !completion) { error(87); return 0; }
    auto file = find(handle);
    if (!file) return 0;
    std::lock_guard<std::mutex> lock(file->mutex);
    if (file->closed) { error(6); return 0; }
    if (!(file->access & readAccess)) { error(5); return 0; }
    if (!(file->flags & overlappedFlag)) { error(87); return 0; }
    const auto offset = (static_cast<std::uint64_t>(overlapped->OffsetHigh) << 32) | overlapped->Offset;
    if (offset > static_cast<std::uint64_t>(INT64_MAX)) { error(87); return 0; }
    try {
        if (!threadCompletions) threadCompletions = std::make_shared<CompletionQueue>();
        auto request = std::make_shared<AsyncRead>();
        request->fd = fcntl(file->fd, F_DUPFD_CLOEXEC, 0);
        if (request->fd < 0) { posixError(); return 0; }
        request->buffer = buffer;
        request->bytes = bytes;
        request->offset = static_cast<off_t>(offset);
        request->overlapped = overlapped;
        request->completion = completion;
        request->owner = threadCompletions;
        request->file = file;
        auto &worker = reader();
        {
            std::lock_guard<std::mutex> queueLock(worker.mutex);
            worker.requests.push_back(request);
            ++file->pendingCallbacks;
            overlapped->Internal = 0x103; // STATUS_PENDING
            overlapped->InternalHigh = 0;
        }
        worker.condition.notify_one();
        return 1;
    } catch (...) { error(8); return 0; }
}
std::uint32_t SleepEx(std::uint32_t milliseconds, int alertable) {
    if (!alertable) {
        if (milliseconds == 0xffffffffu) {
            std::mutex mutex;
            std::condition_variable condition;
            std::unique_lock<std::mutex> lock(mutex);
            condition.wait(lock, [] { return false; });
        } else std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
        return 0;
    }
    try {
        if (!threadCompletions) threadCompletions = std::make_shared<CompletionQueue>();
    } catch (...) { std::fputs("Native alertable wait allocation failed\n", stderr); std::abort(); }
    auto owner = threadCompletions;
    std::shared_ptr<AsyncRead> completed;
    {
        std::unique_lock<std::mutex> lock(owner->mutex);
        if (milliseconds == 0xffffffffu)
            owner->condition.wait(lock, [&] { return bool(owner->first); });
        else if (!owner->condition.wait_for(lock, std::chrono::milliseconds(milliseconds),
                                          [&] { return bool(owner->first); })) return 0;
        completed = std::move(owner->first);
        owner->last.reset();
    }
    while (completed) {
        auto next = std::move(completed->next);
        // A completed request no longer needs to retain the issuing queue.
        completed->owner.reset();
        {
            std::lock_guard<std::mutex> lock(completed->file->mutex);
            --completed->file->pendingCallbacks;
        }
        completed->completion(completed->errorCode, completed->transferred, completed->overlapped);
        completed = std::move(next);
    }
    return 192; // WAIT_IO_COMPLETION; callbacks run only on the issuing thread.
}
