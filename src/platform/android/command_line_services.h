#pragma once
namespace bo1::android {
// Install once before required entry setup. Storage is process-owned and stays
// distinct from the mutable command buffer consumed by Com_ParseCommandLine.
// GetCommandLineA can first read actual OS argv during global construction.
void InstallEngineCommandLine(const char *program, const char *engineArguments);
}
char *GetCommandLineA();
