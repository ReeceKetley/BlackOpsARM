#include <platform/android/wma_decoder.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <array>
static unsigned checks;
static void Check(bool value,int line){if(!value){fprintf(stderr,"ADPCM FAIL line %i\n",line);std::abort();}++checks;}
#define CHECK(x) Check((x),__LINE__)
static void Put(std::uint8_t *p,int value){p[0]=value&255;p[1]=(value>>8)&255;}
int main(int argc,char **argv){
 CHECK(argc==2);FILE *file=fopen(argv[1],"wb");CHECK(file);
 for(unsigned channels:{1u,2u})for(unsigned predictor=0;predictor<7;++predictor){
  std::vector<std::uint8_t> encoded(262*channels*3);
  for(unsigned block=0;block<3;++block){auto *p=encoded.data()+block*262*channels;
   for(unsigned channel=0;channel<channels;++channel){p[channel]=(predictor+channel)%7;Put(p+channels+2*channel,32+block*16);Put(p+3*channels+2*channel,1000+block*300-channel*1700);Put(p+5*channels+2*channel,900+block*300-channel*1500);}
   constexpr unsigned nibble[]={0,1,15,2,14,3,13,0};
   for(unsigned byte=7*channels;byte<262*channels;++byte)p[byte]=(nibble[(byte+block)%8]<<4)|nibble[(byte+block+3)%8];
  }
  bo1::android::MsAdpcmAssetView asset{channels,48000,1536,encoded};
  bo1::android::DecodedMsAdpcm decoded;std::string error;
  if(!bo1::android::DecodeCanonicalMsAdpcm(asset,decoded,error)){fprintf(stderr,"%s\n",error.c_str());CHECK(false);}
  CHECK(decoded.samples.size()==1536*channels);
  CHECK(fwrite(encoded.data(),1,encoded.size(),file)==encoded.size());
  CHECK(fwrite(decoded.samples.data(),sizeof(short),decoded.samples.size(),file)==decoded.samples.size());
  // Every byte truncation, predictor, signed-delta and endpoint failure leaves no output.
  const auto full=asset.blocks;
  for(std::size_t size=0;size<encoded.size();++size){asset.blocks=full.first(size);CHECK(!bo1::android::DecodeCanonicalMsAdpcm(asset,decoded,error)&&decoded.samples.empty());}
  asset.blocks=full;
  for(unsigned bad:{7u,255u}){encoded[0]=bad;CHECK(!bo1::android::DecodeCanonicalMsAdpcm(asset,decoded,error)&&decoded.samples.empty());}encoded[0]=predictor;
  for(unsigned bad:{0u,15u,32768u,65535u}){Put(encoded.data()+channels,bad);CHECK(!bo1::android::DecodeCanonicalMsAdpcm(asset,decoded,error)&&decoded.samples.empty());}Put(encoded.data()+channels,32);
  // Valid signed delta which overflows on a following nibble in FAudio.
  Put(encoded.data()+channels,20000);encoded[7*channels]=0x88;
  CHECK(!bo1::android::DecodeCanonicalMsAdpcm(asset,decoded,error)&&decoded.samples.empty());
  asset.frameCount=1535;CHECK(!bo1::android::DecodeCanonicalMsAdpcm(asset,decoded,error)&&decoded.samples.empty());
 }
 CHECK(!fclose(file));printf("ADPCM checkpoint decoder PASS %u checks; 14 mono/stereo/predictor vectors exported for independent FAudio waveform comparison\n",checks);
}
