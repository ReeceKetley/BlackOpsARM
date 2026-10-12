"""Verify reader build manifests and inherited protected checkout/evidence hashes."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess


def digest(path):
    value = hashlib.sha256()
    with Path(path).open('rb') as stream:
        while chunk := stream.read(1024 * 1024):
            value.update(chunk)
    return value.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    output = (root / args.output).resolve()
    if not output.is_relative_to(root) or output.exists():
        raise RuntimeError('Use fresh worktree evidence path')
    protected = json.loads((root / 'build-android-audit/default_resolution_baseline/hashes.json').read_text(encoding='utf-8-sig'))
    changed = [path for path, sha in protected.items()
               if not Path(path).is_file() or digest(path) != sha.lower()]
    references = json.loads((root / 'build-android-audit/default_resolution_baseline/git.json').read_text(encoding='utf-8-sig'))
    current = {}
    for name in references:
        directory = root.parent / name
        current[name] = {key: subprocess.check_output(['git', '-C', str(directory)] + args, text=True)
                         for key, args in [('head', ['rev-parse', 'HEAD']), ('branch', ['branch', '--show-current']), ('status', ['status', '--short'])]}
    manifests = []
    for directory in ('native_zone_file_host_final3/x86', 'native_zone_file_host_final3/x64',
                      'native_zone_file_arm_O0_final2', 'native_zone_file_arm_O3_final2'):
        path = root / 'build-android-audit' / directory / 'source-manifest.json'
        inputs = json.loads(path.read_text(encoding='utf-8-sig'))
        mismatches = [unit for unit, sha in inputs.items() if digest(root / unit) != sha.lower()]
        manifests.append({'build': directory, 'inputs': len(inputs), 'changed': mismatches})
    archive = root / 'docs/port/evidence/arm64_default_resolution'
    inventory = json.loads((archive / 'inventory.json').read_text(encoding='utf-8-sig'))
    archive_changed = [row['archive'] for row in inventory['files']
                       if digest(archive / row['archive']) != row['sha256']]
    result = {'protected_paths': len(protected), 'changed': changed,
              'reference_git_unchanged': current == references, 'git': current,
              'reader_manifests': manifests, 'previous_checkpoint_archive_records': len(inventory['files']),
              'previous_checkpoint_archive_changed': archive_changed}
    result['passed'] = not changed and current == references and not archive_changed and not any(row['changed'] for row in manifests)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    if not result['passed']:
        raise RuntimeError('Evidence/preservation verification failed; result retained')
    print(f"Protected {len(protected)} paths, previous archive and final reader manifests pass")


if __name__ == '__main__':
    main()
