#pragma once
namespace bo1::android {
void InitializeSystemOS();
void BootstrapNativePlatform();
void ShutdownInputIfInitialized();
}
void IN_Init();
void IN_Shutdown();
void IN_Activate(int active);
void IN_Frame_Native();
