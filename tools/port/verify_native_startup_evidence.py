"""Check final startup builds, frozen prior evidence and protected references."""
from pathlib import Path
import argparse
import json
import subprocess
from verify_startup_reader_evidence import digest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    output = (root / args.output).resolve()
    if not output.is_relative_to(root) or output.exists():
        raise RuntimeError('Use fresh worktree evidence')
    protected = json.loads((root / 'build-android-audit/default_resolution_baseline/hashes.json').read_text(encoding='utf-8-sig'))
    changed = [path for path, sha in protected.items() if not Path(path).is_file() or digest(path) != sha.lower()]
    previous_git = json.loads((root / 'build-android-audit/default_resolution_baseline/git.json').read_text(encoding='utf-8-sig'))
    current_git = {}
    for name in previous_git:
        current_git[name] = {key: subprocess.check_output(['git', '-C', str(root.parent / name)] + command, text=True)
                             for key, command in [('head', ['rev-parse', 'HEAD']), ('branch', ['branch', '--show-current']), ('status', ['status', '--short'])]}
    archives = []
    for name in ('arm64_default_resolution', 'arm64_startup_reader'):
        archive = root / 'docs/port/evidence' / name
        inventory = json.loads((archive / 'inventory.json').read_text(encoding='utf-8-sig'))
        mismatches = [row['archive'] for row in inventory['files'] if digest(archive / row['archive']) != row['sha256']]
        archives.append({'archive': name, 'records': len(inventory['files']), 'changed': mismatches})
    builds = []
    for directory in ('native_startup_zone_host_final1/x86', 'native_startup_zone_host_final1/x64',
                      'native_startup_zone_arm_O0_final2', 'native_startup_zone_arm_O3_final2',
                      'native_zone_load_normal_probe2/x86', 'native_zone_load_normal_probe2/x64'):
        manifest = root / 'build-android-audit' / directory / 'source-manifest.json'
        inputs = json.loads(manifest.read_text(encoding='utf-8-sig'))
        mismatches = [path for path, sha in inputs.items() if digest(root / path) != sha.lower()]
        builds.append({'build': directory, 'inputs': len(inputs), 'changed': mismatches})
    result = {'protected_paths': len(protected), 'changed': changed,
              'reference_git_unchanged': current_git == previous_git, 'git': current_git,
              'frozen_archives': archives, 'build_manifests': builds}
    result['passed'] = not changed and current_git == previous_git and not any(row['changed'] for row in archives + builds)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    if not result['passed']:
        raise RuntimeError('Final evidence/preservation check failed; evidence retained')
    print('Protected references, both prior archives and six final build manifests pass')


if __name__ == '__main__':
    main()
