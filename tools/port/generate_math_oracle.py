"""Extract only the original function for a Windows behavior comparison probe."""
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parents[2]
original = subprocess.check_output(['git', 'show', '76edfd5:src/universal/com_math_anglevectors.cpp'], cwd=root).decode()
start = original.index('void __cdecl AxisToSignedAngles(')
body = original.index('{', start)
depth = 1
end = body + 1
while depth:
    if original[end] == '{': depth += 1
    if original[end] == '}': depth -= 1
    end += 1
output = Path(sys.argv[1])
output.write_text(original[start:end].replace('AxisToSignedAngles(', 'LegacyAxisToSignedAngles(', 1) + '\n')
