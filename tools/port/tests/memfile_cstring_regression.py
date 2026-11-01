"""Actual CString reader and canonical types; bounded byte IO provider isolated."""
from pathlib import Path
import argparse,hashlib,json
root=Path(__file__).resolve().parents[3]
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True)
p.add_argument('--source',type=Path,default=root/'src/universal/memfile.cpp');a=p.parse_args()
s=a.source.read_bytes().decode('cp1252');start=s.index('char *__cdecl MemFile_ReadCString(')
b=s.index('{',start);end=b+1;depth=1
while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
fixture=r'''
#include <universal/memfile.h>
#include <universal/physicalmemory.h>
#include <qcommon/common.h>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdio>
#include <cstdlib>
// Deliberately reverse the unrelated global/buffer order seen by the old guard.
// The actual engine types and reader body are unchanged by this test layout.
struct Layout { PhysicalMemory unrelated; char text[1024]; unsigned char canary[16]; };
static Layout layout;
static unsigned checks;
static void check(bool ok){if(!ok)std::abort();++checks;}
void Com_Error(errorParm_t,const char*,...){throw std::runtime_error("reader rejected");}
bool Assert_MyHandler(const char*,int,int,const char*,...){std::abort();}
void MemFile_ReadData(MemoryFile*f,int count,unsigned char*out){
 if(count<0 || f->bytesUsed+count>f->bufferSize){f->memoryOverflow=true;if(f->errorOnOverflow)Com_Error(ERR_DROP,"truncated");return;}
 std::memcpy(out,f->buffer+f->bytesUsed,count);f->bytesUsed+=count;
}
static char (&buffer)[1024] = layout.text;
#define g_mem layout.unrelated
ACTUAL_READER
#undef g_mem
int main(){
 static MemoryFile file;
 for(int length=0;length<1024;++length){
  std::vector<unsigned char> bytes(length+1);
  for(int i=0;i<length;++i)bytes[i]=(unsigned char)(1+(i%255));bytes[length]=0;
  std::memset(layout.text,0x55,sizeof(layout.text));std::memset(layout.canary,0xA5,sizeof(layout.canary));
  file={};file.buffer=bytes.data();file.bufferSize=(int)bytes.size();file.errorOnOverflow=true;
  char *result=nullptr;try{result=MemFile_ReadCString(&file);}catch(const std::runtime_error&){std::printf("valid length %i wrongly rejected\n",length);return 1;}
  check(result==layout.text && std::memcmp(result,bytes.data(),bytes.size())==0);
  check(file.bytesUsed==length+1 && !file.memoryOverflow);
  for(auto c:layout.canary)check(c==0xA5);
 }
 for(int length:{1024,1025,2048}){
  std::vector<unsigned char> bytes(length+1,'x');bytes[length]=0;
  file={};file.buffer=bytes.data();file.bufferSize=(int)bytes.size();file.errorOnOverflow=true;
  bool rejected=false;try{MemFile_ReadCString(&file);}catch(const std::runtime_error&){rejected=true;}
  check(rejected && file.bytesUsed==1024);for(auto c:layout.canary)check(c==0xA5);
 }
 unsigned char both[]={'a','b','c',0,'x','y',0};file={};file.buffer=both;file.bufferSize=sizeof(both);
 check(!std::strcmp(MemFile_ReadCString(&file),"abc"));check(!std::strcmp(MemFile_ReadCString(&file),"xy"));check(file.bytesUsed==sizeof(both));
 for(int length:{0,1,1023}){
  std::vector<unsigned char> bytes(length?length:1,'x');file={};file.buffer=bytes.data();file.bufferSize=length;
  check(!*MemFile_ReadCString(&file) && file.memoryOverflow);
  for(auto c:layout.canary)check(c==0xA5);
 }
 std::printf("memory CString %u checks PASS\n",checks);
}
'''
a.out.mkdir(parents=True,exist_ok=True)
(a.out/'fixture.cpp').write_text(fixture.replace('ACTUAL_READER',s[start:end]),encoding='utf-8')
(a.out/'source-manifest.json').write_text(json.dumps({'source':str(a.source),'sha256':hashlib.sha256(a.source.read_bytes()).hexdigest()},indent=2)+'\n',encoding='utf-8')
