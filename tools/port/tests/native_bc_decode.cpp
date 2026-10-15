#include "gfx_vulkan/block_compression.h"
#include <cassert>
#include <cstdio>
#include <array>
using namespace bo1::vulkan;
int main() {
 std::vector<unsigned char> out;std::string error;
 // Deliberately unaligned input; BC1 transparent index and two RGB endpoints.
 unsigned char storage[17]{};auto* b=storage+1;b[2]=255;b[3]=255;b[4]=0xe4;
 assert(DecodeBlocks(BlockCompression::BC1,4,4,1,b,8,out,error));
 assert(out[0]==0&&out[3]==255&&out[4]==255&&out[7]==255&&out[11]==255&&out[15]==0);
 // BC2 always uses four colors even with increasing endpoints; alpha nibble 10.
 unsigned char bc2[16]{};bc2[0]=0xa0;bc2[10]=255;bc2[11]=255;bc2[12]=0x0f;
 assert(DecodeBlocks(BlockCompression::BC2,4,4,1,bc2,16,out,error));
 assert(out[0]==170&&out[3]==0&&out[4]==170&&out[7]==170);
 for(bool descending:{false,true}) {
  unsigned char bc3[16]{};bc3[0]=descending?210:10;bc3[1]=descending?0:110;
  unsigned long long indices=0;for(unsigned i=0;i<16;++i)indices|=static_cast<unsigned long long>(i%8)<<(3*i);
  for(unsigned i=0;i<6;++i)bc3[2+i]=(indices>>(8*i))&255;
  bc3[8]=0;bc3[9]=0xf8;
  assert(DecodeBlocks(BlockCompression::BC3,4,4,1,bc3,16,out,error));
  const unsigned char descendingExpected[]={210,0,180,150,120,90,60,30};
  const unsigned char ascendingExpected[]={10,110,30,50,70,90,0,255};
  for(unsigned i=0;i<16;++i)assert(out[4*i]==255&&out[4*i+3]==(descending?descendingExpected:ascendingExpected)[i%8]);
 }
 // A 1x1x2 mip consumes two complete blocks, clips XY and preserves slices.
 unsigned char slices[16]{};slices[1]=0xf8;slices[10]=0x1f;slices[12]=0x55;
 assert(DecodeBlocks(BlockCompression::BC1,1,1,2,slices,16,out,error));
 assert(out.size()==8&&out[0]==255&&out[6]==255);
 assert(!DecodeBlocks(BlockCompression::BC1,1,1,2,slices,15,out,error)&&out.empty());
 assert(!DecodeBlocks(BlockCompression::BC1,0,1,1,slices,8,out,error));
 puts("PASS BC1 transparent/BC2 explicit alpha/BC3 both alpha modes/unaligned/edge-volume/truncation");
}
