#pragma once
namespace bo1::android {
// The calling thread owns reading this stream; no other consumer may read it.
// requireSlash is reserved for the private automatic-test FIFO.
void InstallConsoleInput(int descriptor, bool requireSlash=false);
void RetireConsoleInput();
bool ConsoleInputOwnedByCaller();
void PollConsoleInput();
}
void Platform_InputFrame();
