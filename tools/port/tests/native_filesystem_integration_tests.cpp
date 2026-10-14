#include <platform/startup_services_api.h>
#include <platform/android/thread_services.h>
#include <platform/android/engine_paths.h>
#include <platform/android/file_services.h>
#include <platform/android/virtual_memory.h>
#include <universal/memory_services.h>
#include <universal/com_files.h>
#include <qcommon/md4.h>
#include <universal/com_fileaccess.h>
#include <qcommon/unzip.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <thread>
#include <atomic>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
extern "C" void __cyg_profile_func_enter(void *, void *) {}
extern "C" void __cyg_profile_func_exit(void *, void *) {}
struct HunkUser;
HunkUser **Sys_ListFiles(char *,char *,char *,int *,int);
int CountBitsEnabled(unsigned int);
const char *Sys_DefaultCDPath();
struct GfxImage;
extern GfxImage g_imageProgs[];
bool Image_IsProg(GfxImage *);
void Com_ReadCDKey();
extern char cl_cdkey[34],cl_cdkey_dw[34],cl_cdkeychecksum[10];
void check(bool result,const char *name) {
    if(!result){std::fprintf(stderr,"FAIL %s\n",name);std::exit(1);}
    std::printf("PASS %s\n",name);
}
int main(int argc,char **argv) {
    if(argc!=3)return 2;
    Sys_InitMainThread();
    check(!std::strcmp(Sys_GetCurrentThreadName(),"Main") && !Sys_IsStreamThread(),"canonical main thread identity");
    bool streamIdentity=false;
    std::thread stream([&] {
        Sys_InitThread(THREAD_CONTEXT_STREAM);
        streamIdentity=Sys_IsStreamThread() && !std::strcmp(Sys_GetCurrentThreadName(),"Stream");
        bo1::android::RetireThreadRole();
    });stream.join();
    check(streamIdentity,"actual stream TLS and canonical name");
    check(CountBitsEnabled(0)==0 && CountBitsEnabled(0xffffffffu)==32
        && CountBitsEnabled(0x11111111u)==8 && !*Sys_DefaultCDPath(),"original resource bit counts and no-CD search policy");
    check(Image_IsProg(g_imageProgs) && !Image_IsProg(nullptr),"native program-image address interval preserves reference classification without array-pointer UB");
    std::atomic<bool> checksumValid{true};
    std::vector<std::thread> checksumWorkers;
    for(int i=0;i<8;++i)checksumWorkers.emplace_back([&] {
        const auto value=Com_BlockChecksumKey32(reinterpret_cast<const unsigned char *>("123456789"),9,0);
        if(static_cast<unsigned>(value)!=0xcbf43926u)checksumValid=false;
    });
    for(auto &worker:checksumWorkers)worker.join();
    check(checksumValid,"original checksum with concurrent native table initialization");
    auto mappingBaseline=bo1::android::VirtualReservationCount();
    char *allocation=FS_AllocMem(70000);
    bool zero=true;for(int i=0;i<70000;++i)zero&=allocation[i]==0;
    std::memset(allocation,0xa5,70000);
    check(zero && bo1::android::VirtualReservationCount()==mappingBaseline+1,"actual FS virtual allocation zeroing and ownership");
    FS_FreeMem(allocation);FS_FreeMem(nullptr);
    check(bo1::android::VirtualReservationCount()==mappingBaseline,"actual FS virtual release and original null contract");
    const pid_t child=fork();
    if(child==0) {
        rlimit limit{16*1024*1024,16*1024*1024};
        if(setrlimit(RLIMIT_AS,&limit))std::_Exit(2);
        auto *failed=Z_TryVirtualAlloc(1024*1024*1024,"forced mapping failure",0);
        std::_Exit(!failed && bo1::android::VirtualReservationCount()==mappingBaseline?0:1);
    }
    int status=0;
    check(child>0 && waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==0,
        "actual try-allocation mapping failure returns null without ownership leak");
    bo1::android::InstallEngineDataRoot(argv[1]);
    bo1::android::InstallEngineUserRoot(argv[2]);
    check(!std::strcmp(Sys_DefaultInstallPath(),argv[1]) && !std::strcmp(Sys_EngineUserRoot(),argv[2]),"distinct actual data and writable roots");
    // This executable receives the synthetic fixture directory, never retail assets.
    check(chdir(argv[1])==0 && GetFileAttributesA("dwclientkey.txt")==0xffffffffu
        && GetLastError()==2,"test key fixture has no existing file to overwrite");
    FILE *key=FS_FileOpenWriteBinary("dwclientkey.txt");
    check(key && FS_FileWrite("ABCDE12345FGHIJ67890",20,key)==20,"test-owned key fixture write");FS_FileClose(key);
    Com_ReadCDKey();
    check(!std::strcmp(cl_cdkey,"ABCDE12345FGHIJ67890")
        && !std::strcmp(cl_cdkey_dw,"ABCD-E123-45FG-HIJ6-7890")
        && !std::memcmp(cl_cdkeychecksum,"7890",4),"actual key read, checksum bytes and overlap-safe reference formatting");
    key=FS_FileOpenWriteBinary("dwclientkey.txt");
    check(key && FS_FileWrite("bad",3,key)==3,"test-owned short key fixture write");FS_FileClose(key);
    Com_ReadCDKey();check(!std::strcmp(cl_cdkey,"                                "),"original short-key rejection clears key");
    check(unlink("dwclientkey.txt")==0,"test-owned key fixture cleanup");
    Com_ReadCDKey();check(!std::strcmp(cl_cdkey,"                                "),"original missing-key behavior preserved");
    const auto directory=std::string(argv[2])+"/SubDir";
    Sys_Mkdir(directory.c_str());Sys_Mkdir((std::string(argv[2])+"\\subdir").c_str());
    check(!Sys_DirectoryHasContents(directory.c_str()),"directory creation, case lookup and empty state");
    const auto cvs=directory+"/CVS";Sys_Mkdir(cvs.c_str());
    check(!Sys_DirectoryHasContents(directory.c_str()),"original CVS-directory exclusion");
    const auto text=directory+"/Mixed.TXT";
    FILE *file=FS_FileOpenWriteText(text.c_str());check(file,"actual engine text open");
    const char input[]="a\nb\r\nc\r";
    check(FS_FileWrite(input,sizeof(input)-1,file)==sizeof(input)-1,"logical text write count");
    char update[32]{};
    check(FS_FileSeek(file,0,2)==0 && FS_FileRead(update,sizeof(update),file)==sizeof(input)-1
        && !std::memcmp(update,input,sizeof(input)-1),"actual engine text update seek/read");
    FS_FileClose(file);
    file=FS_FileOpenAppendText((std::string(argv[2])+"\\subdir\\mixed.txt").c_str());check(file,"engine append resolves Windows path");
    check(FS_FileWrite("end\n",4,file)==4,"actual text append");FS_FileClose(file);
    file=FS_FileOpenReadBinary(text.c_str());char buffer[32]{};
    auto count=FS_FileRead(buffer,sizeof(buffer),file);FS_FileClose(file);
    const char physical[]="a\r\nb\r\r\nc\rend\r\n";
    check(count==sizeof(physical)-1 && !std::memcmp(buffer,physical,count),"physical Windows CRLF translation");
    file=FS_FileOpenReadText(text.c_str());count=FS_FileRead(buffer,sizeof(buffer),file);FS_FileClose(file);
    check(count==sizeof(input)-1+4 && !std::memcmp(buffer,"a\nb\r\nc\rend\n",count),"engine text readback");
    check(Sys_DirectoryHasContents(directory.c_str()),"directory detects actual content");
    check(SetFileAttributesA(text.c_str(),1) && (GetFileAttributesA(text.c_str())&1),"real owner-write permission removed");
    check(SetFileAttributesA(text.c_str(),128) && !(GetFileAttributesA(text.c_str())&1),"real owner-write permission restored");
    check(!SetFileAttributesA(text.c_str(),2) && GetLastError()==50,"unsupported attributes rejected");
    const auto visible=directory+"/Visible";Sys_Mkdir(visible.c_str());
    char extension[]="txt",directories[]="/",filter[]="*Mixed.TXT",lowerFilter[]="*mixed.txt";int listed=-1;
    auto **list=Sys_ListFiles(const_cast<char *>(directory.c_str()),extension,nullptr,&listed,0);
    auto **names=static_cast<const char **>(static_cast<void *>(list));
    check(list && listed==1 && !std::strcmp(names[0],"Mixed.TXT") && !names[1],"native enumeration preserves case-insensitive extension policy and terminator");
    FS_FreeFileList(names);
    check(bo1::android::VirtualReservationCount()==mappingBaseline,"original FS_FreeFileList releases real hunk owner");
    list=Sys_ListFiles(const_cast<char *>(directory.c_str()),directories,nullptr,&listed,0);
    names=static_cast<const char **>(static_cast<void *>(list));
    check(list && listed==1 && !std::strcmp(names[0],"Visible") && !names[1],"original directory-only policy excludes CVS");
    FS_FreeFileList(names);
    list=Sys_ListFiles(const_cast<char *>(directory.c_str()),nullptr,filter,&listed,0);
    names=static_cast<const char **>(static_cast<void *>(list));
    check(list && listed==1 && !std::strcmp(names[0],"\\Mixed.TXT") && !names[1],"original filtered filename spelling and ownership");
    FS_FreeFileList(names);
    list=Sys_ListFiles(const_cast<char *>(directory.c_str()),nullptr,lowerFilter,&listed,0);
    check(!list && listed==0,"validated reference wildcard substring case behavior preserved");
    list=Sys_ListFiles(const_cast<char *>(visible.c_str()),nullptr,nullptr,&listed,0);
    check(!list && listed==0 && bo1::android::VirtualReservationCount()==mappingBaseline,"empty enumeration returns no owner or mapping leak");
    const auto archive=std::string(argv[1])+"/Fixture.IWD";
    unzFile zip=unzOpen(archive.c_str());check(zip,"original IWD reader opens archive");
    unz_global_info global{};check(unzGetGlobalInfo(zip,&global)==UNZ_OK && global.number_entry==2,"actual central directory enumeration");
    check(unzGoToFirstFile(zip)==UNZ_OK,"original first-entry selection");
    unz_file_info info{};char name[256]{};
    check(unzGetCurrentFileInfo(zip,&info,name,sizeof(name),nullptr,0,nullptr,0)==UNZ_OK
        && !std::strcmp(name,"Data/Pattern.bin") && info.uncompressed_size==70000,"native metadata preserves disk fields");
    unsigned long position{};check(unzGetCurrentFileInfoPosition(zip,&position)==UNZ_OK,"save actual directory position");
    check(unzOpenCurrentFile(zip)==UNZ_OK,"original deflate stream opened");
    std::vector<unsigned char> payload(70000);unsigned offset=0;
    while(offset<payload.size()) {int got=unzReadCurrentFile(zip,payload.data()+offset,
        std::min<unsigned>(137,static_cast<unsigned>(payload.size())-offset));
        if(got<=0)check(false,"compressed chunk read");offset+=static_cast<unsigned>(got);}
    bool valid=true;for(unsigned i=0;i<payload.size();++i)valid&=payload[i]==i%251;
    check(valid && unzReadCurrentFile(zip,buffer,sizeof(buffer))==0,"full original inflate output and EOF");
    check(unzCloseCurrentFile(zip)==UNZ_OK,"original payload CRC verification");
    check(unzGoToNextFile(zip)==UNZ_OK && unzOpenCurrentFile(zip)==UNZ_OK,"stored entry selection");
    count=unzReadCurrentFile(zip,buffer,sizeof(buffer));
    check(count==6 && !std::memcmp(buffer,"stored",6) && unzCloseCurrentFile(zip)==UNZ_OK,"stored entry data and CRC");
    check(unzGoToNextFile(zip)==UNZ_END_OF_LIST_OF_FILE,"archive enumeration terminates");
    check(unzSetCurrentFileInfoPosition(zip,position)==UNZ_OK,"restore actual directory position");
    unzFile reopened=unzReOpen(archive.c_str(),zip);check(reopened && unzClose(reopened)==UNZ_OK,"independent reopened file lifetime");
    check(unzClose(zip)==UNZ_OK,"original archive closes");
    check(unlink(text.c_str())==0 && rmdir(cvs.c_str())==0 && rmdir(visible.c_str())==0 && rmdir(directory.c_str())==0,"test-owned filesystem cleanup");
}
