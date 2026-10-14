// Native console wrapper; the SDK SDL entry uses the same real runtime body.
#include <platform/android/native_engine_runtime.h>
int main(int argc, char **argv) {
    return bo1::android::RunNativeEngine(bo1::android::NativeEngineEntry::DedicatedConsole,argc,argv);
}
