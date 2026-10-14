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
system = {'libandroid.so', 'libmediandk.so', 'liblog.so', 'libvulkan.so', 'libm.so',
          'libdl.so', 'libc.so', 'libz.so', 'libGLESv1_CM.so', 'libGLESv2.so', 'libEGL.so',
          'libOpenSLES.so', 'libaaudio.so', 'libatomic.so'}
readelf = ndk / 'bin/llvm-readelf.exe'
import re
manifest = []
abi = output / 'arm64-v8a'
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
    if name == 'libblackopsarm_engine.so':
        for symbol in ('SDL_main', 'Java_com_blackopsarm_dev_EngineActivity_nativeAdoptSDLActivity'):
            if not re.search(r'FUNC\s+GLOBAL\s+DEFAULT\s+\d+\s+'+symbol+r'\b', details):
                raise RuntimeError(f'Actual engine export missing: {symbol}')
        (output / 'engine-elf.txt').write_text(details, encoding='utf-8')
    target = abi / name
    if not target.exists() or target.read_bytes() != data:
        shutil.copyfile(source, target)
    if target.read_bytes() != data:
        raise RuntimeError(f'Native staging differs: {name}')
    build_id = re.search(r'Build ID:\s*(\w+)', details)
    manifest.append({'library': name, 'source': str(source), 'bytes': len(data),
                     'sha256': hashlib.sha256(data).hexdigest(),
                     'build_id': build_id.group(1) if build_id else None, 'needed': needed})
(output / 'native-manifest.json').write_text(json.dumps(manifest, indent=2)+'\n', encoding='utf-8')
print(f'Staged {len(manifest)} actual ARM64 libraries; all recursive providers and SDL/JNI exports checked.')
