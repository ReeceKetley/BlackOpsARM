#include <gfx_d3d/r_font_load_obj.h>
#include <algorithm>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
// Deterministic file/hunk/material fixtures for the production parser only.
static std::vector<unsigned char> bytes;
static unsigned position,advertised,closes,materials;
struct Allocation { unsigned char *memory;unsigned size; };
static std::vector<Allocation> allocations;
static void check(bool ok,const char *name){if(!ok){std::fprintf(stderr,"FAIL %s\n",name);std::exit(1);}std::printf("PASS %s\n",name);}
unsigned FS_FOpenFileReadCurrentThread(const char *,int *file){position=0;*file=1;return advertised;}
unsigned FS_Read(unsigned char *out,unsigned count,int){unsigned size=std::min(count,static_cast<unsigned>(bytes.size())-position);memcpy(out,bytes.data()+position,size);position+=size;return size;}
void FS_FCloseFile(int){++closes;}
unsigned char *Hunk_Alloc(unsigned size,const char *,int){auto *memory=static_cast<unsigned char *>(malloc(size+32));check(memory!=nullptr,"fixture allocation");memset(memory,0,size);memset(memory+size,0xa5,32);allocations.push_back({memory,size});return memory;}
Material *Material_RegisterHandle(char *,int){++materials;return reinterpret_cast<Material *>(static_cast<std::uintptr_t>(materials*16));}
void Com_PrintError(int,const char *,...){}
int Com_sprintf(char *out,unsigned capacity,const char *format,...){va_list args;va_start(args,format);int result=vsnprintf(out,capacity,format,args);va_end(args);return result;}
static void word(unsigned offset,int value){memcpy(bytes.data()+offset,&value,4);}
static void fixture(){bytes.assign(16+sizeof(Glyph)+16,0);word(0,16+sizeof(Glyph));word(4,24);word(8,1);word(12,16+sizeof(Glyph)+5);Glyph glyph{};glyph.letter='A';glyph.dx=7;memcpy(bytes.data()+16,&glyph,sizeof(glyph));memcpy(bytes.data()+16+sizeof(Glyph),"font\0material\0",14);advertised=bytes.size();}
int main(){
    fixture();auto *font=R_LoadFont("fixture",7);
    check(font && allocations.back().size==sizeof(Font_s)+bytes.size()-16,"allocation uses native header plus unchanged disk payload");
    check(font->glyphCount==1 && font->pixelHeight==24 && font->glyphs[0].letter=='A' && !strcmp(font->fontName,"font"),"actual parser publishes native pointers and glyph data");
    check(materials==2 && closes==1,"valid parser closes file and registers both materials");
    fixture();bytes.resize(bytes.size()-1);check(!R_LoadFont("truncated",7),"short actual file read rejected");
    fixture();word(8,100);check(!R_LoadFont("bad-count",7),"glyph count outside payload rejected");
    fixture();word(0,15);check(!R_LoadFont("bad-offset",7),"string before payload rejected");
    fixture();memset(bytes.data()+16+sizeof(Glyph),1,16);check(!R_LoadFont("bad-string",7),"unterminated strings rejected");
    check(materials==2 && closes==5,"invalid metadata never registers materials and closes every file");
    bool boundaries=true;
    for(auto allocation:allocations){for(unsigned i=0;i<32;++i)boundaries&=allocation.memory[allocation.size+i]==0xa5;free(allocation.memory);}
    check(boundaries,"all allocation boundaries preserved");
}
