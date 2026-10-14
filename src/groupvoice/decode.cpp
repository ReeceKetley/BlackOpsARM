#include "decode.h"
#include <qcommon/common.h>
#include <speex/speex.h>
#include "encode.h"

void *g_decoder;
int g_decode_frame_size;
int g_current_decode_bandwidth_setting;
SpeexBits decodeBits;

char __cdecl Decode_Init(int bandwidthEnum)
{
    const SpeexMode *mode; // [esp+8h] [ebp-4h]

    if (bandwidthEnum)
    {
        if (bandwidthEnum == 1)
        {
            mode = &speex_wb_mode;
        }
        else
        {
            if (bandwidthEnum != 2)
            {
                Com_Printf(9, "Unknown bandwidth mode %i\n", bandwidthEnum);
                return 0;
            }
            mode = &speex_uwb_mode;
        }
    }
    else
    {
        mode = &speex_nb_mode;
    }
#if defined(__ANDROID__) || defined(BO1_NATIVE_FAUDIO_HOST_TEST)
    if (g_decoder) return 0;
#endif
    g_decoder = speex_decoder_init(mode);
#if defined(__ANDROID__) || defined(BO1_NATIVE_FAUDIO_HOST_TEST)
    if (!g_decoder) return 0;
#endif
    int tmp=1;
#if defined(__ANDROID__) || defined(BO1_NATIVE_FAUDIO_HOST_TEST)
    if(speex_decoder_ctl(g_decoder,SPEEX_SET_ENH,&tmp) ||
        speex_decoder_ctl(g_decoder,SPEEX_SET_SAMPLING_RATE,&g_encoder_samplerate) ||
        speex_decoder_ctl(g_decoder,SPEEX_GET_FRAME_SIZE,&g_decode_frame_size) ||
        g_decode_frame_size<=0 || g_decode_frame_size>640)
    {speex_decoder_destroy(g_decoder);g_decoder=nullptr;g_decode_frame_size=0;return 0;}
#else
    speex_decoder_ctl(g_decoder, SPEEX_SET_ENH, &tmp);
    Decode_SetOptions();
    speex_decoder_ctl(g_decoder, SPEEX_GET_FRAME_SIZE, &g_decode_frame_size);
#endif
    g_current_decode_bandwidth_setting = bandwidthEnum;
    speex_bits_init(&decodeBits);
#if defined(__ANDROID__) || defined(BO1_NATIVE_FAUDIO_HOST_TEST)
    if(!decodeBits.chars){Decode_Shutdown();return 0;}
#endif
    return 1;
}

void __cdecl Decode_SetOptions()
{
    speex_decoder_ctl(g_decoder, SPEEX_SET_SAMPLING_RATE, &g_encoder_samplerate);
}

void __cdecl Decode_Shutdown()
{
    if (g_decoder)
    {
        speex_bits_destroy(&decodeBits);
        speex_decoder_destroy(g_decoder);
    }
    g_decoder = 0;
#if defined(__ANDROID__) || defined(BO1_NATIVE_FAUDIO_HOST_TEST)
    g_decode_frame_size=0;
#endif
}

int __cdecl Decode_Sample(char *buffer, int maxLength, __int16 *out, int frame_size)
{
#if defined(__ANDROID__) || defined(BO1_NATIVE_FAUDIO_HOST_TEST)
    if (!g_decoder || !buffer || !out || maxLength <= 0 || maxLength > 4096 ||
        frame_size != g_decode_frame_size || frame_size <= 0 || frame_size > 640 || maxLength > frame_size)
        return 0;
    speex_bits_read_from(&decodeBits, buffer, maxLength);
    if (speex_decode_int(g_decoder, &decodeBits, out) != 0) return 0;
    return frame_size * static_cast<int>(sizeof(*out)); // byte count, not number of output shorts
#else
    int v5; // [esp+0h] [ebp-400Ch]
    float v6[4097]; // [esp+4h] [ebp-4008h]
    int i; // [esp+4008h] [ebp-4h]

    iassert(maxLength <= 4096);
    iassert(maxLength <= frame_size);

    speex_bits_read_from(&decodeBits, buffer, maxLength);
    if (speex_decode(g_decoder, &decodeBits, v6))
        v5 = 0;
    else
        v5 = 2 * frame_size;
    for ( i = 0; i < v5; ++i )
        out[i] = (int)v6[i];
    return v5;
#endif
}

