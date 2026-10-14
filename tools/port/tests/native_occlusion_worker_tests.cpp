#include <platform/android/occlusion_worker.h>
#include <platform/android/thread_services.h>
#include <atomic>
#include <cstdio>
#include <cstdlib>
extern "C" void __cyg_profile_func_enter(void *,void *) {}
extern "C" void __cyg_profile_func_exit(void *,void *) {}
static std::atomic<int> payload{};
static void check(bool value,const char *reason) {
    if(!value) { std::fprintf(stderr,"FAIL %s\n",reason); std::abort(); }
}
static void callback(unsigned context) {
    check(context==THREAD_CONTEXT_OCCLUSION && Sys_GetThreadContext()==THREAD_CONTEXT_OCCLUSION,"role and callback context");
    check(!Sys_GetValue(0),"fresh role TLS");
    Sys_SetValue(0,&payload);
    Sys_WaitForOcclusion();
    payload.fetch_add(1);
}
int main() {
    Sys_InitMainThread();
    for(int cycle=0;cycle<2;++cycle) {
        Sys_SpawnOcclusion(callback);
        check(!Sys_WaitForSingleObjectTimeout(&occlusionEvent,0),"initial unsignaled auto event");
        check(!bo1::android::WaitForOcclusionThreadRetirement(0),"live callback not retired");
        payload.store(73); Sys_SetOcclusion();
        check(bo1::android::WaitForOcclusionThreadRetirement(2000),"actual callback retired");
        bo1::android::JoinRetiredOcclusionThread();
        check(payload.load()==74,"payload published across worker join");
        check(!Sys_WaitForSingleObjectTimeout(&occlusionEvent,0),"auto signal consumed");
    }
    std::puts("PASS native occlusion worker: role11 TLS, real event, payload, retirement/join and two reuse cycles");
}
