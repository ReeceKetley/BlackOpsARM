from pathlib import Path
import subprocess,json,hashlib
root=Path.cwd();out=root/'docs/port/evidence/native_post_filter_depth1';results={}
for flag in [0,1]:
 label=f'fixture-{flag}';exe=out/(label+'.exe')
 command=f'"C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat" x64 >nul && cl /nologo /W4 /WX /std:c++20 /EHsc /MTd /Od /DBO1_NATIVE_FILTER_DEPTH_UNBIND={flag} /I"{out}" "{out / "fixture.cpp"}" /Fo"{out / (label+".obj")}" /Fe"{exe}"'
 (out/(label+'-command.txt')).write_text(command,encoding='utf8')
 with (out/(label+'-compile.log')).open('wb') as f:code=subprocess.run('cmd.exe /d /s /c "'+command+'"',stdout=f,stderr=subprocess.STDOUT).returncode
 results[label]={'compile':code}
 if code==0:
  for case in ['normal']+(['state','get','get-positive','unbind','restore'] if flag else []):
   run=subprocess.run([str(exe)]+([] if case=='normal' else [case]),capture_output=True)
   log=run.stdout+run.stderr;(out/(label+'-'+case+'.log')).write_bytes(log)
   results[label][case]={'exit':run.returncode,'expected':run.returncode==0 if case=='normal' else run.returncode!=0,'release':b'TRACE Release' in log}
 ndk='D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64'
 command=[ndk+'/bin/clang++.exe','--target=aarch64-none-linux-android28','--sysroot='+ndk+'/sysroot','-I'+str(root/'../DXVK-Native/include/native/directx'),'-I'+str(root/'../DXVK-Native/include/native/windows'),'-std=c++20','-g','-O0','-fPIC','-Wall','-Wextra','-Werror','-fsanitize=undefined','-fno-sanitize-recover=all','-DBO1_NATIVE_FILTER_DEPTH_UNBIND='+str(flag),'-c',str(root/'src/platform/android/post_filter_depth.cpp'),'-o',str(out/f'native-{flag}.o')]
 (out/f'native-{flag}-command.json').write_text(json.dumps(command,indent=2),encoding='utf8')
 with (out/f'native-{flag}.log').open('wb') as f:results[f'native-{flag}']=subprocess.run(command,stdout=f,stderr=subprocess.STDOUT).returncode
(out/'results.json').write_text(json.dumps(results,indent=2),encoding='utf8');print(json.dumps(results,indent=2))
files=[root/'src/platform/android/post_filter_depth.h',root/'src/platform/android/post_filter_depth.cpp',out/'fixture.cpp',out/'d3d9.h',out/'check.py']
(out/'frozen-source-manifest.json').write_text(json.dumps({str(p.relative_to(root)):{'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'size':p.stat().st_size} for p in files},indent=2),encoding='utf8')
