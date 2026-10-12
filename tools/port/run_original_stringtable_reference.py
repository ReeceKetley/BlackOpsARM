"""Execute pinned original x86 StringTable leaf/sort instructions as an oracle.

Never launch the original executable, stub a service, emit asset bytes, or use
this reference emulator as an Android implementation. Only guest indices change.
"""
import argparse
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import struct
import sys
import time
import traceback

sys.dont_write_bytecode = True
os.environ['PYTHONDONTWRITEBYTECODE'] = '1'
ROOT = Path(__file__).resolve().parents[2]
EXE_SHA = '2ba4a04e81de2b60582da0e4489efde21e6cb07904a223bae633b6ccaebbf759'
FUNCTIONS = ('first_hash_index', 'signed_normalized_comparator',
             'signed_sort_wrapper', 'original_qsort',
             'original_qsort_small_sort', 'original_qsort_swap')
PINNED_SHAS = (
    '270eca99cdc5b74eaa744407453413a43981f35cdef8b5363237843bb93e8485',
    '6e376f5f3477b96a8bb29792b77eee6f609db80b0428f317cb6bcffaadc8a274',
    '6a86087edcdba50d113f625882d61359798b2b6092b0035a3f10981712e1bfc2',
    '946955903d496eaee752e1483958538191dcb153fe4746c03d53267df7f4a159',
    '1f4f6ed830b09d3f9c3f908b0d0a328de02f1c91deff46a8d29410a6bed8f7de',
    '860ef2326247c3c3a07307da7a19cfd7ad1d2f54425a87760bd6da3729cad250')
PINNED_RANGES = ((0x8604b0, 0x8604f4), (0x860500, 0x860550),
                 (0x5c03f0, 0x5c041f), (0x967090, 0x9672dd),
                 (0x967000, 0x967083), (0x966fd0, 0x966ff2))
TABLE, CELLS, INDICES = 0x10000000, 0x10001000, 0x10010000
STACK, STACK_SIZE, GLOBAL = 0x20000000, 0x100000, 0x26050d8
STOP = 0x1001f000


def sha(data):
    return hashlib.sha256(data).hexdigest()


def pe_layout(raw):
    pe = struct.unpack_from('<I', raw, 0x3c)[0]
    if raw[pe:pe + 4] != b'PE\0\0':
        raise RuntimeError('Not PE')
    machine, count = struct.unpack_from('<HH', raw, pe + 4)
    optional_size = struct.unpack_from('<H', raw, pe + 20)[0]
    optional = pe + 24
    if machine != 0x14c or struct.unpack_from('<H', raw, optional)[0] != 0x10b:
        raise RuntimeError('Expected pinned x86 PE32')
    base = struct.unpack_from('<I', raw, optional + 28)[0]
    image_size, header_size = struct.unpack_from('<II', raw, optional + 56)
    sections = []
    for i in range(count):
        at = optional + optional_size + i * 40
        virtual_size, rva, raw_size, offset = struct.unpack_from('<IIII', raw, at + 8)
        sections.append(dict(name=raw[at:at + 8].rstrip(b'\0').decode('ascii'),
                             rva=rva, virtual_size=virtual_size,
                             raw_size=raw_size, file_offset=offset))
    if base != 0x400000:
        raise RuntimeError('Unexpected preferred base')
    return base, image_size, header_size, sections


