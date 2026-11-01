#pragma once
#include <cstdint>

namespace bo1::android {
// Android's 24c6:581a driver initially publishes zero for untouched signed
// triggers. SDL maps that to half pressed. -32768 is SDL's synthetic initial
// release; real Android samples span -32767..32767. Do not calibrate against
// either placeholder. Once a real sample arrives, preserve the complete range.
struct TriggerStartup {
    bool observed{};
    std::int16_t Filter(std::int16_t raw, std::int16_t mapped) {
        if (raw != 0 && raw != -32768) observed = true;
        return observed ? mapped : 0;
    }
};
}
