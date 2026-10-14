#include <platform/android/wma_decoder.h>
#include <platform/fastfile_inflate.h>
extern "C" {
#include <libavutil/sha.h>
#include <libavutil/mem.h>
}
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <array>
#include <string>
#include <vector>
static void Check(bool good,int line) { if(!good) {fprintf(stderr,"FAIL canonical WMA fixture line%d\n",line);std::abort();} }
#define CHECK(x) Check((x),__LINE__)
static std::uint32_t Read(const std::uint8_t *p) { std::uint32_t v;memcpy(&v,p,4);return v; }
static void Decode(const std::vector<std::uint8_t> &zone,std::size_t offset,const char *digest) {
    CHECK(offset+60<=zone.size());const auto *h=zone.data()+offset;
    CHECK(Read(h)==0xffffffff && Read(h+4)==1 && Read(h+32)==7 && Read(h+56)==0xffffffff);
    const unsigned frames=Read(h+8),rate=Read(h+12),channels=Read(h+16),packets=Read(h+44),size=Read(h+52);
    const auto *name=h+60;const auto *end=static_cast<const std::uint8_t *>(memchr(name,0,zone.size()-offset-60));CHECK(end);
    CHECK(static_cast<std::uint64_t>(end+1-zone.data())+packets*4ull+size<=zone.size());
    std::vector<std::uint32_t> seek(packets);memcpy(seek.data(),end+1,packets*4ull);const auto *data=end+1+packets*4ull;
    auto *sha=av_sha_alloc();CHECK(sha && !av_sha_init(sha,256));av_sha_update(sha,data,size);
    std::array<std::uint8_t,32> hash{};av_sha_final(sha,hash.data());av_free(sha);
    char text[65]{};for(unsigned i=0;i<32;++i)sprintf(text+i*2,"%02x",hash[i]);CHECK(!strcmp(text,digest));
    bo1::android::WmaAssetView asset{channels,rate,channels==1?2230u:4096u,frames,seek,{data,size}};
    bo1::android::DecodedWma output;std::string error;
    if(!bo1::android::DecodeCanonicalWma(asset,output,error)) { fprintf(stderr,"Actual decode failure: %s\n",error.c_str());CHECK(false); }
    CHECK(output.channels==channels && output.sampleRate==rate && output.samples.size()==static_cast<std::size_t>(frames)*channels);
    double energy=0;for(const float sample:output.samples) {CHECK(std::isfinite(sample));energy+=sample*sample;}
    CHECK(energy>1e-5);
    // Whole-asset output survives the borrowed compressed view and gives SDK one owned frame buffer.
    std::printf("PASS actual canonical WMAv2 channels%u rate%u frames%u packets%u rms%.9f asset%s\n",channels,rate,frames,packets,sqrt(energy/output.samples.size()),text);fflush(stdout);
    for(std::size_t first=0;first<output.samples.size();first+=512*channels) {
        const auto count=std::min<std::size_t>(512*channels,output.samples.size()-first);double sum=0,squares=0;
        for(std::size_t i=0;i<count;++i) {const auto value=output.samples[first+i];sum+=value;squares+=value*value;}
        printf("WMA_ORACLE ch%u first%zu count%zu mean%.12f rms%.12f\n",channels,first,count,sum/count,sqrt(squares/count));
    } fflush(stdout);
    const auto last=seek.back();seek.back()=last+2;CHECK(!bo1::android::DecodeCanonicalWma(asset,output,error) && output.samples.empty());seek.back()=last;
    seek[0]=last+channels*2;CHECK(!bo1::android::DecodeCanonicalWma(asset,output,error) && output.samples.empty());memcpy(seek.data(),end+1,packets*4ull);
    asset.packets=asset.packets.first(size-1);CHECK(!bo1::android::DecodeCanonicalWma(asset,output,error) && output.samples.empty());
    std::vector<std::uint8_t> corrupt(size,0);asset.packets=corrupt;
    CHECK(!bo1::android::DecodeCanonicalWma(asset,output,error) && output.samples.empty());
}
int main(int argc,char **argv) {
    CHECK(argc==2);Bo1InflatedFastFile zone;Bo1InflateLimits limits;
    const auto result=Bo1_InflateFastFilePath(argv[1],limits,&zone);
    if(result!=Bo1InflateResult::Ok) {fprintf(stderr,"actual original-zone inflate: %s\n",Bo1_InflateResultName(result));CHECK(false);}
    Decode(zone.decoded,5292715,"0450a70e738a6dd402b94462adebfdb8179415b786c2d270a3b4a1e057ffb6b6");
    Decode(zone.decoded,5326506,"cb4c00ad6449555adc0fa200dca9151b452a4fc7184d4f8d6d244d0122c57180");
}
