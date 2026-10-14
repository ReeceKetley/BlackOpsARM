"""Host f32 boundary diagnostics, not a GPU or driver-equivalence test."""
from pathlib import Path
import json
import math
import struct

OUT = Path(__file__).resolve().parent


def f32(x):
    return struct.unpack('<f', struct.pack('<f', x))[0]


def neighbors(x):
    bits = struct.unpack('<I', struct.pack('<f', x))[0]
    return [struct.unpack('<f', struct.pack('<I', bits + delta))[0] for delta in (-1, 0, 1)]


reciprocal = f32(1 / 255)
rows = []
for n in range(255):
    for x in neighbors(f32((n + .5) / 255)):
        # f64 exactly represents these f32*integer products plus .5.
        fused = math.floor(f32(x * 255 + .5))
        separate = math.floor(f32(f32(x * 255) + .5))
        ideal_half_up = math.floor(x * 255 + .5)
        ideal_nearest_even = round(x * 255)
        rows.append({'n': n, 'x': x, 'fused_mad_integer': fused,
                     'separate_mul_add_integer': separate,
                     'ideal_half_up_integer': ideal_half_up,
                     'ideal_nearest_even_integer': ideal_nearest_even})
decode_rows = [{'integer': n, 'reciprocal_multiply': f32(n * reciprocal),
                'correctly_rounded_ratio': f32(n / 255)} for n in range(256)]
result = {
    'status': 'host f32 arithmetic diagnostics only; actual UNORM conversion and shader MAD/pp behavior not measured',
    'boundary_inputs': len(rows),
    'mad_vs_separate_count': sum(r['fused_mad_integer'] != r['separate_mul_add_integer'] for r in rows),
    'candidate_mad_vs_ideal_half_up_count': sum(r['fused_mad_integer'] != r['ideal_half_up_integer'] for r in rows),
    'candidate_mad_vs_ideal_nearest_even_count': sum(r['fused_mad_integer'] != r['ideal_nearest_even_integer'] for r in rows),
    'decode_reciprocal_vs_ratio_count': sum(r['reciprocal_multiply'] != r['correctly_rounded_ratio'] for r in decode_rows),
    'mad_vs_separate_examples': [r for r in rows if r['fused_mad_integer'] != r['separate_mul_add_integer']][:5],
    'candidate_vs_half_up_examples': [r for r in rows if r['fused_mad_integer'] != r['ideal_half_up_integer']][:5],
    'candidate_vs_nearest_even_examples': [r for r in rows if r['fused_mad_integer'] != r['ideal_nearest_even_integer']][:5],
    'decode_examples': [r for r in decode_rows if r['reciprocal_multiply'] != r['correctly_rounded_ratio']][:5],
    'max_decode_absolute_error': max(abs(r['reciprocal_multiply'] - r['correctly_rounded_ratio']) for r in decode_rows)
}
assert len(rows) == 765 and len(decode_rows) == 256
# Verify explicit saturation endpoints in the candidate arithmetic.
for x, expected in [(-1., 0.), (0., 0.), (1., 1.), (2., 1.)]:
    q = f32(math.floor(f32(min(1., max(0., x)) * 255 + .5)) * reciprocal)
    assert q == expected
(OUT / 'quantization-boundaries.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf8')
(OUT / 'quantization-boundaries-all.json').write_text(json.dumps({'boundaries': rows, 'decode': decode_rows}, indent=2) + '\n', encoding='utf8')
print(json.dumps(result, indent=2))
