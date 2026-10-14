#include <cstdio>
#include <cstdlib>
#include <thread>
void SV_ClearServerThreadOwnsGame();
void SV_IncServerThreadOwnsGame();
void SV_DecServerThreadOwnsGame();
int SV_GetServerThreadOwnsGame();
bool Assert_MyHandler(const char *,int,int,const char *,...){std::fputs("FAIL production ownership assertion\n",stderr);std::abort();}
static void check(bool ok,const char *name){if(!ok){std::fprintf(stderr,"FAIL %s\n",name);std::exit(1);}std::printf("PASS %s\n",name);}
int main(){
    SV_ClearServerThreadOwnsGame();check(SV_GetServerThreadOwnsGame()==0,"production ownership clear/get");
    for(int cycle=0;cycle<2;++cycle){
        std::thread threads[4];
        for(auto &thread:threads)thread=std::thread([]{for(int i=0;i<256;++i)SV_IncServerThreadOwnsGame();});
        for(auto &thread:threads)thread.join();
        check(SV_GetServerThreadOwnsGame()==1024,"concurrent production increments retained");
        for(auto &thread:threads)thread=std::thread([]{for(int i=0;i<256;++i)SV_DecServerThreadOwnsGame();});
        for(auto &thread:threads)thread.join();
        check(SV_GetServerThreadOwnsGame()==0,"concurrent production decrements return ownership to zero");
    }
}