class OriginalGuest:
    def __init__(self, raw, mapping, hashes, original_indices, columns, rows):
        from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
        from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX,
                                      UC_X86_REG_EBP, UC_X86_REG_ESI,
                                      UC_X86_REG_EDI, UC_X86_REG_ESP)
        self.regs = (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_EBP,
                     UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_ESP)
        self.uc = Uc(UC_ARCH_X86, UC_MODE_32)
        self.functions = mapping
        base, size, headers, self.sections = pe_layout(raw)
        self.uc.mem_map(base, (size + 4095) & ~4095)
        self.uc.mem_write(base, raw[:headers])
        for section in self.sections:
            self.uc.mem_write(base + section['rva'],
                              raw[section['file_offset']:section['file_offset'] + section['raw_size']])
        self.uc.mem_map(TABLE, 0x20000)
        self.uc.mem_map(STACK, STACK_SIZE)
        self.uc.mem_write(TABLE, struct.pack('<5I', 0, columns, rows, CELLS, INDICES))
        self.cells = b''.join(struct.pack('<Ii', 0, h) for h in hashes)
        self.original = struct.pack('<' + 'h' * len(original_indices), *original_indices)
        self.uc.mem_write(CELLS, self.cells)
        self.uc.mem_write(INDICES, self.original)
        self.count = len(original_indices)
        self.histogram = Counter()
        self.total_steps = 0
        self.failure = None
        self.uc.hook_add(UC_HOOK_CODE, self.on_code)
        self.uc.hook_add(UC_HOOK_MEM_WRITE, self.on_write)

    def stop_error(self, message):
        self.failure = message
        self.uc.emu_stop()

    def on_code(self, uc, address, size, data):
        self.steps += 1
        self.total_steps += 1
        if self.steps > self.budget or time.monotonic() > self.deadline:
            return self.stop_error('Instruction/time budget exceeded')
        for name, f in self.functions.items():
            if f['va'] <= address and address + size <= f['end_va_exclusive']:
                self.histogram[name] += 1
                return
        self.stop_error(f'Unsupported original code dispatch at {address:#x}')

    def on_write(self, uc, access, address, size, value, data):
        allowed = ((STACK, STACK + STACK_SIZE),
                   (INDICES, INDICES + len(self.original)), (GLOBAL, GLOBAL + 4))
        if not any(start <= address and address + size <= end for start, end in allowed):
            self.stop_error(f'Unexpected guest write at {address:#x}, bytes={size}')

    def invoke(self, name, args, eax=0, budget=4096):
        eax_reg, ebx, ebp, esi, edi, esp_reg = self.regs
        sp = STACK + STACK_SIZE - 0x100
        self.uc.mem_write(sp, struct.pack('<' + 'I' * (len(args) + 1), STOP, *args))
        canaries = {ebx: 0x12345678, ebp: 0x23456789, esi: 0x3456789a, edi: 0x456789ab}
        for reg, value in canaries.items():
            self.uc.reg_write(reg, value)
        self.uc.reg_write(eax_reg, eax)
        self.uc.reg_write(esp_reg, sp)
        self.steps, self.budget = 0, budget
        self.deadline = time.monotonic() + 30
        self.failure = None
        self.uc.emu_start(self.functions[name]['va'], STOP, timeout=30_000_000, count=budget + 1)
        from unicorn.x86_const import UC_X86_REG_EIP
        if self.failure:
            raise RuntimeError(self.failure)
        if self.uc.reg_read(UC_X86_REG_EIP) != STOP or self.uc.reg_read(esp_reg) != sp + 4:
            raise RuntimeError('Original function did not return with caller cleanup')
        if any(self.uc.reg_read(reg) != value for reg, value in canaries.items()):
            raise RuntimeError('Original function changed nonvolatile registers')
        if bytes(self.uc.mem_read(TABLE, 20)) != struct.pack('<5I', 0, self.columns, self.rows, CELLS, INDICES):
            raise RuntimeError('Table mutated')
        if bytes(self.uc.mem_read(CELLS, len(self.cells))) != self.cells:
            raise RuntimeError('Cell/hash storage mutated')
        return self.uc.reg_read(eax_reg), self.steps

    def index_values(self):
        return list(struct.unpack('<' + 'h' * self.count, self.uc.mem_read(INDICES, len(self.original))))


def independent_first(indices, hashes, sought):
    begin, end = 0, len(indices)
    while begin < end:
        mid = (begin + end) // 2
        if hashes[indices[mid]] >= sought:
            end = mid
        else:
            begin = mid + 1
    return begin


