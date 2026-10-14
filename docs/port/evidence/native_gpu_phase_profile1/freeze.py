from pathlib import Path
import hashlib,json,re,subprocess
root=Path.cwd();out=root/'docs/port/evidence/native_gpu_phase_profile1'; rows=json.loads((root/'build-android-audit/native_engine_entry1/compile_commands.json').read_text());result={}
for flag in [0,1]:
 row=next(r for r in rows if Path(r['file']).name=='performance_gpu_overlay.cpp');cmd=row['command'];cmd=re.sub(r' -DBO1_NATIVE_GPU_PASS_PROFILE(?:=[^ ]+)?','',cmd);cmd=re.sub(r' -o .*? -c ',f' -DBO1_NATIVE_GPU_PASS_PROFILE={flag} -Wall -Wextra -Werror -o "{(out/f"actual-provider-{flag}.o").as_posix()}" -c ',cmd,count=1)
 (out/f'actual-provider-{flag}-command.txt').write_text(cmd,encoding='utf8')
 with (out/f'actual-provider-{flag}.log').open('wb') as f:result[f'provider{flag}']=subprocess.run(cmd,cwd=row['directory'],stdout=f,stderr=subprocess.STDOUT).returncode
p=root/'src/gfx_d3d/rb_backend.cpp';before=(out/'rb_backend.cpp.before').read_bytes();after=p.read_bytes();hook=b'#if defined(__ANDROID__)\r\n    bo1::android::performance::gpu::Mark(idx);\r\n#endif\r\n';assert after.count(hook)==1;result['backend_exact_after_hook_removal']=after.replace(hook,b'',1)==before
manifest={}
for name in ['src/platform/android/performance_gpu_overlay.cpp','src/platform/android/performance_gpu_overlay.h','src/gfx_d3d/rb_backend.cpp']:
 p=root/name;manifest[name]={'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'bytes':len(p.read_bytes())}
(out/'frozen-source-manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf8');(out/'actual-compile-results.json').write_text(json.dumps(result,indent=2),encoding='utf8');print(json.dumps(result));print(json.dumps(manifest))
