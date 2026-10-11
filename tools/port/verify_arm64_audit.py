"""Verify protected checkouts and report changes against a captured baseline."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def git(repo, *args):
    return subprocess.check_output(['git', '--no-optional-locks', *args], cwd=repo, text=True).strip()


def files(repo):
    names = git(repo, 'ls-files', '--cached', '--others', '--exclude-standard').splitlines()
    return {name: sha(repo / name) for name in sorted(set(names)) if (repo / name).is_file()}


def snapshot(repo):
    index = Path(git(repo, 'rev-parse', '--git-path', 'index'))
    if not index.is_absolute():
        index = repo / index
    return dict(head=git(repo, 'rev-parse', 'HEAD'), branch=git(repo, 'branch', '--show-current'),
                status=git(repo, 'status', '--porcelain=v2', '--branch', '--untracked-files=all').splitlines(),
                indexSHA256=sha(index), files=files(repo))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    baseline = json.loads(args.baseline.read_text(encoding='utf-8-sig'))
    report = {'protected': {}}
    for name in ['BlackOps64', 'DXVK-Native']:
        current = snapshot(ROOT.parent / name)
        original = baseline[name]
        differences = [key for key in original if original[key] != current[key]]
        report['protected'][name] = dict(unchanged=not differences, differences=differences,
                                       filesVerified=len(current['files']), head=current['head'], branch=current['branch'])
        if differences:
            raise RuntimeError(name + ' differs from baseline: ' + ', '.join(differences))
    root_files = {str(p): sha(p) for p in (ROOT.parent / '.ledger').rglob('*') if p.is_file()}
    if root_files != baseline['rootLedger']:
        raise RuntimeError('Root Ledger differs from baseline')
    report['protected']['rootLedger'] = dict(unchanged=True, filesVerified=len(root_files))
    current = files(ROOT)
    for name in ['.ledger/.lock', '.ledger/ledger.db', 'android/local.properties']:
        path = ROOT / name
        if path.is_file():
            current[name] = sha(path)
    original = baseline['BlackOpsARM']['files']
    report['created'] = sorted(set(current) - set(original))
    report['modified'] = sorted(name for name in set(current) & set(original) if current[name] != original[name])
    report['removed'] = sorted(set(original) - set(current))
    report['android'] = dict(head=git(ROOT, 'rev-parse', 'HEAD'), branch=git(ROOT, 'branch', '--show-current'),
                             worktrees=git(ROOT, 'worktree', 'list', '--porcelain'))
    def git_path(repo, *options):
        path = Path(git(repo, 'rev-parse', *options))
        return (path if path.is_absolute() else repo / path).resolve()
    windows = ROOT.parent / 'BlackOps64'
    dxvk = ROOT.parent / 'DXVK-Native'
    common = git_path(ROOT, '--git-common-dir')
    shared = common == git_path(windows, '--git-common-dir')
    independent = git_path(ROOT, '--git-path', 'index') != git_path(windows, '--git-path', 'index')
    dxvk_independent = common != git_path(dxvk, '--git-common-dir')
    if not shared or not independent or not dxvk_independent:
        raise RuntimeError('Unexpected worktree/repository relationship')
    report['android'].update(commonGitDirectory=str(common), windowsHistoryShared=shared,
                             independentIndex=independent, independentWorkingDirectory=ROOT != windows)
    report['protected']['DXVK-Native']['independentRepository'] = dxvk_independent
    if report['removed'] or report['android']['head'] != baseline['BlackOpsARM']['head'] or report['android']['branch'] != 'android-arm64':
        raise RuntimeError('Unexpected Android removal/commit/branch change')
    if current['android/local.properties'] != original['android/local.properties']:
        raise RuntimeError('Local SDK path changed')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS preservation:', ', '.join(f'{k}={v["filesVerified"]} files' for k, v in report['protected'].items()))
    print(f'Phase 1: {len(report["modified"])} modified, {len(report["created"])} created, {len(report["removed"])} removed; no commit/push.')


if __name__ == '__main__':
    main()
