"""Inventory every legacy flag reference and generate compiler guard checks.

The reviewed, per-site classification is evidence, not a search/replace rule.
This tool never edits engine sources.
"""
from pathlib import Path
import argparse
import collections
import json
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('output', type=Path)
parser.add_argument('--refresh-evidence', action='store_true')
args = parser.parse_args()
evidence = ROOT / 'docs/port/evidence/arm64_phase1'
audit = json.loads((evidence / 'flag_guard_audit.json').read_text())
guards = audit['guards']
for row in guards:
    if row['file'] == 'src/universal/narrow32.h':
        row['new_guard'] = '#if BO1_NATIVE_64BIT'
    if row['file'] == 'src/clientscript/cscr_variable.h':
        row['new_guard'] = '#if defined(BO1_PORT_X64) || defined(__ANDROID__)'
audit['counts'] = dict(collections.Counter(row['category'] for row in guards))
if args.refresh_evidence:
    (evidence / 'flag_guard_audit.json').write_text(json.dumps(audit, indent=2) + '\n')

references = []
matches = subprocess.check_output(['git', 'grep', '-n', '-w', 'BO1_PORT_X64', '76edfd5'],
                                  cwd=ROOT).decode('utf-8')
for match in matches.splitlines():
    _, file, number, line = match.split(':', 3)
    if file == 'CMakeLists.txt':
        kind = 'Windows build configuration retained'
    elif file.startswith('docs/') or line.lstrip().startswith('//'):
        kind = 'historical documentation/comment retained'
    elif file.startswith('tools/'):
        kind = 'Windows regression/build/run tooling retained'
    elif re.match(r'\s*#\s*(?:if|elif)', line):
        kind = 'reviewed conditional; see flag_guard_audit.json'
    else:
        kind = 'closing guard comment retained'
    references.append(dict(file=file, original_line=int(number), kind=kind, text=line.strip()))
if args.refresh_evidence:
    (evidence / 'legacy_flag_references.json').write_text(json.dumps({
        'baseline': '76edfd5', 'references': references,
        'reference_count': len(references), 'file_count': len(set(r['file'] for r in references)),
    }, indent=2) + '\n')

output = args.output
parts = ['#include <platform/bo1_native_width.h>']
for i, row in enumerate(guards):
    for label, key in [('original', 'original_guard'), ('current', 'new_guard')]:
        directive = row[key]
        # A stand-alone conditional preserves the expression of an elif.
        directive = re.sub(r'^#\s*elif', '#if', directive)
        parts += [directive, '#define ' + label + str(i) + ' 1', '#else',
                  '#define ' + label + str(i) + ' 0', '#endif']
    parts += ['#if defined(_WIN32)',
              f'static_assert(original{i} == current{i}, "Windows guard changed: {row["file"]}:{row["original_line"]}");',
              '#endif']
parts += ['#if defined(__ANDROID__) && defined(__aarch64__)',
          'static_assert(BO1_NATIVE_64BIT == 1);',
          '#ifdef BO1_PORT_X64', '#error Android must not require the Windows test flag', '#endif', '#endif']
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text('\n'.join(parts) + '\n')
print(f'Audited {len(references)} references in {len(set(r["file"] for r in references))} files; {len(guards)} conditional guards.')
