#include <platform/android/assertion_services.h>
#include <universal/assertive.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <atomic>
#include <stdexcept>

static unsigned checks;
static unsigned *callbackCount;
static void check(bool ok,const char *name){if(!ok){fprintf(stderr,"FAIL %s\n",name);exit(1);}++checks;printf("PASS %s\n",name);}
static void callback(const char *message){if(strstr(message,"fixture.cpp(42) type=7: value=99"))++*callbackCount;}
static void recursive(const char *){++*callbackCount;Assert_MyHandler("recursive.cpp",11,0,"recursive");}
static void throwing(const char *){++*callbackCount;throw std::runtime_error("fixture callback failure");}
static void normal(){Assert_SetMonkeyCallbackHandler(callback);Assert_MyHandler("fixture.cpp",42,7,"value=%d",99);}
static void cleared(){Assert_SetMonkeyCallbackHandler(callback);Assert_SetMonkeyCallbackHandler(nullptr);Assert_MyHandler("fixture.cpp",42,7,"value=%d",99);}
static void recursion(){Assert_SetMonkeyCallbackHandler(recursive);Assert_MyHandler("fixture.cpp",42,7,"value=%d",99);}
static void exception(){Assert_SetMonkeyCallbackHandler(throwing);Assert_MyHandler("fixture.cpp",42,7,"value=%d",99);}
static void longMessage(){char text[3000];memset(text,'x',sizeof(text)-1);text[sizeof(text)-1]=0;Assert_MyHandler("long.cpp",1,0,"%s",text);}
static std::string probe(void(*action)(),unsigned expected){
    *callbackCount=0;int channel[2];check(pipe(channel)==0,"create actual assertion capture pipe");pid_t child=fork();
    if(!child){close(channel[0]);dup2(channel[1],STDERR_FILENO);close(channel[1]);action();_Exit(99);}
    check(child>0,"fork real assertion failure process");close(channel[1]);std::string text;char buffer[4096];ssize_t size;
    while((size=read(channel[0],buffer,sizeof(buffer)))>0)text.append(buffer,size);close(channel[0]);int status=0;check(waitpid(child,&status,0)==child,"join terminated assertion process");
    check(WIFEXITED(status)&&WEXITSTATUS(status)==2,"actual native fatal owner terminates assertion with exit2");
    check(*callbackCount==expected,"actual reporter invocation count preserves registration/clear/recursion");return text;
}
int main(){
    callbackCount=static_cast<unsigned *>(mmap(nullptr,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0));check(callbackCount!=MAP_FAILED,"shared actual reporter evidence created");
    std::atomic<bool> done{false};std::thread publisher([&]{for(unsigned i=0;i<10000;++i)Assert_SetMonkeyCallbackHandler(i&1?callback:nullptr);done.store(true,std::memory_order_release);});
    while(!done.load(std::memory_order_acquire))Assert_SetMonkeyCallbackHandler(nullptr);publisher.join();Assert_SetMonkeyCallbackHandler(nullptr);
    check(true,"concurrent actual reporter publication/clear completes without a held engine lock");
    auto text=probe(normal,1);check(text.find("ASSERT fixture.cpp(42) type=7: value=99")!=std::string::npos,"actual bounded formatter logs source/line/type/expression");
    check(text.find("Native assertion stack (program counters):")!=std::string::npos&&text.find("#0 0x")!=std::string::npos,"actual native assertion stack captures program counters");
    const auto pc=text.find("#0 0x");check(pc!=std::string::npos&&strtoull(text.c_str()+pc+3,nullptr,16)>0xffffffffULL,"actual unwind retains native high-width program counter");
    text=probe(cleared,0);check(text.find("Native engine fatal error:")!=std::string::npos,"cleared reporter never suppresses actual fatal assertion");
    text=probe(recursion,1);check(text.find("Native recursive/concurrent assertion")!=std::string::npos,"recursive reporter assertion takes immediate fatal path");
    text=probe(exception,1);check(text.find("Assertion reporting callback threw an exception")!=std::string::npos,"throwing reporter cannot escape assertion fatal path");
    text=probe(longMessage,0);check(text.find("ASSERT long.cpp(1)")!=std::string::npos&&text.size()<10000,"oversize expression stays bounded through actual stack/fatal logging");
    munmap(callbackCount,4096);printf("ASSERTION_SERVICES PASS checks=%u\n",checks);
}
