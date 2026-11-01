#include "wma_decoder.h"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/mem.h>
#include <libavutil/samplefmt.h>
}
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <cstdio>

static_assert(LIBAVCODEC_VERSION_INT==AV_VERSION_INT(61,19,101));
namespace bo1::android {
namespace {
struct CodecDeleter { void operator()(AVCodecContext *p) const { avcodec_free_context(&p); } };
struct FrameDeleter { void operator()(AVFrame *p) const { av_frame_free(&p); } };
struct PacketDeleter { void operator()(AVPacket *p) const { av_packet_free(&p); } };
std::string CodecError(const char *operation, int code) {
    char message[AV_ERROR_MAX_STRING_SIZE]{};av_strerror(code,message,sizeof(message));
    return std::string(operation)+": "+message;
}
}
static bool DecodeWma(const WmaAssetView &asset, bool tolerant, DecodedWma &output, std::string &error) {
    output={};error.clear();
    auto Fail=[&](const char *message) { error=message;return false; };
    if((asset.channels!=1 && asset.channels!=2) || !asset.frameCount ||
        asset.sampleRate<1000 || asset.sampleRate>100000 ||
        asset.packetBytes!=(asset.channels==1?2230u:4096u) || asset.decodedPacketBytes.empty())
        return Fail("invalid canonical WMAv2 asset format");
    const std::uint64_t sampleCount=static_cast<std::uint64_t>(asset.frameCount)*asset.channels;
    if(sampleCount>std::numeric_limits<std::uint32_t>::max()/2 ||
        sampleCount>std::numeric_limits<std::size_t>::max()/sizeof(float) ||
        asset.decodedPacketBytes.size()>asset.packets.size()/asset.packetBytes)
        return Fail("canonical WMAv2 packet/sample bounds overflow");
    std::uint32_t prior=0;
    for(const auto value:asset.decodedPacketBytes) {
        if(value<prior || value%(asset.channels*2)) return Fail("invalid WMAv2 cumulative decoded-byte table");
        prior=value;
    }
    if(prior!=sampleCount*2) return Fail("WMAv2 final decoded bytes differ from canonical frame count");
    if(avcodec_version()!=LIBAVCODEC_VERSION_INT) return Fail("WMAv2 library differs from pinned decoder ABI");
    const auto *decoder=avcodec_find_decoder(AV_CODEC_ID_WMAV2);
    if(!decoder) return Fail("actual WMAv2 decoder is unavailable");
    std::unique_ptr<AVCodecContext,CodecDeleter> codec(avcodec_alloc_context3(decoder));
    std::unique_ptr<AVFrame,FrameDeleter> frame(av_frame_alloc());
    std::unique_ptr<AVPacket,PacketDeleter> packet(av_packet_alloc());
    if(!codec || !frame || !packet) return Fail("WMAv2 decoder allocation failed");
    codec->sample_rate=asset.sampleRate;codec->block_align=asset.packetBytes;
    codec->bit_rate=48000*asset.channels;codec->bits_per_coded_sample=16;
    // Match upstream xwma demuxer's bitrate normalization for canonical nominal headers.
    if(asset.channels==1 && (asset.sampleRate==22050 || asset.sampleRate==32000)) codec->bit_rate=20000;
    av_channel_layout_default(&codec->ch_layout,asset.channels);
    // xWMA deliberately carries no WAVEFORMATEX extra bytes. Use the actual
    // upstream xwma.c codec flags (31), not WMAv2's incompatible no-extra defaults.
    codec->extradata_size=6;
    codec->extradata=static_cast<std::uint8_t *>(av_mallocz(6+AV_INPUT_BUFFER_PADDING_SIZE));
    if(!codec->extradata) return Fail("WMAv2 codec flag allocation failed");
    codec->extradata[4]=31;
    codec->err_recognition=tolerant?0:(AV_EF_CAREFUL|AV_EF_EXPLODE);

    int result=avcodec_open2(codec.get(),decoder,nullptr);
    if(result<0) { error=CodecError("open actual WMAv2 decoder",result);return false; }
    DecodedWma decoded;decoded.channels=asset.channels;decoded.sampleRate=asset.sampleRate;
    try { decoded.samples.reserve(static_cast<std::size_t>(sampleCount)); }
    catch(const std::bad_alloc &) { return Fail("WMAv2 decoded sample allocation failed"); }
    std::uint64_t actualFrames=0;
    std::size_t packetsSent=0;
    auto Where=[&]()->std::string {
        return " [packets sent "+std::to_string(packetsSent)+"/"+std::to_string(asset.decodedPacketBytes.size())
            +", decoded frames "+std::to_string(actualFrames)+"/"+std::to_string(asset.frameCount)
            +", "+std::to_string(asset.channels)+"ch "+std::to_string(asset.sampleRate)+"Hz]";
    };
    // Tolerant mode only: the codec rejects the final transform block of some xWMA sounds. The
    // original XAudio2 path plays what it can decode, so accept a failure that leaves less than
    // two transform blocks (4096 frames) missing, zero-fill the tail to the canonical length and
    // report exactly how much was filled. Anything larger still fails.
    auto AcceptTail=[&]()->bool {
        if(!tolerant || !actualFrames || actualFrames>=asset.frameCount || asset.frameCount-actualFrames>4096) return false;
        decoded.samples.resize(static_cast<std::size_t>(sampleCount),0.0f);
        decoded.zeroFilledFrames=static_cast<unsigned>(asset.frameCount-actualFrames);
        error.clear();output=std::move(decoded);return true;
    };
    auto Drain=[&](bool final)->bool {
        for(;;) {
            const int status=avcodec_receive_frame(codec.get(),frame.get());
            if(status==AVERROR(EAGAIN)) { if(final)error="WMAv2 final drain unexpectedly requires another packet";return !final; }
            if(status==AVERROR_EOF) return true;
            if(status<0) { error=CodecError("decode actual WMAv2 frame",status)+Where();return false; }
            if(frame->format!=AV_SAMPLE_FMT_FLTP || frame->ch_layout.nb_channels!=static_cast<int>(asset.channels) ||
                frame->sample_rate!=static_cast<int>(asset.sampleRate) || frame->nb_samples<0 || !frame->extended_data)
                { error="WMAv2 decoder returned incompatible sample layout";return false; }
            actualFrames+=static_cast<unsigned>(frame->nb_samples);
            // Codec trailing transform padding may exceed the canonical dpds
            // endpoint by less than two2048-frame transform blocks. Never pad a short decode.
            if(actualFrames>static_cast<std::uint64_t>(asset.frameCount)+4096)
                { error="WMAv2 decode exceeds canonical endpoint and transform padding";return false; }
            for(int sample=0;sample<frame->nb_samples;++sample)
                for(unsigned channel=0;channel<asset.channels;++channel) {
                    if(!frame->extended_data[channel]) { error="WMAv2 decoder returned null sample plane";return false; }
                    const float value=reinterpret_cast<const float *>(frame->extended_data[channel])[sample];
                    if(!std::isfinite(value)) { error="WMAv2 decoder returned nonfinite audio";return false; }
                    if(decoded.samples.size()<sampleCount) decoded.samples.push_back(value);
                }
            av_frame_unref(frame.get());
        }
    };
    try {
        for(std::size_t index=0;index<asset.decodedPacketBytes.size();++index) {
            result=av_new_packet(packet.get(),asset.packetBytes);
            if(result<0) { error=CodecError("allocate padded WMAv2 packet",result);return false; }
            std::memcpy(packet->data,asset.packets.data()+index*asset.packetBytes,asset.packetBytes);
            result=avcodec_send_packet(codec.get(),packet.get());av_packet_unref(packet.get());
            if(result<0) { error=CodecError("submit actual WMAv2 packet",result)+Where();return false; }
            ++packetsSent;
            if(!Drain(false)) return AcceptTail();
        }
        result=avcodec_send_packet(codec.get(),nullptr);
        if(result<0 && result!=AVERROR_EOF) { error=CodecError("drain actual WMAv2 decoder",result);return false; }
        if(!Drain(true)) return AcceptTail();
    } catch(const std::bad_alloc &) { return Fail("WMAv2 decoded sample allocation failed"); }
    if(decoded.samples.size()!=sampleCount) { error="WMAv2 decoder produced fewer frames than canonical endpoint: actual="+std::to_string(actualFrames)+" expected="+std::to_string(asset.frameCount);return false; }
    output=std::move(decoded);return true;
}
bool DecodeCanonicalWma(const WmaAssetView &asset, DecodedWma &output, std::string &error) {
    return DecodeWma(asset,false,output,error);
}
bool DecodeCanonicalWmaTolerant(const WmaAssetView &asset, DecodedWma &output, std::string &error) {
    return DecodeWma(asset,true,output,error);
}
bool DecodeCanonicalMsAdpcm(const MsAdpcmAssetView &asset, DecodedMsAdpcm &output, std::string &error) {
    output={};error.clear();
    auto Fail=[&](const char *message) { error=message;return false; };
    if((asset.channels!=1 && asset.channels!=2) || !asset.frameCount ||
        asset.sampleRate<1000 || asset.sampleRate>100000)
        return Fail("invalid canonical MSADPCM metadata");
    const unsigned blockBytes=262*asset.channels;
    const std::uint64_t sampleCount=static_cast<std::uint64_t>(asset.frameCount)*asset.channels;
    if(asset.blocks.empty() || asset.blocks.size()%blockBytes ||
        asset.blocks.size()/blockBytes*512ull!=asset.frameCount ||
        sampleCount>std::numeric_limits<std::uint32_t>::max()/sizeof(std::int16_t))
        return Fail("incomplete or padded canonical MSADPCM extent");
    // FAudio 25.02 uses signed 16-bit adaptive deltas; FFmpeg uses int.
    // Refuse an asset whose delta would wrap in FAudio rather than changing its
    // audible waveform during a checkpoint. All retained samples must agree.
    static constexpr unsigned adaptation[16]={230,230,230,230,307,409,512,614,768,614,512,409,307,230,230,230};
    for(std::size_t offset=0;offset<asset.blocks.size();offset+=blockBytes) {
        const auto *block=asset.blocks.data()+offset;
        unsigned delta[2]{};
        for(unsigned channel=0;channel<asset.channels;++channel) {
            delta[channel]=block[asset.channels+channel*2] | (block[asset.channels+channel*2+1]<<8);
            if(block[channel]>=7 || delta[channel]<16 || delta[channel]>32767)
                return Fail("MSADPCM predictor/delta differs from pinned SDK bounds");
        }
        for(unsigned byte=7*asset.channels;byte<blockBytes;++byte) {
            for(unsigned half=0;half<2;++half) {
                const auto channel=asset.channels==1?0:half;
                const auto nibble=half?block[byte]&15:block[byte]>>4;
                delta[channel]=delta[channel]*adaptation[nibble]/256;
                if(delta[channel]>32767) return Fail("MSADPCM adaptive delta wraps in pinned SDK");
                if(delta[channel]<16) delta[channel]=16;
            }
        }
    }
    if(avcodec_version()!=LIBAVCODEC_VERSION_INT) return Fail("MSADPCM library differs from pinned decoder ABI");
    const auto *decoder=avcodec_find_decoder(AV_CODEC_ID_ADPCM_MS);
    if(!decoder) return Fail("actual MSADPCM decoder is unavailable");
    std::unique_ptr<AVCodecContext,CodecDeleter> codec(avcodec_alloc_context3(decoder));
    std::unique_ptr<AVFrame,FrameDeleter> frame(av_frame_alloc());
    std::unique_ptr<AVPacket,PacketDeleter> packet(av_packet_alloc());
    if(!codec || !frame || !packet) return Fail("MSADPCM decoder allocation failed");
    codec->sample_rate=asset.sampleRate;codec->block_align=blockBytes;codec->bits_per_coded_sample=4;
    av_channel_layout_default(&codec->ch_layout,asset.channels);
    codec->err_recognition=AV_EF_CAREFUL|AV_EF_EXPLODE;
    int result=avcodec_open2(codec.get(),decoder,nullptr);
    if(result<0) { error=CodecError("open actual MSADPCM decoder",result);return false; }
    DecodedMsAdpcm decoded;
    try {
        decoded.samples.reserve(static_cast<std::size_t>(sampleCount));
        for(std::size_t offset=0;offset<asset.blocks.size();offset+=blockBytes) {
            result=av_new_packet(packet.get(),blockBytes);
            if(result<0) { error=CodecError("allocate padded MSADPCM packet",result);return false; }
            std::memcpy(packet->data,asset.blocks.data()+offset,blockBytes);
            result=avcodec_send_packet(codec.get(),packet.get());av_packet_unref(packet.get());
            if(result<0) { error=CodecError("submit actual MSADPCM packet",result);return false; }
            result=avcodec_receive_frame(codec.get(),frame.get());
            if(result<0) { error=CodecError("decode actual MSADPCM block",result);return false; }
            if((frame->format!=AV_SAMPLE_FMT_S16 && frame->format!=AV_SAMPLE_FMT_S16P) ||
                frame->ch_layout.nb_channels!=static_cast<int>(asset.channels) || frame->nb_samples!=512 ||
                frame->sample_rate!=static_cast<int>(asset.sampleRate) || !frame->extended_data)
                return Fail("MSADPCM decoder returned incompatible frame layout");
            for(unsigned channel=0;channel<(frame->format==AV_SAMPLE_FMT_S16P?asset.channels:1);++channel)
                if(!frame->extended_data[channel]) return Fail("MSADPCM decoder returned null sample plane");
            for(unsigned sample=0;sample<512;++sample)
                for(unsigned channel=0;channel<asset.channels;++channel) {
                    const auto plane=frame->format==AV_SAMPLE_FMT_S16P?channel:0;
                    const auto index=frame->format==AV_SAMPLE_FMT_S16P?sample:sample*asset.channels+channel;
                    std::int16_t value;std::memcpy(&value,frame->extended_data[plane]+index*sizeof(value),sizeof(value));
                    decoded.samples.push_back(value);
                }
            av_frame_unref(frame.get());
            result=avcodec_receive_frame(codec.get(),frame.get());
            if(result!=AVERROR(EAGAIN)) return Fail("MSADPCM block produced unexpected extra output");
        }
        result=avcodec_send_packet(codec.get(),nullptr);
        if(result<0 || avcodec_receive_frame(codec.get(),frame.get())!=AVERROR_EOF)
            return Fail("MSADPCM final drain differs from complete block extent");
    } catch(const std::bad_alloc &) { return Fail("MSADPCM decoded sample allocation failed"); }
    if(decoded.samples.size()!=sampleCount) return Fail("MSADPCM decoded sample count differs from canonical extent");
    output=std::move(decoded);return true;
}
}
