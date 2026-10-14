#pragma once
#include "faudio_bridge.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <deque>
#include <mutex>
#include <span>
#include <string>
#include <vector>
#include <SDL.h>
namespace bo1::android {
// The callback's actual PCM is copied into this bounded frame ring. Overflow is counted.
class VoiceCaptureRing {
    std::array<std::int16_t,25600> frames{};
    std::mutex gate;
    std::size_t first{},count{};
    std::uint64_t overruns{};
public:
    void Push(std::span<const std::int16_t> input);
    std::size_t Read(std::span<std::int16_t> output);
    void Clear();
    std::uint64_t Overruns();
};
class VoiceCapture {
    SDL_AudioDeviceID device{};
    SDL_AudioStream *conversion{};
    std::string route;
    std::atomic<bool> failed{};
    VoiceCaptureRing ring;
    std::atomic<bool> muted{true};
    std::atomic<unsigned> gain{65535};
    static void Callback(void *owner,std::uint8_t *data,int bytes);
public:
    ~VoiceCapture();
    bool Open(unsigned rate,unsigned channels);
    bool Start();
    bool Stop();
    std::ptrdiff_t Read(std::span<std::int16_t> output);
    bool Controls(unsigned level,bool mute);
    bool Route(char *output,std::size_t capacity);
    bool Live() const;
};
class VoicePlayback final:public IXAudio2VoiceCallback {
    struct Packet {std::vector<std::int16_t> frames;std::atomic<bool> ended{};};
    IXAudio2 *engine;
    IXAudio2Voice *output;
    IXAudio2SourceVoice *voice{};
    std::deque<Packet> packets;
    std::vector<std::int16_t> pending;
    unsigned frequency,channels;
    int jitterMode{2};
    std::uint64_t queuedFrames{};
    std::atomic<bool> failed{};
    bool Create();
public:
    VoicePlayback(IXAudio2 *owner,IXAudio2Voice *destination,unsigned rate,unsigned channelCount);
    ~VoicePlayback();
    bool Live() const;
    int Queue(const std::int16_t *data,unsigned bytes,unsigned maximumBytes);
    int BufferedBytes();
    bool Frame(unsigned startBytes,unsigned lowBytes,unsigned highBytes,unsigned comfortableBytes,float up,float down);
    bool Stop();
    bool Playing() const { return playing; }
    bool playing{};
    void OnVoiceProcessingPassStart(UINT32) override {}
    void OnVoiceProcessingPassEnd() override {}
    void OnStreamEnd() override {}
    void OnBufferStart(void *) override {}
    void OnBufferEnd(void *context) override;
    void OnLoopEnd(void *) override {}
    void OnVoiceError(void *,HRESULT) override {failed=true;}
};
}
