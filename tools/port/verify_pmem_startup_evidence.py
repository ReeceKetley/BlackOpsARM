"""Verify the two reviewed production deltas and current PMem/startup evidence."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    if not output.is_relative_to(ROOT) or output.exists():
        raise RuntimeError('Use fresh worktree evidence')
    deltas = {
        'src/universal/physicalmemory.cpp': (
            '            allocEntry = (PhysicalMemoryAllocation *)(&prim->allocListCount + 2 * prim->allocListCount);\n',
            '''#if BO1_NATIVE_64BIT
            // The decompiled Win32 expression advances eight bytes per record.
            // Native records contain an eight-byte name pointer and padding.
            allocEntry = &prim->allocList[prim->allocListCount - 1];
#else
            allocEntry = (PhysicalMemoryAllocation *)(&prim->allocListCount + 2 * prim->allocListCount);
#endif
'''),
        'src/platform/native_zone_file.cpp': (
            '''#if BO1_NATIVE_64BIT
    // The legacy PMem third-record native stride is not verified. Contain this
    // component to the already verified two-owner boundary; do not alter PMem.
    if (g_mem.prim[type].allocListCount >= 2) return false;
#endif
''', ''),
    }
    modified = subprocess.check_output(['git', 'diff', 'c297348', '--name-only', '--', 'src', 'android'], cwd=ROOT, text=True).splitlines()
    if set(modified) != set(deltas):
        raise RuntimeError('Unexpected production source delta')
    proof = {}
    for path, (old, new) in deltas.items():
        before = subprocess.check_output(['git', 'show', 'c297348:' + path], cwd=ROOT).decode('utf-8-sig').replace('\r\n', '\n')
        after = (ROOT / path).read_text(encoding='utf-8-sig')
        if before.count(old) != 1 or after != before.replace(old, new):
            raise RuntimeError('Reviewed exact delta differs: ' + path)
        proof[path] = {'before_sha256': hashlib.sha256(before.encode()).hexdigest(), 'after_sha256': hashlib.sha256(after.encode()).hexdigest()}
    protected = json.loads((ROOT / 'build-android-audit/default_resolution_baseline/hashes.json').read_text(encoding='utf-8-sig'))
    if any(digest(path) != sha.lower() for path, sha in protected.items()):
        raise RuntimeError('Protected file changed')
    reference_git = json.loads((ROOT / 'build-android-audit/default_resolution_baseline/git.json').read_text(encoding='utf-8-sig'))
    for name, expected in reference_git.items():
        actual = {key: subprocess.check_output(['git', '-C', str(ROOT.parent / name)] + command, text=True)
                  for key, command in [('head', ['rev-parse', 'HEAD']), ('branch', ['branch', '--show-current']), ('status', ['status', '--short'])]}
        if actual != expected:
            raise RuntimeError('Protected reference Git changed')
    archives = {}
    for name in ('arm64_default_resolution', 'arm64_startup_reader', 'arm64_native_startup_zone'):
        directory = ROOT / 'docs/port/evidence' / name
        rows = json.loads((directory / 'inventory.json').read_text())['files']
        if any(digest(directory / row['archive']) != row['sha256'] for row in rows):
            raise RuntimeError('Frozen archive changed')
        archives[name] = len(rows)
    build_dirs = []
    for name in ('pmem_owner_stack_after1', 'native_zone_file_multiowner2', 'native_startup_zone_pmem1', 'native_zone_pmem_normal1'):
        build_dirs.extend(ROOT / 'build-android-audit' / name / arch for arch in ('x86', 'x64'))
    build_dirs.extend(ROOT / 'build-android-audit' / name for name in ('native_startup_zone_pmem_O0_1', 'native_startup_zone_pmem_O3_1'))
    for directory in build_dirs:
        result = json.loads((directory / 'build-result.json').read_text(encoding='utf-8-sig'))
        if not result['completed']:
            raise RuntimeError('Build did not pass')
        inputs = json.loads((directory / 'source-manifest.json').read_text(encoding='utf-8-sig'))
        if any(digest(ROOT / path) != sha.lower() for path, sha in inputs.items()):
            raise RuntimeError('Build source no longer current: ' + str(directory))
    for arch in ('x86', 'x64'):
        for name in ('pmem_owner_stack_after1', 'native_zone_file_multiowner2', 'native_startup_zone_pmem1'):
            directory = ROOT / 'build-android-audit' / name / arch
            if json.loads((directory / 'run-result.json').read_text(encoding='utf-8-sig'))['exit'] != 0:
                raise RuntimeError('Host runtime failed')
        if not json.loads((ROOT / 'build-android-audit/native_startup_zone_pmem1' / arch / 'collector-result.json').read_text())['passed']:
            raise RuntimeError('Host independent collector failed')
    for opt in ('O0', 'O3'):
        if not json.loads((ROOT / f'build-android-audit/native_startup_zone_pmem_{opt}_run1/result.json').read_text())['passed']:
            raise RuntimeError('HONOR collector failed')
    result = {'passed': True, 'baseline': 'c297348', 'reviewed_production_deltas': proof,
              'protected_paths': len(protected), 'reference_git_unchanged': True,
              'frozen_archive_records': archives, 'current_build_manifests': len(build_dirs),
              'scope': 'Native Windows PMem stack fix; ARM uses its existing named-zone backend. No DB worker/Com_Init claim.'}
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print('Reviewed PMem deltas, current builds/runtime and protected evidence pass')


if __name__ == '__main__':
    main()
