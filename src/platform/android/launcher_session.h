#pragma once
#include <string>
namespace bo1::android::launcher_session {
// Engine-side state the in-process launcher menu follows (EngineActivity polls it).
enum State { Booting = 0, Menu = 1, Loading = 2, Match = 3 };
// Engine main thread only.
void Publish(State state);
// Any thread.
State Read();
// Any thread. Map names are zone stems (lowercase letters, digits, underscore).
// Returns false for an invalid name; a later request replaces an untaken one.
bool RequestMap(const std::string &map);
// Engine main thread. Takes (and clears) the pending request.
bool TakeRequestedMap(std::string &map);
}
