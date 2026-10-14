#include <physics/phys_main.h>
#include <physics/phys_mem_new.h>
#include <physics/physics_system_internal.h>
#include <cstdio>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>
#include <csignal>
#include <cstring>
// Observation seam: deliberately decline the diagnostic break. Production's
// explicit failure must still terminate without publishing an invalid arena.
extern void *g_phys_memory_buffer;
extern int g_phys_memory_buffer_size;
extern phys_memory_manager *g_phys_memory_manager;
static bool checkingUnpublished;
char _tlAssert(const char *,int,const char *,const char *) {
    if(checkingUnpublished && (g_phys_memory_manager || g_phys_memory_buffer || g_phys_memory_buffer_size)) _Exit(91);
    return 0;
}
bool Sys_IsHeadless(){return true;}
bool Monkey_IsRunning(){return false;}
static void check(bool ok,const char *what){if(!ok){std::fprintf(stderr,"FAIL %s\n",what);std::abort();}}
static void badSpan(void *p,int n){
    const pid_t child=fork();check(child>=0,"fork");
    if(!child){checkingUnpublished=true;phys_memory_manager_init(p,n);_Exit(92);}
    int status;check(waitpid(child,&status,0)==child,"wait");
    check(WIFSIGNALED(status)&&WTERMSIG(status)==SIGABRT,"invalid span rejects without publication even when assert declines break");
}
int main(){
    alignas(16) static unsigned char arena[3670016+16];
    badSpan(nullptr,3670016);badSpan(arena,0);badSpan(arena,static_cast<int>(sizeof(phys_memory_manager)));
    badSpan(reinterpret_cast<void *>(UINTPTR_MAX-8),32);
    // Deliberately unaligned owner, with dirty storage rather than accidental zeros.
    std::memset(arena,0xcd,sizeof(arena));
    phys_memory_manager_init(arena+1,3670016);
    check(g_phys_memory_buffer==arena+1 && g_phys_memory_buffer_size==3670016,"exact original span retained");
    check(g_phys_memory_manager && reinterpret_cast<uintptr_t>(g_phys_memory_manager)%alignof(phys_memory_manager)==0,"manager aligned");
    for(auto *entry:g_phys_memory_manager->m_slot_pool_map.m_hash_table)check(!entry,"native pointer map complete");
    check(g_phys_memory_manager->m_buffer_start==reinterpret_cast<char *>(g_phys_memory_manager)+sizeof(phys_memory_manager),"payload begins after actual native manager");
    const auto before=g_phys_memory_manager->m_buffer_cur;
    // Canonical permanent topology only: algorithms are exercised by production.
    auto *sys=PMM_PERM_ALLOCATE(sizeof(physics_system),alignof(physics_system));
    auto *bpm=PMM_PERM_ALLOCATE(sizeof(broad_phase_memory),alignof(broad_phase_memory));
    PMM_PERM_ALLOCATE(4,4); // Force an intervening non-eight-byte cursor.
    auto *sap=PMM_PERM_ALLOCATE(sizeof(axis_aligned_sweep_and_prune),alignof(axis_aligned_sweep_and_prune));
    auto *surfaces=PMM_PERM_ALLOCATE(630*sizeof(phys_surface_type_info),alignof(phys_surface_type_info));
    check(reinterpret_cast<uintptr_t>(sys)%16==0 && reinterpret_cast<uintptr_t>(bpm)%8==0 && reinterpret_cast<uintptr_t>(sap)%8==0 && reinterpret_cast<uintptr_t>(surfaces)%4==0,"actual native topology alignment");
    check(g_phys_memory_manager->m_buffer_cur-before<50000,"canonical startup fits unchanged arena");
    auto child=fork();check(child>=0,"oom fork");
    if(!child){PMM_PERM_ALLOCATE(3670016,16);_Exit(93);}
    int status;check(waitpid(child,&status,0)==child,"oom wait");check(WIFEXITED(status)&&WEXITSTATUS(status)==3,"actual arena exhaustion uses terminal OOM owner");
    child=fork();check(child>=0,"reinit fork");
    if(!child){phys_memory_manager_init(arena,3670016);_Exit(94);}
    check(waitpid(child,&status,0)==child,"reinit wait");check(WIFSIGNALED(status)&&WTERMSIG(status)==SIGABRT,"duplicate owner rejects");
    std::puts("PASS physics arena initialization: four invalid spans, complete native manager, canonical topology alignment/budget, exhaustion and duplicate owner");
}
