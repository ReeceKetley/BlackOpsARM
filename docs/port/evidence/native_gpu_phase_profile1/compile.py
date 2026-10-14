from pathlib import Path
import subprocess,re,json,hashlib
root=Path.cwd();out=root/'docs/port/evidence/native_gpu_phase_profile1'
base=(root/'build-android-audit/native_performance_gpu_overlay1/performance_gpu_overlay-native-command.txt').read_text().strip()
results={}
for enabled in [0,1]:
 label=f'gpu-provider-native-{enabled}'
 cmd=re.sub(r' -o .*? -c ',f' -DBO1_NATIVE_GPU_PASS_PROFILE={enabled} -o "{(out/(label+".o")).as_posix()}" -c ',base,count=1)
 (out/(label+'-command.txt')).write_text(cmd+'\n',encoding='utf8')
 with (out/(label+'.log')).open('wb') as f: results[label]=subprocess.run(cmd,stdout=f,stderr=subprocess.STDOUT).returncode
(out/'compile-results.json').write_text(json.dumps(results,indent=2),encoding='utf8');print(results)
