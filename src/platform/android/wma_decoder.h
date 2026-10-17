#pragma once
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace bo1::android {
// Borrowed immutable canonical resident-asset packet and cumulative decoded-byte views.
struct WmaAssetView {
    unsigned channels, sampleRate, packetBytes, frameCount;
    std::span<const std::uint32_t> decodedPacketBytes;
    std::span<const std::uint8_t> packets;
};
struct DecodedWma {
    unsigned channels{}, sampleRate{};
    std::vector<float> samples; // Owned interleaved float frames; retain through SDK voice retirement.
    // Tolerant decode only: trailing frames the codec could not decode, zero-filled to keep the canonical length.
    unsigned zeroFilledFrames{};
};
// Actual WMAv2 decoding. Failure leaves output empty, never produces a silence-success substitute.
bool DecodeCanonicalWma(const WmaAssetView &asset, DecodedWma &output, std::string &error);
// Same decoder and the same canonical sample-count validation, but recoverable bitstream
// irregularities are tolerated like the original XAudio2 xWMA path (the strict form aborts
// on the first one). Used only after the strict decode fails.
bool DecodeCanonicalWmaTolerant(const WmaAssetView &asset, DecodedWma &output, std::string &error);
}
