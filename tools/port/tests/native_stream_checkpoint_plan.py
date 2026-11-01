"""Actual stream seek planner with canonical asset types and independent cursor oracle."""
from pathlib import Path
import argparse,hashlib,json
root=Path(__file__).resolve().parents[3]
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);a=p.parse_args()
source=root/'src/sound/snd_stream_checkpoint.inl';s=source.read_text()
begin=s.index('bool Snd_StreamCheckpointPlan(');end=s.index('{',begin)+1;depth=1
while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
fixture=r'''
#include <sound/snd.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <initializer_list>
static unsigned checks;
static void check(bool b){if(!b)std::abort();++checks;}
ACTUAL
int main(){
 for(unsigned channels:{1u,2u})for(bool adpcm:{false,true}){
  snd_asset h{};h.version=1;h.header_size=2096;h.channel_count=channels;h.frame_rate=48000;h.frame_count=4096;
  h.format=adpcm?SND_ASSET_FORMAT_MSADPCM:SND_ASSET_FORMAT_PCMS16;h.data_size=adpcm?channels*262*8:channels*2*4096;
  for(bool loop:{false,true}){
   h.flags=static_cast<snd_asset_flags>(loop?1:0);
   for(unsigned long long cursor=0;cursor<8193;++cursor){unsigned offset=0xFEED,skip=0xF00D;
    const bool valid=loop||cursor<4096;
    check(Snd_StreamCheckpointPlan(h,loop,cursor,&offset,&skip)==valid);
    if(valid){auto phase=cursor%4096;check(offset==(adpcm?(phase/512)*262*channels:phase*2*channels));check(skip==(adpcm?phase%512:0));check(offset<h.data_size);}
    else check(offset==0xFEED&&skip==0xF00D);
   }
  }
  h.flags=static_cast<snd_asset_flags>(1);
  for(unsigned long long cursor:{0ULL,1ULL,511ULL,512ULL,513ULL,0x7FFFFFFFFFFFFFFFULL,0xFFFFFFFFFFFFFFFFULL}){
   unsigned offset,skip;check(Snd_StreamCheckpointPlan(h,true,cursor,&offset,&skip));
   auto phase=cursor%4096;check(offset==(adpcm?(phase/512)*262*channels:phase*2*channels));check(skip==(adpcm?phase%512:0));
  }
  unsigned offset,skip;
  check(!Snd_StreamCheckpointPlan(h,false,0,&offset,&skip));check(!Snd_StreamCheckpointPlan(h,true,0,nullptr,&skip));check(!Snd_StreamCheckpointPlan(h,true,0,&offset,nullptr));
  auto original=h;
  for(unsigned value:{0u,999u,100001u,0xFFFFFFFFu}){h.frame_rate=value;check(!Snd_StreamCheckpointPlan(h,true,0,&offset,&skip));}h=original;
  for(unsigned value:{0u,3u,0xFFFFFFFFu}){h.channel_count=value;check(!Snd_StreamCheckpointPlan(h,true,0,&offset,&skip));}h=original;
  h.frame_count=4095;check(!Snd_StreamCheckpointPlan(h,true,0,&offset,&skip));h=original;
  for(unsigned value:{0u,1u,0xFFFFFFFFu,0xFFFFF830u}){h.data_size=value;check(!Snd_StreamCheckpointPlan(h,true,0,&offset,&skip));}h=original;
  h.version=2;check(!Snd_StreamCheckpointPlan(h,true,0,&offset,&skip));h=original;
  h.header_size=56;check(!Snd_StreamCheckpointPlan(h,true,0,&offset,&skip));h=original;
  h.seek_table_count=1;check(!Snd_StreamCheckpointPlan(h,true,0,&offset,&skip));h=original;
  h.format=SND_ASSET_FORMAT_WMA;check(!Snd_StreamCheckpointPlan(h,true,0,&offset,&skip));
 }
 printf("actual stream checkpoint planner %u checks PASS, pointer %zu bytes\n",checks,sizeof(void*));
}
'''
a.out.mkdir(parents=True,exist_ok=False)
(a.out/'fixture.cpp').write_text(fixture.replace('ACTUAL',s[begin:end]),encoding='utf-8')
(a.out/'source-manifest.json').write_text(json.dumps({'file':str(source),'sha256':hashlib.sha256(source.read_bytes()).hexdigest()},indent=2),encoding='utf-8')