def query_stage(guest, indices, oracle, lowercase, label):
    hashes = [cell['signed_hash'] for cell in oracle['stringtable_cells']]
    columns = oracle['stringtable_header']['column_count']
    records, misses, disagreements = [], 0, 0
    for cell in oracle['stringtable_cells']:
        index, sought = cell['index'], cell['signed_hash']
        first, steps = guest.invoke('first_hash_index', [sought & 0xffffffff], eax=TABLE)
        expected = independent_first(indices, hashes, sought)
        if first != expected:
            raise RuntimeError('Original FirstHashIndex disagrees with independently modeled CFG')
        if label == 'original' and first != oracle['stringtable_lookup_records'][index]['first_hash_index']:
            raise RuntimeError('Original FirstHashIndex differs from frozen source reference')
        row = -1
        pos = first
        while pos < len(indices) and hashes[indices[pos]] == sought:
            candidate = indices[pos]
            if candidate % columns == index % columns and lowercase[candidate] == lowercase[index]:
                row = candidate // columns
                break
            pos += 1
        valid_rows = sorted({candidate // columns for candidate in indices
                             if candidate % columns == index % columns
                             and hashes[candidate] == sought and lowercase[candidate] == lowercase[index]})
        linear_row = next(candidate // columns for candidate in indices
                          if candidate % columns == index % columns
                          and hashes[candidate] == sought and lowercase[candidate] == lowercase[index])
        misses += row < 0
        disagreements += row != linear_row
        records.append(dict(index=index, hash=sought, first_hash_index=first,
                            source_row_model=row, linear_row=linear_row,
                            valid_rows=valid_rows, instruction_count=steps))
    return dict(query_count=len(records), misses=misses,
                row_disagreements=disagreements, query_records=records)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, default=Path('D:/Games/Call of Duty - Black Ops/BlackOps.exe'))
    parser.add_argument('--dependencies', type=Path, default=ROOT.parent / 'BlackOps64/build/deps/water-oracle')
    parser.add_argument('--locator', type=Path, required=True)
    parser.add_argument('--lookup-results', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    if ROOT.resolve() not in output.parents or output.exists():
        raise RuntimeError('Output must be a fresh directory inside Android worktree')
    output.mkdir(parents=True)
    result = dict(schema='BlackOpsARM.TASK-0024.original-stringtable-reference.v1',
                  source_sha256=sha(Path(__file__).read_bytes()),
                  argv=[sys.executable, *sys.argv], status='running', assets={}, stubbed_services=[],
                  scope='original x86 instruction reference only; no executable process launch',
                  query_scope='actual FirstHashIndex; row matching independently modeled using genuine bytes',
                  budgets=dict(first_hash_instructions=4096, sort_instructions=2_000_000, per_call_seconds=30))
    try:
        sys.path.insert(0, str(args.dependencies.resolve()))
        import unicorn
        result['unicorn_version'] = unicorn.__version__
        result['dependency_path'] = str(args.dependencies.resolve())
        result['unicorn_module_sha256'] = sha(Path(unicorn.__file__).read_bytes())
        raw = args.exe.read_bytes()
        if sha(raw) != EXE_SHA:
            raise RuntimeError('Unrecognized original executable SHA256')
        locator_raw, lookup_raw = args.locator.read_bytes(), args.lookup_results.read_bytes()
        locator, lookup = json.loads(locator_raw), json.loads(lookup_raw)
        if locator['exe_sha256'] != EXE_SHA or not lookup['all_tests_completed']:
            raise RuntimeError('Unverified prerequisite evidence')
        functions = {name: locator['functions'][name] for name in FUNCTIONS}
        base, image_size, headers, sections = pe_layout(raw)
        for name, expected, extent in zip(FUNCTIONS, PINNED_SHAS, PINNED_RANGES):
            f = functions[name]
            if (f['va'], f['end_va_exclusive']) != extent or f['rva'] != f['va'] - base:
                raise RuntimeError(f'Function address/extent differs: {name}')
            section = next(s for s in sections if s['rva'] <= f['rva']
                           and f['rva'] + f['function_bytes'] <= s['rva'] + s['raw_size'])
            if f['file_offset'] != section['file_offset'] + f['rva'] - section['rva']:
                raise RuntimeError(f'Function PE source offset differs: {name}')
            data = raw[f['file_offset']:f['file_offset'] + f['function_bytes']]
            if sha(data) != expected or f['original_function_sha256'] != expected:
                raise RuntimeError(f'Function bytes differ: {name}')
        result.update(exe_path=str(args.exe.resolve()), exe_sha256=EXE_SHA,
                      locator_path=str(args.locator.resolve()), locator_sha256=sha(locator_raw),
                      lookup_results_path=str(args.lookup_results.resolve()), lookup_results_sha256=sha(lookup_raw),
                      functions=functions, comparator_global=GLOBAL,
                      first_hash_abi=dict(table='EAX', sought_hash='[entryESP+4]', result='EAX', cleanup='caller'),
                      sort_abi=dict(table='[entryESP+4]', cleanup='caller'))
        from stringtable_payload_oracle import _stream_prefix
        result['payload_oracle_source_sha256'] = sha((ROOT / 'tools/port/stringtable_payload_oracle.py').read_bytes())
        for name, oracle in lookup['oracles'].items():
            prefix = _stream_prefix(oracle['original_path'], oracle, oracle['prefix_bytes'])
            h = oracle['stringtable_header']
            actual_header = struct.unpack_from('<5I', prefix, oracle['stringtable_source_start'])
            if actual_header != tuple(h[field] for field in ('name_tag', 'column_count', 'row_count', 'values_tag', 'cell_index_tag')):
                raise RuntimeError('Frozen header metadata differs from genuine decoded header')
            if h['row_count'] * h['column_count'] != len(oracle['stringtable_cells']):
                raise RuntimeError('Header dimensions differ from genuine cell count')
            index_metadata = oracle['stringtable_indices']
            index_bytes = prefix[index_metadata['source_start']:index_metadata['source_end']]
            actual_indices = list(struct.unpack('<' + 'h' * index_metadata['count'], index_bytes))
            if (actual_indices != index_metadata['values'] or sha(index_bytes) != index_metadata['sha256']
                    or sorted(actual_indices) != list(range(len(actual_indices)))):
                raise RuntimeError('Frozen indices differ from genuine signed16 permutation')
            lowercase = []
            for cell in oracle['stringtable_cells']:
                if struct.unpack_from('<Ii', prefix, cell['source_descriptor_start']) != (cell['tag'], cell['signed_hash']):
                    raise RuntimeError('Frozen cell tag/hash differs from genuine descriptor')
                string = cell['string']
                if string['kind'] == 'offset':
                    data = lowercase[string['target_cell_index']]
                else:
                    value = bytes(prefix[string['source_start']:string['source_end'] - 1])
                    if any(c >= 128 for c in value):
                        raise RuntimeError('Non-ASCII genuine string outside validated query scope')
                    data = value.lower()
                if sha(data) != string['lowercase_sha256']:
                    raise RuntimeError('Lowercase genuine string digest differs')
                computed = 5381
                for byte in data:
                    computed = (byte + 33 * computed) & 0xffffffff
                if computed != cell['signed_hash'] & 0xffffffff:
                    raise RuntimeError('Stored genuine hash differs from exact ASCII Com_HashString recipe')
                lowercase.append(data)
            hashes = [cell['signed_hash'] for cell in oracle['stringtable_cells']]
            indices = oracle['stringtable_indices']['values']
            header = oracle['stringtable_header']
            guest = OriginalGuest(raw, functions, hashes, indices, header['column_count'], header['row_count'])
            guest.columns, guest.rows = header['column_count'], header['row_count']
            asset = result['assets'][name] = dict(raw_sha256=oracle['raw_sha256'], decoded_sha256=oracle['decoded_sha256'],
                       prefix_bytes=len(prefix), prefix_sha256=sha(prefix),
                       original_indices_sha256=sha(guest.original), original_indices=list(indices),
                       cell_count=len(indices), column_count=guest.columns, row_count=guest.rows)
            asset['original_queries'] = query_stage(guest, indices, oracle, lowercase, 'original')
            if asset['original_queries']['misses'] != oracle['stringtable_lookup_source_model_misses']:
                raise RuntimeError('Original miss count differs from source audit')
            if guest.index_values() != indices:
                raise RuntimeError('Original query changed indices')
            if bytes(guest.uc.mem_read(GLOBAL, 4)) != b'\0' * 4:
                raise RuntimeError('Comparator global not initially zero')
            _, sort_steps = guest.invoke('signed_sort_wrapper', [TABLE], budget=2_000_000)
            normalized = guest.index_values()
            if sorted(normalized) != sorted(indices):
                raise RuntimeError('Original qsort lost index permutation')
            keys = [(hashes[i], i % guest.columns) for i in normalized]
            if any(a > b for a, b in zip(keys, keys[1:])):
                raise RuntimeError('Original qsort failed signed hash/column ordering')
            if bytes(guest.uc.mem_read(GLOBAL, 4)) != b'\0' * 4:
                raise RuntimeError('Original helper did not clear comparator global')
            asset.update(normalized_indices=normalized, normalized_indices_sha256=sha(struct.pack('<' + 'h' * len(normalized), *normalized)),
                         original_sort_instruction_count=sort_steps, comparator_global_cleared=True,
                         max_equal_key_group=max(Counter(keys).values()),
                         duplicate_key_groups=sum(count > 1 for count in Counter(keys).values()))
            asset['normalized_queries'] = query_stage(guest, normalized, oracle, lowercase, 'normalized')
            if asset['normalized_queries']['misses'] or asset['normalized_queries']['row_disagreements']:
                raise RuntimeError('Normalized original query has missing/wrong matching row')
            asset.update(instruction_histogram=dict(guest.histogram), total_instruction_count=guest.total_steps,
                         original_guest_inputs_unchanged=True)
            if guest.index_values() != normalized:
                raise RuntimeError('Normalized query changed indices')
        result['status'] = 'passed'
    except Exception as error:
        result.update(status='failed', error=str(error), traceback=traceback.format_exc())
        (output / 'stderr.log').write_text(result['traceback'], encoding='utf-8')
    (output / 'results.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    summary = dict(status=result['status'], output=str(output), assets={name: {
        key: value for key, value in asset.items() if key in ('cell_count', 'original_sort_instruction_count',
        'max_equal_key_group', 'duplicate_key_groups', 'normalized_indices_sha256')}
        for name, asset in result['assets'].items()})
    (output / 'stdout.log').write_text(json.dumps(summary) + '\n', encoding='utf-8')
    print(json.dumps(summary))
    return 0 if result['status'] == 'passed' else 1


if __name__ == '__main__':
    raise SystemExit(main())
