from pathlib import Path
import hashlib,json
root=Path.cwd();out=Path(__file__).parent
p=root/'src/platform/android/performance_gpu_overlay.cpp';before=p.read_bytes()
(out/'performance_gpu_overlay.cpp.before').write_bytes(before)
assert hashlib.sha256(before).hexdigest().upper()=='EFC2485C3FD7813A1BAABE0512009D51348C5FF2A40E283A1B152EE1D2BA0C47'
old=b'constexpr unsigned MarkCount = 7;\n'
new=old+b'#if defined(BO1_NATIVE_GPU_POST_PROFILE) && BO1_NATIVE_GPU_POST_PROFILE\nconstexpr int MarkBase = 100;\nconstexpr const char *ProfileName = "post";\n#else\nconstexpr int MarkBase = 0;\nconstexpr const char *ProfileName = "coarse";\n#endif\n'
assert before.count(old)==1;after=before.replace(old,new,1)
old=b'"BO1_NATIVE_GPU_PHASE seq=%llu';new=b'"BO1_NATIVE_GPU_PHASE profile=%s seq=%llu'
assert after.count(old)==1;after=after.replace(old,new,1)
old=b'        static_cast<unsigned long long>(set.sequence),';new=b'        ProfileName, static_cast<unsigned long long>(set.sequence),'
assert after.count(old)==1;after=after.replace(old,new,1)
old=b'    if (index < 0 || index >= static_cast<int>(MarkCount)) return;\n'
new=b'    if (index < MarkBase || index >= MarkBase + static_cast<int>(MarkCount)) return;\n    index -= MarkBase;\n'
assert after.count(old)==1;after=after.replace(old,new,1)
p.write_bytes(after)
assert after.count(b'\r')==before.count(b'\r')==0
(out/'source-manifest.json').write_text(json.dumps({'before':hashlib.sha256(before).hexdigest(),'after':hashlib.sha256(after).hexdigest(),'bytes':len(after)},indent=2),encoding='utf8')
print((out/'source-manifest.json').read_text())
