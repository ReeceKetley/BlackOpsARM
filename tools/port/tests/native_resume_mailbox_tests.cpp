#include <platform/android/launcher_session.cpp>
#include <cassert>
#include <thread>

using namespace bo1::android::launcher_session;
int main() {
    ResumeInfo info;
    PublishResumeInfo(info);
    assert(!RequestResumeAction(0) && !RequestResumeAction(4));
    assert(!RequestResumeAction(1) && !RequestResumeAction(3));
    info.available=true; info.canSave=true; info.map="zombie_theater";
    info.message="quoted\" slash\\ newline\n"; info.result=17;
    PublishResumeInfo(info); info.map="changed caller storage";
    const auto json=ResumeInfoJson();
    assert(json.find("zombie_theater")!=std::string::npos);
    assert(json.find("quoted\\\" slash\\\\ newline\\n")==std::string::npos); // control characters use unicode escapes
    assert(json.find("\\u000a")!=std::string::npos);
    assert(RequestResumeAction(3) && !RequestResumeAction(2));
    PublishResumeInfo(info); // A publication cannot erase an untaken request.
    assert(!RequestResumeAction(1) && TakeResumeAction()==3 && TakeResumeAction()==0);
    info.busy=true;PublishResumeInfo(info);assert(!RequestResumeAction(1));
    info.busy=false;info.loading=true;PublishResumeInfo(info);assert(ResumeLoading());
    info.loading=false;PublishResumeInfo(info);assert(!ResumeLoading());
    std::atomic<unsigned> accepted{}, consumed{};
    std::atomic<bool> done{};
    std::thread ui([&] {
        while(accepted.load()<2000) {
            if(RequestResumeAction(1))++accepted;
            else std::this_thread::yield();
        }
        done=true;
    });
    while(!done.load() || consumed.load()<accepted.load()) {
        const int action=TakeResumeAction();
        assert(action==0 || action==1);
        if(action)++consumed;
        PublishResumeInfo(info);
        std::this_thread::yield();
    }
    ui.join();assert(accepted==2000 && consumed==2000 && TakeResumeAction()==0);
    std::printf("actual resume mailbox 2000 concurrent requests, type/gate/ownership/JSON checks PASS\nJSON:%s\n",json.c_str());
}
