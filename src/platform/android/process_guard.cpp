#include "process_guard.h"
#include "thread_services.h"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

void Sys_Error(char *,...);
namespace {
int descriptor=-1;
pid_t ownerPid{},ownerTid{};
[[noreturn]] void fail(const char *reason,int error=0) {
    Sys_Error(const_cast<char *>("Native process guard: %s (%d)"),reason,error);
    std::abort();
}
void requireOwner() {
    if(!Sys_IsMainThread() || ownerPid!=getpid() || ownerTid!=gettid())fail("guard release requires original main/process owner");
}
void record(const char *state) {
    char text[96];const int bytes=std::snprintf(text,sizeof(text),"%s pid=%ld\n",state,static_cast<long>(ownerPid));
    if(bytes<=0 || bytes>=static_cast<int>(sizeof(text)))fail("marker format");
    if(ftruncate(descriptor,0))fail("marker truncate",errno);
    std::size_t done=0;
    while(done<static_cast<std::size_t>(bytes)) {
        const auto wrote=pwrite(descriptor,text+done,bytes-done,done);
        if(wrote<0 && errno==EINTR)continue;
        if(wrote<=0)fail("marker write",errno);
        done+=static_cast<std::size_t>(wrote);
    }
    if(fdatasync(descriptor))fail("marker persistence",errno);
}
}
bool bo1::android::AcquireProcessGuard(const char *root) {
    if(!Sys_IsMainThread() || descriptor>=0 || !root || !*root)fail("invalid/duplicate guard acquisition");
    const int directory=open(root,O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
    if(directory<0)fail("user directory",errno);
    const int opened=openat(directory,".blackopsarm-instance",O_RDWR|O_CREAT|O_CLOEXEC|O_NOFOLLOW,0600);
    const int openError=errno;close(directory);
    if(opened<0)fail("instance inode",openError);
    struct stat info{};
    if(fstat(opened,&info) || !S_ISREG(info.st_mode) || info.st_uid!=geteuid() || info.st_nlink!=1) {
        close(opened);fail("instance file ownership/type");
    }
    if(flock(opened,LOCK_EX|LOCK_NB)) {
        const int error=errno;close(opened);
        if(error==EWOULDBLOCK || error==EAGAIN)return false;
        fail("instance exclusion lock",error);
    }
    char previous[96]{};
    const auto bytes=pread(opened,previous,sizeof(previous)-1,0);
    if(bytes<0){const int error=errno;close(opened);fail("previous marker read",error);}
    if(bytes>0 && std::strncmp(previous,"clean pid=",10))
        std::fputs("Native process guard: previous instance did not publish a clean exit\n",stderr);
    descriptor=opened;ownerPid=getpid();ownerTid=gettid();record("running");
    return true;
}
void bo1::android::ReleaseProcessGuardClean() {
    requireOwner();if(descriptor<0)fail("clean release without acquired guard");
    record("clean");
    // Never unlink a locked instance inode: another process could open a new
    // inode and defeat exclusion while the old owner still holds its lock.
    if(flock(descriptor,LOCK_UN))fail("instance unlock",errno);
    if(close(descriptor))fail("instance descriptor retirement",errno);
    descriptor=-1;ownerPid=ownerTid=0;
}
bool bo1::android::ProcessGuardHeld() {
    return Sys_IsMainThread() && descriptor>=0 && ownerPid==getpid() && ownerTid==gettid();
}
