#pragma once
#include <cstddef>
bool MjpegEncodePlanes(const unsigned char *y,const unsigned char *u,const unsigned char *v,unsigned width,unsigned height,unsigned char **bytes,std::size_t *size);
