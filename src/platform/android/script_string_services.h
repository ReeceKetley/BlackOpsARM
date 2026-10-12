#pragma once

namespace bo1::script {
// Independent recursive sections. This is not the global Win32 section enum.
enum class PoolLock { Strings, MemoryTree, Database };
void InitializeServices();
void Lock(PoolLock);
bool TryLock(PoolLock);
void Unlock(PoolLock);
// An unset server role compares false. Registration by the same thread is
// idempotent; a different thread cannot replace a registered role. Identity is
// immutable for the process, with no unregister/reset API. The registered server
// thread must remain alive whenever RegisterServerThread/IsServerThread is called
// after publication (pthread identities can be invalid/reused after exit).
void RegisterServerThread();
bool IsServerThread();
}
