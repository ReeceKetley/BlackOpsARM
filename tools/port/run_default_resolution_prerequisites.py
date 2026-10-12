"""Retain prior registration checks; new target now uses the actual default fatal."""
from pathlib import Path


def patch_default_control(source):
    lines = [line for line in source.splitlines() if 'Unexpected unavailable default output' in line]
    if len(lines) != 1:
        raise RuntimeError('Unique historical default diagnostic check required')
    diagnostic = "Could not load default asset '' for asset type 'rawfile'.\nTried to load asset 'not-present'."
    literal = repr(diagnostic).replace('\\', '\\\\')
    replacement = "            if (out/(fatal_label+'.stdout.log')).read_text() or (out/(fatal_label+'.stderr.log')).read_text()!=" + literal + ":raise RuntimeError('Unexpected actual missing-default output')"
    return source.replace(lines[0], replacement)


base = Path(__file__).with_name('run_code_zone_gate.py')
program = base.read_text(encoding='utf-8-sig')
program = program.replace('code_zone.exe', 'default_resolution.exe').replace('bo1_code_zone_tests', 'bo1_default_resolution_tests')
anchor = "program=base.read_text(encoding='utf-8-sig')"
if program.count(anchor) != 1:
    raise RuntimeError('Unique registry-gate loading anchor required')
program = program.replace(anchor, "program=patch_default_control(base.read_text(encoding='utf-8-sig'))")
exec(compile(program, str(base), 'exec'), globals())
