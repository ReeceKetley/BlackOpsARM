#include <win32/win_tasks.h>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <thread>

bool Assert_MyHandler(const char *file,int line,int,const char *,...) {
    std::fprintf(stderr,"Task fixture assertion %s:%d\n",file,line);
    std::abort();
}
struct ReferenceTarget : bdReferencable {
    std::atomic<unsigned> *retired;
    explicit ReferenceTarget(std::atomic<unsigned> &count):retired(&count) { m_refCount=0; }
    ~ReferenceTarget() override { retired->fetch_add(1); }
};
int main() {
    std::atomic<unsigned> retired{};
    alignas(ReferenceTarget) unsigned char storage[sizeof(ReferenceTarget)];
    auto *target=new(storage) ReferenceTarget(retired);
    bdReference<ReferenceTarget> seed{};
    seed=target;
    auto worker=[&] {
        bdReference<ReferenceTarget> first{},second{};
        for(unsigned i=0;i<20000;++i) {
            first=target;second=first;first=nullptr;second=nullptr;
        }
    };
    std::thread first(worker),second(worker),third(worker);
    first.join();second.join();third.join();
    assert(target->m_refCount==1 && retired.load()==0);
    seed=nullptr;
    assert(retired.load()==1 && seed.m_ptr==nullptr);

    overlappedTask tasks[32]{};
    for(auto &task:tasks) {
        task.active=true;task.controllerIndex=0;task.type=17;
        task.errorCode=static_cast<bdLobbyErrorCode>(1);task.data=&retired;
        task.miscData=&retired;task.startMS=123;
    }
    TaskManager_ClearOverlappedTasks(tasks);
    assert(!TaskManager_AnyTaskInProgress(tasks));
    for(const auto &task:tasks) {
        assert(!task.active && !task.overlappedIO.m_ptr && !task.data && !task.miscData);
        assert(task.type==0 && task.controllerIndex==0 && task.startMS==0);
        assert(task.errorCode==BD_NO_ERROR && task.finalStatus==bdRemoteTask::BD_EMPTY);
    }
    auto *slot=TaskManager_GetOpenTaskSlot(tasks,0,7);
    assert(slot==tasks && slot->active && slot->type==7);
    assert(TaskManager_AnyTaskInProgress(tasks) && TaskManager_TaskIsInProgress(tasks,7));
    TaskManager_ClearTask(slot);
    assert(!TaskManager_AnyTaskInProgress(tasks));
    std::puts("PASS actual fixed32 atomic references and canonical 32-slot task lifecycle");
}
