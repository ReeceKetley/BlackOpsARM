#include "zombie_save_store.h"
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <sys/stat.h>
#include <unistd.h>

namespace bo1::android::save_store {
namespace {
constexpr std::size_t HeaderSize = 64;
constexpr unsigned char Magic[8] = {'B','O','1','Z','S','A','V','E'};
constexpr const char *Names[] = {"suspend.0", "suspend.1"};
struct File {
    int fd{-1};
    explicit File(int value) : fd(value) {}
    ~File() { if (fd >= 0) close(fd); }
};
void Put(unsigned char *p, std::uint64_t value, unsigned size) {
    for (unsigned i=0; i<size; ++i) p[i]=static_cast<unsigned char>(value>>(i*8));
}
std::uint64_t Get(const unsigned char *p, unsigned size) {
    std::uint64_t value=0;
    for (unsigned i=0; i<size; ++i) value|=std::uint64_t(p[i])<<(i*8);
    return value;
}
std::uint32_t Crc(std::uint32_t value, const unsigned char *p, std::size_t size) {
    for (std::size_t i=0; i<size; ++i) {
        value^=p[i];
        for (unsigned bit=0; bit<8; ++bit) value=(value>>1)^(0xedb88320u & (0u-(value&1)));
    }
    return value;
}
bool Transfer(int fd, unsigned char *p, std::size_t size, bool writing) {
    while (size) {
        const auto n=writing ? write(fd,p,size) : read(fd,p,size);
        if (n<0 && errno==EINTR) continue;
        if (n<=0) return false;
        p+=n; size-=static_cast<std::size_t>(n);
    }
    return true;
}
int Directory(const char *root, bool create) {
    if (!root || !*root) return -1;
    File parent(open(root,O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW));
    if (parent.fd<0) return -1;
    if (create) {
        if (mkdirat(parent.fd,"save-states",0700) && errno!=EEXIST) return -1;
        // Persist directory creation before publishing any snapshot in it.
        if (fsync(parent.fd)) return -1;
    }
    return openat(parent.fd,"save-states",O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
}
bool ValidSizes(std::size_t m, std::size_t g, std::size_t c) {
    return m && m<=MaxMetadata && g && g<=MaxGame && c<=MaxClient;
}
bool ReadOne(int directory, unsigned slot, Snapshot &out) {
    File file(openat(directory,Names[slot],O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK));
    if (file.fd<0) return false;
    struct stat info{};
    if (fstat(file.fd,&info) || !S_ISREG(info.st_mode) || info.st_size<static_cast<off_t>(HeaderSize)) return false;
    std::array<unsigned char,HeaderSize> header{};
    if (!Transfer(file.fd,header.data(),header.size(),false) || std::memcmp(header.data(),Magic,8)
        || Get(header.data()+8,4)!=1) return false;
    const auto flags=Get(header.data()+12,4);
    if (flags!=DiagnosticPayload && flags!=SoloResumePayload) return false;
    for (unsigned i=40; i<HeaderSize; ++i) if (header[i]) return false;
    const auto m=Get(header.data()+24,4), g=Get(header.data()+28,4), c=Get(header.data()+32,4);
    const auto generation=Get(header.data()+16,8);
    if (!generation || !ValidSizes(m,g,c) || (flags==SoloResumePayload && !c)
        || std::uint64_t(info.st_size)!=HeaderSize+m+g+c) return false;
    const auto expected=Get(header.data()+36,4);
    Put(header.data()+36,0,4);
    auto crc=Crc(~0u,header.data(),header.size());
    Snapshot candidate;
    candidate.generation=generation;
    candidate.flags=static_cast<std::uint32_t>(flags);
    candidate.metadata.resize(m); candidate.game.resize(g); candidate.client.resize(c);
    for (auto *bytes : {&candidate.metadata,&candidate.game,&candidate.client}) {
        if (!Transfer(file.fd,bytes->data(),bytes->size(),false)) return false;
        crc=Crc(crc,bytes->data(),bytes->size());
    }
    if ((crc^~0u)!=expected) return false;
    out=std::move(candidate);
    return true;
}
}
bool Read(const char *root, Snapshot &snapshot, std::string &error) {
    File directory(Directory(root,false));
    Snapshot a,b;
    const bool av=directory.fd>=0 && ReadOne(directory.fd,0,a);
    const bool bv=directory.fd>=0 && ReadOne(directory.fd,1,b);
    if (!av && !bv) { error="no intact supported snapshot generation"; return false; }
    snapshot=std::move(av && (!bv || a.generation>=b.generation) ? a : b);
    error.clear(); return true;
}
bool Write(const char *root, const Snapshot &snapshot, std::string &error) {
    if ((snapshot.flags!=DiagnosticPayload && snapshot.flags!=SoloResumePayload)
        || (snapshot.flags==SoloResumePayload && snapshot.client.empty())
        || !ValidSizes(snapshot.metadata.size(),snapshot.game.size(),snapshot.client.size())) {
        error="unsupported flags or snapshot sizes"; return false;
    }
    File directory(Directory(root,true));
    if (directory.fd<0) { error="cannot open save directory"; return false; }
    Snapshot a,b;
    const bool av=ReadOne(directory.fd,0,a), bv=ReadOne(directory.fd,1,b);
    const auto generation=av && (!bv || a.generation>=b.generation) ? a.generation : (bv ? b.generation : 0);
    if (generation==std::numeric_limits<std::uint64_t>::max()) { error="snapshot generation exhausted"; return false; }
    const unsigned target=!av ? 0 : (!bv ? 1 : (a.generation<=b.generation ? 0 : 1));
    std::array<unsigned char,HeaderSize> header{};
    std::memcpy(header.data(),Magic,8);
    Put(header.data()+8,1,4); Put(header.data()+12,snapshot.flags,4); Put(header.data()+16,generation+1,8);
    Put(header.data()+24,snapshot.metadata.size(),4); Put(header.data()+28,snapshot.game.size(),4); Put(header.data()+32,snapshot.client.size(),4);
    auto crc=Crc(~0u,header.data(),header.size());
    for (const auto *bytes : {&snapshot.metadata,&snapshot.game,&snapshot.client}) crc=Crc(crc,bytes->data(),bytes->size());
    Put(header.data()+36,crc^~0u,4);
    // Fixed temporary name is safe under the single-writer contract. Never follow
    // stale links or truncate either committed generation while staging.
    unlinkat(directory.fd,"pending",0);
    File pending(openat(directory.fd,"pending",O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600));
    bool ok=pending.fd>=0 && Transfer(pending.fd,header.data(),header.size(),true);
    for (const auto *bytes : {&snapshot.metadata,&snapshot.game,&snapshot.client})
        if (ok) ok=Transfer(pending.fd,const_cast<unsigned char *>(bytes->data()),bytes->size(),true);
    if (ok) ok=fsync(pending.fd)==0;
    if (pending.fd>=0) { if (close(pending.fd)) ok=false; pending.fd=-1; }
    if (!ok) { unlinkat(directory.fd,"pending",0); error="snapshot staging write or flush failed"; return false; }
    if (renameat(directory.fd,"pending",directory.fd,Names[target])) {
        unlinkat(directory.fd,"pending",0); error="snapshot publish failed"; return false;
    }
    if (fsync(directory.fd)) { error="snapshot published but directory durability is unconfirmed"; return false; }
    error.clear(); return true;
}
}
