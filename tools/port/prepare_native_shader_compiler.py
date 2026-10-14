"""Fetch and extract the pinned, unmodified CPU shader compiler sources."""
import hashlib
import json
from pathlib import Path
import tarfile
import urllib.request

root = Path(__file__).resolve().parents[2]
cache = root / 'build-android-audit/native_registry_closure/vkd3d-source-inspect'
cache.mkdir(parents=True, exist_ok=True)
archive = cache / 'vkd3d-1.17.tar.xz'
url = 'https://dl.winehq.org/vkd3d/source/vkd3d-1.17.tar.xz'
expected = 'bc61cb9e84d5045cbcaffbdd707940d399d8bf62874663dfe5809a0bfb87e9b6'
if not archive.exists():
    urllib.request.urlretrieve(url, archive)
digest = hashlib.sha256(archive.read_bytes()).hexdigest()
if digest != expected:
    raise RuntimeError('Pinned vkd3d source archive hash mismatch')
destination = root / 'third_party/native-shader-compiler'
destination.mkdir(parents=True, exist_ok=True)
source = destination / 'vkd3d-1.17'
if not source.exists():
    with tarfile.open(archive, 'r:xz') as package:
        package.extractall(destination, filter='data')

commit = 'aa6cef192b8e693916eb713e7a9ccadf06062ceb'
header_root = destination / 'spirv-headers'
header_root.mkdir(exist_ok=True)
files = ('include/spirv/unified1/spirv.h', 'include/spirv/unified1/GLSL.std.450.h', 'LICENSE')
header_hashes = {
    'include/spirv/unified1/spirv.h': '6266690ea0097e6e137d343a38444debecfe06d765b0ab9c256b8cf5918e7aae',
    'include/spirv/unified1/GLSL.std.450.h': '8bca15bf3e149e5927e366adfe048c00535273e81dd21e3f0f0bc1f58fd9f293',
    'LICENSE': 'ff04ae7e6a468587fe59434fee2e67c0e2a749b592c794b764ff80f30b94a084',
}
records = []
for relative in files:
    target = header_root / relative
    target.parent.mkdir(parents=True, exist_ok=True)
    remote = f'https://raw.githubusercontent.com/KhronosGroup/SPIRV-Headers/{commit}/{relative}'
    if not target.exists():
        urllib.request.urlretrieve(remote, target)
    header_digest = hashlib.sha256(target.read_bytes()).hexdigest()
    if header_digest != header_hashes[relative]:
        raise RuntimeError(f'Pinned SPIR-V header hash mismatch: {relative}')
    records.append({'path': relative, 'url': remote, 'sha256': header_digest})
manifest = {'vkd3d': {'version': '1.17', 'url': url, 'sha256': digest},
            'spirv_headers': {'commit': commit, 'files': records}}
(destination / 'source-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
print('Prepared pinned vkd3d 1.17 CPU compiler sources and SPIR-V enum headers; licenses preserved')
