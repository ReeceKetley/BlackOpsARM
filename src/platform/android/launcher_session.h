#pragma once
#include <string>
namespace bo1::android::launcher_session {
// Engine-side state the in-process launcher menu follows (EngineActivity polls it).
enum State { Booting = 0, Menu = 1, Loading = 2, Match = 3, RecoveryMenu = 4 };
// Engine main thread only.
void Publish(State state);
// Any thread.
State Read();
// Any thread. Map names are zone stems (lowercase letters, digits, underscore).
// Returns false for an invalid name; a later request replaces an untaken one.
bool RequestMap(const std::string &map);
// Engine main thread. Takes (and clears) the pending request.
bool TakeRequestedMap(std::string &map);
// Immutable device strings copied from the active renderer; safe on any thread.
void PublishGraphics(const std::string &name, const std::string &api, const std::string &driver);
struct ResumeInfo {
    bool available{}, canSave{}, paused{}, busy{}, loading{}, success{};
    unsigned result{};
    std::string map, message;
};
// Main thread publishes owned metadata; UI threads only queue typed requests.
void PublishResumeInfo(const ResumeInfo &info);
bool RequestResumeAction(int action); // 1 save, 2 save and leave, 3 resume
int TakeResumeAction();
bool ResumeLoading();
std::string ResumeInfoJson();
}
