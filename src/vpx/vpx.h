#pragma once
#include <vpx/vpx_codec.h>
#include <vpx/vpx_encoder.h>
#include <cstdio>

void __cdecl vpx_init(const char *filename, int width, int height);
void __cdecl write_ivf_file_header(FILE *outfile, const vpx_codec_enc_cfg *cfg, unsigned int frame_cnt);
void __cdecl mem_put_le16(char *mem, __int16 val);
void __cdecl mem_put_le32(char *mem, unsigned int val);
void __cdecl vpx_encode_frame(unsigned __int8 *y, unsigned __int8 *u, unsigned __int8 *v, bool eof);
void __cdecl write_ivf_frame_header(FILE *outfile, const vpx_codec_cx_pkt *pkt);
void __cdecl vpx_shutdown();

#if defined(__ANDROID__)
bool vpx_init_native(const char *filename,int width,int height,unsigned fps);
bool vpx_is_ready();
bool vpx_ready_for(int width,int height,unsigned fps);
bool vpx_encode_native(const unsigned char *y,const unsigned char *u,const unsigned char *v,bool eof=false);
bool vpx_shutdown_native();
#endif
