from pathlib import Path
import subprocess,json
root=Path.cwd();out=Path(__file__).parent;s=(root/'src/platform/android/post_fusion_trial.cpp').read_text();scan=s[s.index('struct PixelDiff {'):s.index('void ReportDiff(')]
text='''#include <algorithm>
#include <cstdint>
#include <climits>
#include <cstdlib>
#include <cassert>
#include <cstdio>
struct D3DLOCKED_RECT { int Pitch; void* pBits; };
'''+scan+'''
int main(){
 unsigned char a[24]={},b[24]={};D3DLOCKED_RECT left{12,a},right{12,b};
 {PixelDiff r;assert(ScanRGBA(2,2,left,right,r));assert(r.channels==16&&r.histogram[0]==16&&!r.pixelsDifferent);}
 b[0]=1;b[12+4+2]=1;b[8]=255;b[20]=255;
 {PixelDiff r;assert(ScanRGBA(2,2,left,right,r));assert(r.channels==16&&r.histogram[1]==2&&r.histogram[0]==14&&r.pixelsDifferent==2&&r.maxRgb==1&&!r.alphaMismatch);}
 b[0]=2;
 {PixelDiff r;assert(ScanRGBA(2,2,left,right,r));assert(r.maxRgb==2&&!r.alphaMismatch);}
 b[0]=0;b[12+4+3]=1;
 {PixelDiff r;assert(ScanRGBA(2,2,left,right,r));assert(r.maxRgb==1&&r.alphaMismatch==1&&r.maxAlpha==1);}
 {PixelDiff r;auto invalid=right;invalid.Pitch=7;assert(!ScanRGBA(2,2,left,invalid,r));invalid.Pitch=-1;assert(!ScanRGBA(2,2,left,invalid,r));invalid.Pitch=12;invalid.pBits=nullptr;assert(!ScanRGBA(2,2,left,invalid,r));}
 std::puts("production ScanRGBA pitch/fullRGBA/histogram/acceptance-boundary PASS");
}
'''
(out/'pixel.cpp').write_text(text,encoding='utf8');exe=out/'pixel.exe';cmd=f'"C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat" x64 >nul && cl /nologo /W4 /WX /std:c++20 /EHsc /MTd "{(out/"pixel.cpp").resolve()}" /Fo"{(out/"pixel.obj").resolve()}" /Fe"{exe.resolve()}"'
(out/'pixel-command.txt').write_text(cmd,encoding='utf8')
with (out/'pixel-compile.log').open('wb') as f:code=subprocess.run('cmd.exe /d /s /c "'+cmd+'"',stdout=f,stderr=subprocess.STDOUT).returncode
result={'compile':code}
if code==0:
 with (out/'pixel-run.log').open('wb') as f:result['run']=subprocess.run([str(exe.resolve())],stdout=f,stderr=subprocess.STDOUT).returncode
(out/'pixel-results.json').write_text(json.dumps(result),encoding='utf8');print(result);assert result=={'compile':0,'run':0}
