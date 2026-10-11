"""Build a focused test from the current production glass allocator source."""
from pathlib import Path
import argparse
import re
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--baseline', action='store_true')
args = parser.parse_args()

root = Path(__file__).resolve().parents[2]
header = (root / "src/glass/glass_allocator.h").read_bytes().decode("latin1")
source = (subprocess.check_output(['git', 'show', '17003a7:src/glass/glass_allocator.cpp'], cwd=root)
          if args.baseline else (root / "src/glass/glass_allocator.cpp").read_bytes()).decode("latin1")
header = re.sub(r'^#(?:pragma|include).*$', '', header, flags=re.M)
source = re.sub(r'^#include.*$', '', source, flags=re.M)
test = (root / "tools/port/test_glass_allocator.cpp").read_text()
prelude, body = test.split("// PRODUCTION_ALLOCATOR", 1)
output = root / ("build/run/test_glass_allocator_baseline.cpp" if args.baseline
                 else "build/run/test_glass_allocator.cpp")
output.write_text("\n".join((prelude, header, source, body)))
print(output)
