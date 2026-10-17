"""Exercise extracted production SMC allocator with native 64-bit list/leaf layout."""
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / "build-android-audit" / "smc-native-allocator"
OUT.mkdir(exist_ok=True)
def read(path):
    return (ROOT / path).read_bytes().decode("cp1252").replace("\r\n", "\n")
cpp=read("src/gfx_d3d/r_staticmodelcache.cpp")
hdr=read("src/gfx_d3d/rb_backend.h")
cachehdr=read("src/gfx_d3d/r_staticmodelcache.h")
def section(text,start,end):
    a=text.index(start);return text[a:text.index(end,a)]
structs=section(hdr,"struct GfxCachedSModelSurf", "struct ScopedShaderConstantSetUndo")
structs+=section(cachehdr,"struct static_model_cache_t", "void __cdecl R_SkinCachedStaticModelCmd")
functions=section(cpp,"static_model_leaf_t *__cdecl SMC_GetLeaf(","void __cdecl SetupTransformUnitVec")
functions+=section(cpp,"unsigned __int16 __cdecl SMC_Allocate(","void __cdecl R_CacheStaticModelIndices")
functions+=section(cpp,"unsigned int SMC_ClearCache()", "void __cdecl R_FlushStaticModelCache()")
# The unused diagnostic return narrows a pointer in legacy ClearCache. Preserve its
# low 32 bits through a pointer-sized intermediate so host clang accepts it.
functions=functions.replace("(unsigned int)v1", "(unsigned int)(unsigned long long)v1")
assert functions.count("/ sizeof(*leafs)")==2
prefix=r'''extern "C" { int _fltused=0; }
static int assertionLine;
bool Assert_MyHandler(const char*, int line, int, const char*, ...) { assertionLine=line;return false; }
#define __debugbreak() __builtin_trap()
'''
preamble=r'''
static_assert(sizeof(void*)==8);
static_assert(sizeof(static_model_leaf_t)==16);
static_model_cache_t s_cache;
struct Inst { unsigned short smodelCacheIndex[4]; } instances[16384];
struct World { struct { Inst* smodelDrawInsts; } dpvs; } world;
struct { World* world; } rgp;
struct { unsigned frontEndFrameCount; } rg;
char SMC_GetFreeBlockOfSize(unsigned,unsigned);
char SMC_ForceFreeBlock(unsigned);
void SMC_FreeCachedSurface_r(static_model_tree_t*,static_model_leaf_t*,int,int);
'''
checks=r'''
static void Reset() {
 volatile unsigned char* p=(volatile unsigned char*)&s_cache;
 for(unsigned i=0;i<sizeof(s_cache);++i)p[i]=0;
 for(unsigned i=0;i<16384;++i)for(unsigned j=0;j<4;++j)instances[i].smodelCacheIndex[j]=0;
 world.dpvs.smodelDrawInsts=instances;rgp.world=&world;rg.frontEndFrameCount=100;
 SMC_ClearCache();
}
static unsigned checks;
#define CHECK(x) do { ++checks; if(!(x))return __LINE__; } while(0)
static void Publish(unsigned cache,unsigned model,unsigned lod) {
 auto* leaf=SMC_GetLeaf(cache);
 leaf->cachedSurf.smodelIndex=(unsigned short)model;leaf->cachedSurf.lodIndex=(unsigned short)lod;
 instances[model].smodelCacheIndex[lod]=(unsigned short)cache;
 s_cache.trees[(cache-1)/32].frameCount=rg.frontEndFrameCount;
}
extern "C" __declspec(dllexport) unsigned Checks() { return checks; }
extern "C" __declspec(dllexport) int AssertionLine() { return assertionLine; }
extern "C" __declspec(dllexport) int RunTests() {
 Reset(); checks=0;
 // All four arenas, all six allocation sizes. Fully fill one tree with each
 // size; verify unique aligned ranges, base indices and tree accounting.
 for(unsigned arena=0;arena<4;++arena)for(unsigned bits=4;bits<=9;++bits) {
  Reset();unsigned ids[32];unsigned count=512u>>bits;
  for(unsigned i=0;i<count;++i) {
   unsigned id=SMC_Allocate(arena,bits);CHECK(id>0 && id<=16384);ids[i]=id;
   auto* leaf=SMC_GetLeaf(id);unsigned tree=(id-1)/32;unsigned offset=(id-1)%32;
   CHECK(tree/128==arena);CHECK(offset%(1u<<(bits-4))==0);
   CHECK(leaf->cachedSurf.baseVertIndex==tree*512+offset*16);
   for(unsigned j=0;j<i;++j)CHECK(ids[j]!=id);
   Publish(id,i,0);CHECK(s_cache.trees[tree].nodes[0].usedVerts==(int)((i+1)*(1u<<bits)));
  }
  CHECK(s_cache.trees[(ids[0]-1)/32].nodes[0].usedVerts==512);
 }
 // Mixed splits and partially-free recursive eviction unlink every free buddy
 // while invalidating precisely the published model references.
 for(unsigned arena=0;arena<4;++arena) {
  Reset();unsigned ids[6];
  for(unsigned i=0;i<6;++i){ids[i]=SMC_Allocate(arena,4+i%4);CHECK(ids[i]);Publish(ids[i],i,i%4);}
  rg.frontEndFrameCount+=3;CHECK(!SMC_ForceFreeBlock(arena));
  for(unsigned i=0;i<6;++i)CHECK(instances[i].smodelCacheIndex[i%4]==ids[i]);
  ++rg.frontEndFrameCount;CHECK(SMC_ForceFreeBlock(arena));
  for(unsigned i=0;i<6;++i)CHECK(instances[i].smodelCacheIndex[i%4]==0);
  CHECK(s_cache.usedlist[arena].next==&s_cache.usedlist[arena]);
  unsigned full=SMC_Allocate(arena,9);CHECK(full);Publish(full,7,0);
 }
 // Exhaust all 128 trees of an arena, observe four-frame lifetime protection,
 // then allocate through forced LRU eviction and confirm reference retirement.
 for(unsigned arena=0;arena<4;++arena) {
  Reset();unsigned oldest=0;
  for(unsigned i=0;i<128;++i){unsigned id=SMC_Allocate(arena,9);CHECK(id);Publish(id,i,0);if(!i)oldest=id;}
  CHECK(!SMC_Allocate(arena,9));CHECK(instances[0].smodelCacheIndex[0]==oldest);
  rg.frontEndFrameCount+=4;unsigned recycled=SMC_Allocate(arena,9);CHECK(recycled==oldest);
  CHECK(instances[0].smodelCacheIndex[0]==0);Publish(recycled,128,0);
  for(unsigned i=1;i<128;++i)CHECK(instances[i].smodelCacheIndex[0]!=0);
 }
 return 0;
}
'''
base=prefix+structs+preamble+functions+checks
compiler=r"C:\Program Files\LLVM\bin\clang++.exe"
first=base.index("/ sizeof(*leafs)")
second=base.index("/ sizeof(*leafs)",first+1)
mutants=[("fixed",base),("old_allocate",base[:first]+base[first:].replace("/ sizeof(*leafs)","/ 8",1)),("old_split",base[:second]+base[second:].replace("/ sizeof(*leafs)","/ 8",1))]
for name,body in mutants:
    src=OUT/(name+".cpp");dll=OUT/(name+".dll");src.write_text(body,encoding="ascii")
    subprocess.run([compiler,"-std=c++17","-O0","-fno-strict-aliasing","-fno-stack-protector","-shared","-nostdlib","-fuse-ld=lld",str(src),"-o",str(dll),"-Wl,/noentry"],check=True)
    probe="import ctypes\nd=ctypes.CDLL("+repr(str(dll))+")\ntry: print('result',d.RunTests(),'checks',d.Checks(),flush=True)\nexcept OSError:\n print('assertion_line',d.AssertionLine(),flush=True)\n raise SystemExit(1)"
    result=subprocess.run([sys.executable,"-c",probe],capture_output=True,text=True)
    print(name,"exit",result.returncode,result.stdout.strip(),result.stderr.strip())
    if name=="fixed":assert result.returncode==0 and "result 0 " in result.stdout
    else:
        expected=613 if name=="old_allocate" else 521
        assert result.returncode==1 and ("assertion_line "+str(expected)) in result.stdout, "old stride did not fail expected index bound"
print("PASS production allocator native64 split, full/mixed allocation, retirement and eviction; old stride fails")
