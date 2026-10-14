#include <tl/base/tl_thread.h>
#include <sound/snd_dsp_types.h>
#include <atomic>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <string>
#if defined(__ANDROID__)
#include <platform/android/file_services.h>
#include <platform/android/legacy_crt.h>
using Count = std::uint32_t;
#else
#include <Windows.h>
using Count = DWORD;
#endif
std::thread::id issuingThread;
int callbackCount;
bool callbackThreadCorrect=true;
struct ReadRecord {
    OVERLAPPED state{};
    Count error{999}, bytes{999};
};
void
#if !defined(__ANDROID__)
CALLBACK
#endif
completed(Count error, Count bytes, OVERLAPPED *state) {
    auto *record=reinterpret_cast<ReadRecord *>(state);
    record->error=error;record->bytes=bytes;
    callbackThreadCorrect &= std::this_thread::get_id()==issuingThread;
    ++callbackCount;
}
void check(bool value,const char *name) {
    if(!value){std::fprintf(stderr,"FAIL %s\n",name);std::exit(1);}
    std::printf("PASS %s\n",name);
}
int main(int argc,char **argv) {
    if(argc!=2)return 2;
    tlAtomicReadWriteMutex mutex;
    mutex.WriteLock();mutex.WriteLock();mutex.ReadLock();
    check(mutex.WriteLockCount==2&&mutex.ReadLockCount==1,"recursive writer and nested reader");
    mutex.ReadUnlock();mutex.WriteUnlock();mutex.WriteUnlock();
    check(!mutex.WriteThreadId&&!mutex.WriteLockCount&&!mutex.ReadLockCount,"nested counts restore");
    int payload=0;std::atomic<bool> done{},consistent{true};
    std::thread reader([&]{int previous=0;while(!done){mutex.ReadLock();int now=payload;
        if(now<previous)consistent=false;previous=now;mutex.ReadUnlock();}});
    auto write=[&]{for(int i=0;i<1000;++i){mutex.WriteLock();++payload;mutex.WriteUnlock();}};
    std::thread one(write),two(write);one.join();two.join();done=true;reader.join();
    check(consistent&&payload==2000&&!mutex.WriteThreadId&&!mutex.ReadLockCount&&!mutex.WriteLockCount,
          "TL writer exclusion reader publication and zero counts");
    std::printf("DSP %zu %zu %zu %zu\n",sizeof(snd_dsp_master_params),
                sizeof(snd_dsp_dynamo_state),sizeof(snd_dsp_master_state),alignof(snd_dsp_master_state));
    for(int number:{0,1,-1,INT_MIN,INT_MAX})for(int radix:{2,10,16,36}) {
        char text[40]{};_itoa(number,text,radix);std::printf("ITOA %d %d %s\n",number,radix,text);
    }
    for(auto first:{"", "AbC", "abc_01", "Z"})for(auto second:{"", "aBc", "abcd", "z"})
        std::printf("CMP %s|%s %d\n",first,second,_stricmp(first,second));
    char *copy=_strdup("Game asset/name");check(copy&&strcmp(copy,"Game asset/name")==0,"owned duplicate");free(copy);
    std::string name=std::string(argv[1])+"/payload.dat";
    void *invalid=reinterpret_cast<void *>(static_cast<std::intptr_t>(-1));
    auto file=CreateFileA(name.c_str(),0x40000000u,0,nullptr,1,0x80u,nullptr);
    check(file!=invalid,"create real file");Count count=0;
    const unsigned char expected[]={0,7,255,1,0,8,2};
    check(WriteFile(file,expected,sizeof(expected),&count,nullptr)&&count==sizeof(expected),"exact binary write");
    Count high=99;check(GetFileSize(file,&high)==sizeof(expected)&&high==0,"native size words");
    auto denied=CreateFileA(name.c_str(),0x80000000u,1,nullptr,3,0x80u,nullptr);
    check(denied==invalid&&GetLastError()==32,"sharing violation before mutation");
    check(CloseHandle(file)&&!CloseHandle(file)&&GetLastError()==6,"close and reject stale handle");
    std::string folded=std::string(argv[1])+"/PAYLOAD.DAT";
    file=CreateFileA(folded.c_str(),0x80000000u,1,nullptr,3,0x80u,nullptr);
    check(file!=invalid,"case folded existing game path");
    unsigned char read[16]{};count=0;
    check(ReadFile(file,read,sizeof(read),&count,nullptr)&&count==sizeof(expected)
          &&!memcmp(expected,read,sizeof(expected)),"exact binary read and short count");
    check(ReadFile(file,read,1,&count,nullptr)&&count==0,"EOF is successful zero read");
    check(CloseHandle(file),"close reader");
    file=CreateFileA(name.c_str(),0x40000000u,0,nullptr,2,0x80u,nullptr);
    check(file!=invalid&&GetLastError()==183&&GetFileSize(file,nullptr)==0,"existing create always truncates and reports");
    check(WriteFile(file,expected,sizeof(expected),&count,nullptr)&&CloseHandle(file)
          &&GetFileAttributesA(name.c_str())!=0xffffffffu,"real file attributes");
    file=CreateFileA(name.c_str(),0x80000000u,1,nullptr,3,0x40000000u,nullptr);
    check(file!=invalid,"open real overlapped file");
    ReadRecord records[3];unsigned char buffers[3][16]{};
    records[1].state.Offset=2;records[2].state.OffsetHigh=1;
    issuingThread=std::this_thread::get_id();int accepted=0;
    for(int i=0;i<3;++i) {
        bool success=ReadFileEx(file,buffers[i],sizeof(buffers[i]),&records[i].state,completed)!=0;
        if(success)++accepted;
        else {records[i].error=GetLastError();records[i].bytes=0;}
        std::printf("SUBMIT %d %d %u\n",i,success,success?0:records[i].error);
    }
    SleepEx(10,0);
    check(callbackCount==0,"nonalertable sleep does not dispatch callbacks");
#if defined(__ANDROID__)
    check(!CloseHandle(file)&&GetLastError()==170,"pending close explicitly rejects unavailable cancellation");
#endif
    std::thread other([] {check(SleepEx(5,1)==0,"other thread cannot consume issuing callbacks");});
    other.join();
    while(callbackCount<accepted)check(SleepEx(5000,1)==192,"alertable wait dispatches real completion");
    check(callbackThreadCorrect,"completions run on issuing thread");
    check(records[0].error==0&&records[0].bytes==7&&!memcmp(buffers[0],expected,7)
          &&records[1].error==0&&records[1].bytes==5&&!memcmp(buffers[1],expected+2,5)
          &&records[2].error==38&&records[2].bytes==0,"offset snapshots short reads and 64 bit EOF");
    for(int i=0;i<3;++i)std::printf("COMPLETE %d %u %u\n",i,records[i].error,records[i].bytes);
    check(SleepEx(1,1)==0&&CloseHandle(file),"drained completion timeout and close");
}
