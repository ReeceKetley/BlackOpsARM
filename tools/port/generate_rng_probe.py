"""Generate a test-only production RNG body probe; never copy an engine unit."""
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parents[2]
# Existing decompiler comments use Windows bytes; extracted functions are ASCII.
# Decode byte-preservingly rather than rewriting the production source encoding.
source = (root / 'src/universal/com_math.cpp').read_bytes().decode('latin1')
baseline = subprocess.check_output(['git', 'show', '76edfd5:src/universal/com_math.cpp'], cwd=root).decode('latin1')

def function(text, name):
    start = text.index('double __cdecl ' + name + '()')
    begin = text.index('{', start)
    end = begin + 1
    depth = 1
    while depth:
        if text[end] == '{': depth += 1
        if text[end] == '}': depth -= 1
        end += 1
    return text[start:end]

parts = ['#include <universal/com_math.h>\n#include <platform/engine_random.h>\n'
         '#include <windows_crt_rng_vectors.h>\n#include <cstdlib>\n#include <cstdio>',
         function(source, 'BO1_Random'), function(source, 'crandom'),
         '#if !defined(__ANDROID__)', function(source, 'random'),
         function(baseline, 'random').replace('random()', 'LegacyRandom()', 1), '#endif', r'''
int main()
{
    for (const auto &sequence : bo1::test::windowsRng) {
        BO1_CrtSrand(sequence.seed);
        for (int raw : sequence.draws) {
            const double expected = static_cast<double>(raw) / 32768.0;
            const double actual = BO1_Random();
            if (actual != expected || actual < 0.0 || actual >= 1.0) std::abort();
        }
        BO1_CrtSrand(sequence.seed);
        for (int raw : sequence.draws) {
            const double expected = static_cast<double>(raw) / 32768.0 * 2.0 - 1.0;
            const double actual = crandom();
            if (actual != expected || actual < -1.0 || actual >= 1.0) std::abort();
        }
        // Direct and normalized consumers must advance the same stream.
        BO1_CrtSrand(sequence.seed);
        for (int i = 0; i < 128; ++i) {
            const double expected = static_cast<double>(sequence.draws[i]) / 32768.0;
            const double actual = i & 1 ? BO1_Random() : static_cast<double>(BO1_CrtRand()) / 32768.0;
            if (actual != expected) std::abort();
        }
#if !defined(__ANDROID__)
        BO1_CrtSrand(sequence.seed);
        for (int raw : sequence.draws) if (random() != static_cast<double>(raw) / 32768.0) std::abort();
        BO1_CrtSrand(sequence.seed);
        for (int raw : sequence.draws) if (LegacyRandom() != static_cast<double>(raw) / 32768.0) std::abort();
#endif
    }
    std::puts("PASS production RNG bodies: captured Windows streams, [0,1) / [-1,1) ranges, mixed raw/normalized draws; unchanged scaling and Windows legacy entry point");
}
''']
Path(sys.argv[1]).write_text('\n\n'.join(parts), encoding='utf-8')
