#include <cstdio>
#include <cstdlib>
#include <cstring>
unsigned MSG_CompressWithZLib(unsigned char *,unsigned,unsigned char *,unsigned);
unsigned MSG_DecompressWithZLib(unsigned char *,unsigned,unsigned char *,unsigned);
static void check(bool ok,const char *name){if(!ok){std::fprintf(stderr,"FAIL %s\n",name);std::exit(1);}std::printf("PASS %s\n",name);}
int main(){
    unsigned char input[4096],compressed[8192],output[4096];
    for(unsigned i=0;i<sizeof(input);++i)input[i]=static_cast<unsigned char>((i*i+i/17)%251);
    unsigned size=MSG_CompressWithZLib(input,sizeof(input),compressed,sizeof(compressed));
    check(size>0 && size<sizeof(compressed),"original MSG wrapper generates actual raw deflate");
    unsigned result=MSG_DecompressWithZLib(compressed,size,output,sizeof(output));
    check(result==sizeof(input) && !memcmp(input,output,sizeof(input)),"original MSG wrapper exact-byte round trip");
    check(MSG_CompressWithZLib(input,sizeof(input),compressed,1)==0,"insufficient compression output returns real failure");
    size=MSG_CompressWithZLib(input,sizeof(input),compressed,sizeof(compressed));
    check(MSG_DecompressWithZLib(compressed,size,output,1)==0,"insufficient decode output returns real failure");
    check(MSG_DecompressWithZLib(compressed,size-1,output,sizeof(output))==0,"truncated stream rejected");
}
