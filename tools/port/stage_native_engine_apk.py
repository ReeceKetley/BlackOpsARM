"""Stage the real canonical engine build and its audited APK providers; no assets."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--build', required=True)
parser.add_argument('--output', required=True)
parser.add_argument('--asan', action='store_true', help='Stage explicit ASan/shared-STL providers and the NDK wrap resources')
parser.add_argument('--validate-only', action='store_true', help='Validate actual providers/dependencies without writing staging files')
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
build = Path(args.build).resolve()
output = Path(args.output).resolve()
for directory in (build, output):
    if not directory.is_relative_to(root):
        parser.error('Build and staging must stay inside BlackOpsARM')
ndk = Path('D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64')
providers = {
    'libblackopsarm_engine.so': build / 'libblackopsarm_engine.so',
    'libblackopsarm_environment.so': build / 'libblackopsarm_environment.so',
    'libSDL2.so': root.parent / 'DXVK-Native/android/app/src/main/jniLibs/arm64-v8a/libSDL2.so',
    'libdxvk_d3d9.so': root.parent / 'DXVK-Native/android/app/src/main/jniLibs/arm64-v8a/libdxvk_d3d9.so',
    'libsteam_api.so': root / 'src/steam/redistributable_bin/androidarm64/libsteam_api.so',
    'libbo1_native_shader_compiler.so': build / 'shader-compiler/libbo1_native_shader_compiler.so',
    'libclang_rt.ubsan_standalone-aarch64-android.so': ndk / 'lib/clang/21/lib/linux/libclang_rt.ubsan_standalone-aarch64-android.so',
}
if args.asan:
    cache = (build / 'CMakeCache.txt').read_text(encoding='utf-8')
    import re
    if not re.search(r'^BO1_NATIVE_ASAN:[^=]+=ON$', cache, re.M) or not re.search(r'^ANDROID_STL:[^=]+=c\+\+_shared$', cache, re.M):
        parser.error('--asan requires a separately configured BO1_NATIVE_ASAN=ON, ANDROID_STL=c++_shared build')
    if output.exists() and not args.validate_only:
        parser.error('ASan staging requires a fresh output directory; preserve prior evidence')
    providers['libclang_rt.asan-aarch64-android.so'] = ndk / 'lib/clang/21/lib/linux/libclang_rt.asan-aarch64-android.so'
    providers['libc++_shared.so'] = ndk / 'sysroot/usr/lib/aarch64-linux-android/libc++_shared.so'
    wrapper = ndk.parents[3] / 'wrap.sh/asan.sh'
    if not wrapper.is_file():
        parser.error('Actual NDK ASan wrap.sh is missing')
system = {'libandroid.so', 'libmediandk.so', 'liblog.so', 'libvulkan.so', 'libm.so',
          'libdl.so', 'libc.so', 'libz.so', 'libGLESv1_CM.so', 'libGLESv2.so', 'libEGL.so',
          'libOpenSLES.so', 'libaaudio.so', 'libatomic.so'}
readelf = ndk / 'bin/llvm-readelf.exe'
import re
manifest = []
abi = output / 'arm64-v8a'
if not args.validate_only:
    abi.mkdir(parents=True, exist_ok=True)
for name, source in providers.items():
    data = source.read_bytes()
    details = subprocess.check_output([str(readelf), '-h', '-d', '-n', '--dyn-syms', '--wide', str(source)], text=True)
    if 'AArch64' not in details or 'ELF64' not in details:
        raise RuntimeError(f'{name}: not a genuine ARM64 ELF')
    needed = re.findall(r'\(NEEDED\).*?\[(.*?)\]', details)
    unresolved = set(needed) - system - providers.keys()
    if unresolved:
        raise RuntimeError(f'{name}: providers missing {sorted(unresolved)}')
    if args.asan and name in ('libblackopsarm_engine.so', 'libbo1_native_shader_compiler.so'):
        if 'libclang_rt.asan-aarch64-android.so' not in needed or '__asan_init' not in details:
            raise RuntimeError(f'{name}: actual ASan runtime dependency/instrumentation is absent')
    if args.asan and name == 'libblackopsarm_engine.so' and 'libc++_shared.so' not in needed:
        raise RuntimeError('ASan engine does not use the staged shared STL')
    if name == 'libblackopsarm_engine.so':
        for symbol in ('SDL_main', 'Java_com_blackopsarm_dev_EngineActivity_nativeAdoptSDLActivity'):
            if not re.search(r'FUNC\s+GLOBAL\s+DEFAULT\s+\d+\s+'+symbol+r'\b', details):
                raise RuntimeError(f'Actual engine export missing: {symbol}')
        if not args.validate_only:
            (output / 'engine-elf.txt').write_text(details, encoding='utf-8')
    target = abi / name
    if not args.validate_only and (not target.exists() or target.read_bytes() != data):
        shutil.copyfile(source, target)
    if not args.validate_only and target.read_bytes() != data:
        raise RuntimeError(f'Native staging differs: {name}')
    build_id = re.search(r'Build ID:\s*(\w+)', details)
    manifest.append({'library': name, 'source': str(source), 'bytes': len(data),
                     'sha256': hashlib.sha256(data).hexdigest(),
                     'build_id': build_id.group(1) if build_id else None, 'needed': needed})
if not args.validate_only:
    (output / 'native-manifest.json').write_text(json.dumps(manifest, indent=2)+'\n', encoding='utf-8')
    if args.asan:
        wrap_data = wrapper.read_bytes()
        wrap_target = output / 'asan-resources/lib/arm64-v8a/wrap.sh'
        wrap_target.parent.mkdir(parents=True, exist_ok=True)
        wrap_target.write_bytes(wrap_data)
        (output / 'asan-wrap-manifest.json').write_text(json.dumps({
            'source': str(wrapper), 'apk_path': 'lib/arm64-v8a/wrap.sh',
            'sha256': hashlib.sha256(wrap_data).hexdigest(), 'bytes': len(wrap_data),
        }, indent=2)+'\n', encoding='utf-8')
print(f'{"Validated" if args.validate_only else "Staged"} {len(manifest)} actual ARM64 libraries; all recursive providers and SDL/JNI exports checked.')
