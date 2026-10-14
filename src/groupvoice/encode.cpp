#include "encode.h"
#include <speex/speex.h>
#include <qcommon/common.h>
#include <server_mp/sv_init_mp.h>

void *g_encoder;
SpeexBits encodeBits;
int g_frame_size;
int g_encoder_quality = 1;
int g_encoder_samplerate = 8000;
int g_speex_initialized;
int g_current_bandwidth_setting;


void __cdecl Encode_SetOptions(int frequency, int quality)
{
    //bool yes; // [esp+3h] [ebp-1h] BYREF
    int yes; // this needs to be 4byte, otherwise ASAN chimps out

#if defined(__ANDROID__) || defined(BO1_NATIVE_FAUDIO_HOST_TEST)
    yes=1;
    if(!g_encoder || quality<0 || quality>10 || frequency<=0 ||
        speex_encoder_ctl(g_encoder,SPEEX_SET_SAMPLING_RATE,&frequency) ||
        speex_encoder_ctl(g_encoder,SPEEX_SET_QUALITY,&quality) ||
        speex_encoder_ctl(g_encoder,SPEEX_GET_FRAME_SIZE,&g_frame_size) ||
        speex_encoder_ctl(g_encoder,SPEEX_SET_VAD,&yes) ||
        speex_encoder_ctl(g_encoder,SPEEX_SET_DTX,&yes))
    {g_frame_size=0;Com_PrintError(9,"Actual Speex encoder options failed\n");return;}
#else
    yes = 1;
    speex_encoder_ctl(g_encoder, 24, &frequency);
    speex_encoder_ctl(g_encoder, 4, &quality);
    speex_encoder_ctl(g_encoder, 3, &g_frame_size);
    speex_encoder_ctl(g_encoder, 30, &yes);
    speex_encoder_ctl(g_encoder, 34, &yes);
#endif
    g_encoder_quality = quality;
    g_encoder_samplerate = frequency;
}

bool __cdecl Encode_Init(int bandwidthEnum)
{
#if defined(__ANDROID__) || defined(BO1_NATIVE_FAUDIO_HOST_TEST)
    if (g_encoder || g_speex_initialized) return false;
#endif
    void *v1; // eax

    if ( bandwidthEnum )
    {
        if ( bandwidthEnum == 1 )
        {
            v1 = (void *)speex_encoder_init(&speex_wb_mode);
        }
        else
        {
            if ( bandwidthEnum != 2 )
            {
                Com_Printf(9, "Unknown bandwidth mode %i\n", bandwidthEnum);
                return 0;
            }
            v1 = (void *)speex_encoder_init(&speex_uwb_mode);
        }
    }
    else
    {
        v1 = (void *)speex_encoder_init(&speex_nb_mode);
    }
    g_encoder = v1;
#if defined(__ANDROID__) || defined(BO1_NATIVE_FAUDIO_HOST_TEST)
    if (!g_encoder) return false;
#endif
    speex_bits_init(&encodeBits);
    g_speex_initialized = 1;
    Encode_SetOptions(g_encoder_samplerate, g_encoder_quality);
#if defined(__ANDROID__) || defined(BO1_NATIVE_FAUDIO_HOST_TEST)
    if(g_frame_size<=0 || g_frame_size>640 || !encodeBits.chars){Encode_Shutdown();return false;}
#else
    speex_encoder_ctl(g_encoder, SPEEX_GET_FRAME_SIZE, &g_frame_size);
#endif
    g_current_bandwidth_setting = bandwidthEnum;
    return g_encoder != 0;
}

char __cdecl Encode_Shutdown()
{
    if (g_encoder)
        speex_encoder_destroy(g_encoder);
    g_encoder = 0;
    if (g_speex_initialized)
    {
        speex_bits_destroy(&encodeBits);
        g_speex_initialized = 0;
    }
#if defined(__ANDROID__) || defined(BO1_NATIVE_FAUDIO_HOST_TEST)
    g_frame_size=0;
#endif
    return 1;
}

int __cdecl Encode_Sample(__int16 *buffer_in, char *buffer_out, int maxLength)
{
#if defined(__ANDROID__) || defined(BO1_NATIVE_FAUDIO_HOST_TEST)
    if (!g_encoder || g_frame_size<=0 || !buffer_in || !buffer_out || maxLength <= 0 || !sv_voiceQuality) return -1;
#endif
    if (sv_voiceQuality->current.integer != g_encoder_quality)
    {
        #if defined(__ANDROID__) || defined(BO1_NATIVE_FAUDIO_HOST_TEST)
        int requestedQuality=sv_voiceQuality->current.integer;
        if(requestedQuality<0 || requestedQuality>10 || speex_encoder_ctl(g_encoder,SPEEX_SET_QUALITY,&requestedQuality))return -1;
        g_encoder_quality=requestedQuality;
#else
        g_encoder_quality = sv_voiceQuality->current.integer;
        speex_encoder_ctl(g_encoder, SPEEX_SET_QUALITY, &g_encoder_quality);
#endif
    }
    speex_bits_reset(&encodeBits);
#if defined(__ANDROID__) || defined(BO1_NATIVE_FAUDIO_HOST_TEST)
    if (speex_encode_int(g_encoder, buffer_in, &encodeBits) < 0 || speex_bits_nbytes(&encodeBits) > maxLength) return -1;
#else
    speex_encode_int(g_encoder, buffer_in, &encodeBits);
#endif
    return speex_bits_write(&encodeBits, buffer_out, maxLength);
}

int __cdecl Encode_GetFrameSize()
{
    return g_frame_size;
}

