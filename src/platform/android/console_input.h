#pragma once
namespace bo1::android {
// The calling thread owns reading this stream; no other consumer may read it.
void InstallConsoleInput(int descriptor);
void RetireConsoleInput();
bool ConsoleInputOwnedByCaller();
void PollConsoleInput();
}
void Platform_InputFrame();
