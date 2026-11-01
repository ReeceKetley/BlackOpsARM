"""Exercise production checkpoint boolean codec against legacy FF sentinels."""
from pathlib import Path
import argparse, hashlib, json
root = Path(__file__).resolve().parents[3]
p = argparse.ArgumentParser(); p.add_argument('--out', type=Path, required=True); a = p.parse_args()
source = root/'src/game_sp/g_sp_native_owners_checkpoint.inl'
text = source.read_text(encoding='utf-8')
start = text.index('struct SPNativeCheckpointCodec')
end = text.index('{', start)+1; depth = 1
while depth:
    depth += (text[end] == '{')-(text[end] == '}'); end += 1
codec = text[start:end]+';'
start = text.index('unsigned char unusedTimescale = 0;')
end = text.index('SND_FIELD(positionUpdated)', start)
sentinel = text[start:end]
fixture = r'''
#include <universal/memfile.h>
#include <qcommon/common.h>
#include <sound/snd.h>
#include <type_traits>
#include <stdexcept>
#include <cstdio>
#include <cstdlib>
static int checks;
void Com_Error(errorParm_t,const char*,...){throw std::runtime_error("rejected");}
bool Assert_MyHandler(const char*,int,int,const char*,...){std::abort();}
void MemFile_ReadData(MemoryFile*f,int count,unsigned char*out){
 if(count<0 || count>f->bufferSize-f->bytesUsed)Com_Error(ERR_DROP,"truncated");
 memcpy(out,f->buffer+f->bytesUsed,count);f->bytesUsed+=count;
}
void MemFile_WriteDataInternal(MemoryFile*,unsigned,unsigned char*){Com_Error(ERR_DROP,"unexpected flush");}
static void check(bool ok){if(!ok)std::abort();++checks;}
ACTUAL_CODEC
static void preserveUnused(SPNativeCheckpointCodec &c,snd_voice_t &voice){ ACTUAL_SENTINEL }
int main(){
 static_assert(sizeof(bool)==1);
 static MemoryFile reader,writer;
 for(unsigned n=0;n<256;++n){
  unsigned char byte=n; bool value=false;memcpy(&value,&byte,1);
  writer={}; SPNativeCheckpointCodec capture{nullptr,&writer};bool rejected=false;
  try{capture.Field(value);}catch(const std::runtime_error&){rejected=true;}
  check(rejected==(n>1));check(writer.cacheBufferUsed==(n>1?0:1));
  unsigned char after;memcpy(&after,&value,1);check(after==n);
  reader={};writer={};reader.buffer=&byte;reader.bufferSize=1;SPNativeCheckpointCodec decode{&reader,&writer};rejected=false;
  try{decode.Field(value);}catch(const std::runtime_error&){rejected=true;}
  check(rejected==(n>1));check(writer.cacheBufferUsed==(n>1?0:1));
  if(n<2)check(value==(n!=0));
  snd_voice_t voice{};memcpy(&voice.timescale,&byte,1);writer={};rejected=false;
  try{preserveUnused(capture,voice);}catch(const std::runtime_error&){rejected=true;}
  const bool valid=n==0 || n==1 || n==255;
  check(rejected!=valid);if(valid)check(writer.cacheBufferUsed==1 && writer.cacheBuffer[0]==n);
  reader={};reader.buffer=&byte;reader.bufferSize=1;writer={};rejected=false;
  try{preserveUnused(decode,voice);}catch(const std::runtime_error&){rejected=true;}
  check(rejected!=valid);if(valid)check(writer.cacheBufferUsed==1 && writer.cacheBuffer[0]==n);
 }
 std::printf("actual native boolean/sentinel codec %i checks PASS\n",checks);
}
'''
a.out.mkdir(parents=True, exist_ok=False)
(a.out/'fixture.cpp').write_text(fixture.replace('ACTUAL_CODEC',codec).replace('ACTUAL_SENTINEL',sentinel), encoding='utf-8')
(a.out/'source-manifest.json').write_text(json.dumps({'file':str(source),'sha256':hashlib.sha256(source.read_bytes()).hexdigest()},indent=2),encoding='utf-8')
