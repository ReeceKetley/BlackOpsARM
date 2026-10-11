"""Deterministic independent zlib fixtures; generated only in fresh ignored builds."""
from pathlib import Path
import hashlib
import json
import struct
import sys
import zlib

output = Path(sys.argv[1]).resolve()
output.mkdir(parents=True, exist_ok=True)
cases = [0, 1, 127, 32767, 32768, 32769, 524287, 524288, 524289]
metadata = {'generatorZlib': zlib.ZLIB_RUNTIME_VERSION, 'files': {}}
for length in cases:
    state = 11
    payload = bytearray(length)
    for i in range(length):
        state = (state * 1664525 + 1013904223) & 0xffffffff
        payload[i] = state >> 24
    # Allocation/external fields deliberately differ from serialized payload size.
    header = struct.pack('<9I', length, 0xfffffffe, 36, 0xffffffff, 13, 0, 4097, 0, 0)
    decoded = header + payload
    data = b'IWffu100' + struct.pack('<I', 473) + zlib.compress(decoded)
    name = f'payload-{length}.ff'
    (output/name).write_bytes(data)
    metadata['files'][name] = {'fileBytes': len(data), 'decodedBytes':len(decoded),
                             'fileSHA256':hashlib.sha256(data).hexdigest(),
                             'decodedSHA256':hashlib.sha256(decoded).hexdigest()}
# Short valid stream and preset-dictionary stream exercise distinct failures.
for name, decoded, dictionary in [('short-xfile.ff',b'X'*35,None),('dictionary.ff',b'X'*100,b'T5 dictionary')]:
    encoder = zlib.compressobj(zdict=dictionary) if dictionary else zlib.compressobj()
    data=b'IWffu100'+struct.pack('<I',473)+encoder.compress(decoded)+encoder.flush()
    (output/name).write_bytes(data)
    metadata['files'][name]={'fileSHA256':hashlib.sha256(data).hexdigest(),'fileBytes':len(data)}
# Correct stream with inconsistent XFile.size in either direction.
for delta in [-1,1]:
    data = b'IWffu100'+struct.pack('<I',473)+zlib.compress(struct.pack('<9I',127+delta,*([0]*8))+bytes(127))
    name=f'mismatch-{delta}.ff'; (output/name).write_bytes(data)
    metadata['files'][name]={'fileSHA256':hashlib.sha256(data).hexdigest(),'fileBytes':len(data)}
pattern = (b'T5 Zombies' * 13108)[:131073]
decoded = struct.pack('<9I',len(pattern),*([0]*8)) + pattern
for name, window in [('pattern.ff',15),('raw-deflate.ff',-15),('gzip.ff',31)]:
    encoder=zlib.compressobj(wbits=window)
    data=b'IWffu100'+struct.pack('<I',473)+encoder.compress(decoded)+encoder.flush()
    (output/name).write_bytes(data)
    metadata['files'][name]={'fileSHA256':hashlib.sha256(data).hexdigest(),'fileBytes':len(data),
                           'decodedSHA256':hashlib.sha256(decoded).hexdigest(),'decodedBytes':len(decoded)}
(output/'manifest.json').write_text(json.dumps(metadata,indent=2)+'\n')
(output/'ready.stamp').write_text('Independent fixtures ready\n')
print('Generated',len(metadata['files']),'independent zlib fixtures')
