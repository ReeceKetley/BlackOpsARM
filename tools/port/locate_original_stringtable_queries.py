"""Read-only, SHA-pinned original x86 StringTable instruction locator.

This tool never launches or imports the game. Original bytes stay in memory;
only address/digest metadata and bounded LLVM disassembly go to fresh ignored
Android scratch. Byte-pattern xrefs remain candidates until LLVM decodes them.
"""
import sys
sys.dont_write_bytecode = True

import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[2]
EXE_SHA256 = '2ba4a04e81de2b60582da0e4489efde21e6cb07904a223bae633b6ccaebbf759'
DEFAULT_OBJDUMP = Path('D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin/llvm-objdump.exe')


def digest(data):
    return hashlib.sha256(data).hexdigest()


class PE:
    def __init__(self, raw):
        self.raw = raw
        if raw[:2] != b'MZ':
            raise RuntimeError('Missing DOS signature')
        pe = struct.unpack_from('<I', raw, 0x3c)[0]
        if raw[pe:pe + 4] != b'PE\0\0':
            raise RuntimeError('Missing PE signature')
        machine, count = struct.unpack_from('<HH', raw, pe + 4)
        optional_size = struct.unpack_from('<H', raw, pe + 20)[0]
        if machine != 0x14c or struct.unpack_from('<H', raw, pe + 24)[0] != 0x10b:
            raise RuntimeError('Expected original i386 PE32')
        self.image_base = struct.unpack_from('<I', raw, pe + 52)[0]
        self.sections = []
        for number in range(count):
            start = pe + 24 + optional_size + 40 * number
            name = raw[start:start + 8].split(b'\0')[0].decode('ascii')
            virtual_size, rva, raw_size, file_offset = struct.unpack_from('<IIII', raw, start + 8)
            self.sections.append({'name': name, 'virtual_size': virtual_size, 'rva': rva,
                                  'va': self.image_base + rva, 'raw_size': raw_size,
                                  'file_offset': file_offset})
        self.text = next(section for section in self.sections if section['name'] == '.text')
        self.rdata = next(section for section in self.sections if section['name'] == '.rdata')
        if (self.image_base != 0x400000 or self.text['va'] != 0x401000
                or self.text['virtual_size'] != 0x5a1a10 or self.rdata['va'] != 0x9a3000):
            raise RuntimeError('Pinned original section layout differs')

    def at_offset(self, offset):
        for section in self.sections:
            if section['file_offset'] <= offset < section['file_offset'] + section['raw_size']:
                va = section['va'] + offset - section['file_offset']
                return {'va': va, 'va_hex': hex(va), 'rva': va - self.image_base,
                        'file_offset': offset, 'section': section['name']}
        raise RuntimeError('File offset outside mapped raw section')

    def at_va(self, va):
        for section in self.sections:
            if section['va'] <= va < section['va'] + section['raw_size']:
                return self.at_offset(section['file_offset'] + va - section['va'])
        raise RuntimeError('VA outside file-backed section')

    def slice(self, start, end):
        a, b = self.at_va(start), self.at_va(end - 1)
        if a['section'] != b['section']:
            raise RuntimeError('Cross-section function extent')
        return self.raw[a['file_offset']:b['file_offset'] + 1]

    def immediate_refs(self, target, text_only=False):
        needle = struct.pack('<I', target)
        start = self.text['file_offset'] if text_only else 0
        end = start + self.text['virtual_size'] if text_only else len(self.raw)
        records = []
        while (offset := self.raw.find(needle, start, end)) >= 0:
            records.append(self.at_offset(offset))
            start = offset + 1
        return records

    def branch_refs(self, target):
        first = self.text['file_offset']
        text = self.raw[first:first + self.text['virtual_size']]
        records = []
        for opcode, kind in [(0xe8, 'call'), (0xe9, 'jmp')]:
            start = 0
            while (offset := text.find(bytes([opcode]), start)) >= 0:
                start = offset + 1
                if offset + 5 > len(text):
                    continue
                va = self.text['va'] + offset
                if va + 5 + struct.unpack_from('<i', text, offset + 1)[0] == target:
                    record = self.at_va(va)
                    record.update({'kind': kind, 'target_va': target,
                                   'classification': 'byte-pattern candidate until LLVM boundary verification'})
                    records.append(record)
        return sorted(records, key=lambda record: record['va'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, default=Path('D:/Games/Call of Duty - Black Ops/BlackOps.exe'))
    parser.add_argument('--objdump', type=Path, default=DEFAULT_OBJDUMP)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    allowed = ROOT / 'build-android-audit'
    if not output.is_relative_to(allowed) or output.exists():
        raise RuntimeError('Use a fresh ignored build-android-audit output directory inside BlackOpsARM')
    raw = args.exe.read_bytes()
    if digest(raw) != EXE_SHA256:
        raise RuntimeError('Unsupported original executable SHA256; no outputs created')
    pe = PE(raw)
    output.mkdir(parents=True)
    commands = []
    report = {'schema': 'BlackOpsARM.TASK-0024.original-query-locator.v1',
              'exe_path': str(args.exe.resolve()), 'exe_sha256': digest(raw), 'exe_bytes': len(raw),
              'locator_source_sha256': digest(Path(__file__).read_bytes()),
              'objdump_path': str(args.objdump.resolve()), 'objdump_sha256': digest(args.objdump.read_bytes()),
              'image_base': pe.image_base, 'sections': pe.sections,
              'game_launched': False, 'game_imported': False, 'original_files_written': False,
              'full_original_bytes_copied': False, 'dependency_imports': 'standard library only',
              'limitations': ['No live original game or full original loading pipeline executed.',
                              'Immediate VA and raw E8/E9 searches alone are not instruction xrefs.',
                              'Function name assignments are semantic identifications, not original symbols.'],
              'strings': [], 'functions': {}, 'instruction_refs': {}}

    def write_json(name, value):
        (output / name).write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8')

    def disassemble(label, start, end):
        argv = [str(args.objdump), '-d', '--no-show-raw-insn', '--x86-asm-syntax=intel',
                f'--start-address={hex(start)}', f'--stop-address={hex(end)}', str(args.exe)]
        completed = subprocess.run(argv, capture_output=True, text=True, encoding='utf-8', errors='replace', timeout=45)
        (output / (label + '.stdout.log')).write_text(completed.stdout, encoding='utf-8')
        (output / (label + '.stderr.log')).write_text(completed.stderr, encoding='utf-8')
        commands.append({'label': label, 'argv': argv, 'exit': completed.returncode,
                         'start_va': start, 'end_va_exclusive': end,
                         'original_range_sha256': digest(pe.slice(start, end)),
                         'stdout_sha256': digest(completed.stdout.encode('utf-8'))})
        write_json('commands.json', commands)
        if completed.returncode:
            raise RuntimeError('LLVM disassembly failed; logs retained: ' + label)
        return completed.stdout

    try:
        for label, needle in [('column_error', b'Unable to compare against column number'),
                              ('row_error', b'Unable to compare against row number'),
                              ('loadobj_name', b'StringTable_LoadObj'), ('cellcompare_name', b'CellCompare'),
                              ('source_filename', b'com_stringtable_obj.cpp')]:
            start, found = 0, []
            while (offset := raw.find(needle, start)) >= 0:
                record = pe.at_offset(offset)
                terminator = raw.find(b'\0', offset, offset + 256)
                record.update({'ascii_prefix': needle.decode('ascii'),
                               'terminated_string_sha256': digest(raw[offset:terminator + 1]) if terminator >= 0 else None,
                               'immediate_va_candidates': pe.immediate_refs(record['va'], text_only=True)})
                for number, ref in enumerate(record['immediate_va_candidates']):
                    disassemble(f'{label}-{len(found)}-xref-{number}', max(pe.text['va'], ref['va'] - 0x180), ref['va'] + 0x80)
                found.append(record)
                start = offset + 1
            report['strings'].append({'label': label, 'present': bool(found), 'occurrences': found})

        # These discovered extents are valid ONLY for the SHA-pinned executable.
        # Each is checked against decoded instructions and an original range hash.
        functions = [
            ('first_hash_index', 0x8604b0, 0x8604f4, 'semantically confirmed signed lower-bound leaf',
             ['mov\tecx, dword ptr [eax + 0x8]', 'imul\tecx, dword ptr [eax + 0x4]',
              'movsx\tedx, word ptr [edi + 2*eax]', 'cmp\tdword ptr [ebx + 8*edx + 0x4], ebp',
              'jge\t0x8604e7', 'mov\teax, esi']),
            ('row_query', 0x6207d0, 0x620883, 'semantically confirmed simple row-query leaf',
             ['push\t0x9f2550', 'call\t0x8604b0', 'movsx\tebx, word ptr [ecx + 2*edi]',
              'call\t0x552bf0', 'idiv\tdword ptr [esi + 0x4]']),
            ('signed_normalized_comparator', 0x860500, 0x860550, 'confirmed signed normalized hash comparator',
             ['mov\tebx, dword ptr [0x26050d8]', 'cmp\tedx, ecx', 'setge\tal',
              'lea\teax, [eax + eax - 0x1]', 'sub\teax, edi']),
            ('wrapping_subtraction_comparator', 0x860560, 0x86059f, 'confirmed wrapping hash-subtraction comparator',
             ['mov\tedi, dword ptr [0x26050dc]', 'sub\teax, dword ptr [ecx + 8*esi + 0x4]',
              'jne\t0x86059c', 'sub\tedi, edx']),
            ('signed_sort_wrapper', 0x5c03f0, 0x5c041f, 'confirmed signed comparator qsort wrapper',
             ['push\t0x860500', 'push\t0x2', 'call\t0x967090',
              'mov\tdword ptr [0x26050d8], eax', 'mov\tdword ptr [0x26050d8], 0x0']),
            ('registration_sort_wrapper', 0x53edd0, 0x53edf1, 'confirmed type37 registration followed by signed-sort tail call',
             ['mov\teax, 0x25', 'call\t0x7a3330', 'mov\tdword ptr [esi], eax',
              'mov\tdword ptr [esp + 0x4], eax', 'jmp\t0x5c03f0']),
            ('inline_stringtable_pointer_loader', 0x604f50, 0x604fe9, 'confirmed inline pointer loader calls header then registration/sort',
             ['push\t0x4', 'cmp\tesi, -0x1', 'cmp\tesi, -0x2', 'mov\tdword ptr [0xc8188c], ecx',
              'call\t0x5e53e0', 'call\t0x53edd0', 'mov\tdword ptr [esi], ecx', 'jmp\t0x460ed0']),
            ('stringtable_header_body_loader', 0x5e53e0, 0x5e548e, 'confirmed20-byte header and name/cell/index child loader',
             ['push\t0x14', 'call\t0x5a3780', 'mov\tedx, dword ptr [0xc8188c]',
              'call\t0x5d8f00', 'cmp\tdword ptr [eax + 0xc], 0x0', 'call\t0x650ae0',
              'cmp\tdword ptr [edx + 0x10], 0x0', 'add\tedx, edx', 'jmp\t0x460ed0']),
            ('original_qsort', 0x967090, 0x9672dd, 'confirmed original comparison-callback sort service',
             ['mov\tebx, dword ptr [ebp + 0x8]', 'mov\tesi, dword ptr [ebp + 0xc]',
              'mov\tedi, dword ptr [ebp + 0x10]', 'call\tdword ptr [ebp + 0x14]',
              'call\t0x967000', 'call\t0x966fd0', 'jmp\t0x967118']),
            ('original_qsort_small_sort', 0x967000, 0x967083, 'confirmed original small-range sort helper',
             ['mov\tedi, eax', 'call\tdword ptr [ebp + 0x10]', 'mov\tbyte ptr [ecx + eax], bl',
              'sub\tedi, ecx', 'ja\t0x967020']),
            ('original_qsort_swap', 0x966fd0, 0x966ff2, 'confirmed original byte-swap helper',
             ['mov\tesi, edx', 'cmp\teax, ecx', 'mov\tbyte ptr [eax], bl', 'mov\tbyte ptr [ecx], dl']),
        ]
        for name, start, end, conclusion, required in functions:
            text = disassemble(name, start, end)
            if any(fragment not in text for fragment in required):
                raise RuntimeError('Pinned semantic instruction check failed: ' + name)
            info = pe.at_va(start)
            info.update({'end_va_exclusive': end, 'function_bytes': end - start,
                         'original_function_sha256': digest(pe.slice(start, end)),
                         'classification': conclusion, 'required_disassembly_fragments': required})
            if name == 'first_hash_index':
                info['calling_convention'] = {'table': 'EAX at entry', 'hash': '[entry ESP+4]',
                                               'return': 'EAX', 'stack_cleanup': 'caller',
                                               'callee_preserved': ['EBP', 'ESI', 'EBX', 'EDI']}
                info['branch_semantics'] = 'signed CMP/JGE; equal hashes move end to midpoint; no hash subtraction'
            elif name == 'signed_normalized_comparator':
                info['branch_semantics'] = {
                    'unequal_hashes': 'signed CMP EDX,ECX; SETGE AL; return2*AL-1 (no hash subtraction/overflow)',
                    'equal_hashes': 'signed16 cell indices; IDIV columnCount; return indexA%columns-indexB%columns',
                    'stable_row_tie': 'No row comparison when hash and column agree; qsort stability not promised',
                    'global_table_va': 0x26050d8}
            elif name == 'wrapping_subtraction_comparator':
                info['branch_semantics'] = {
                    'unequal_hashes': 'SUB hashA-hashB in32-bit EAX; return that modulo32-bit result',
                    'equal_hashes': 'signed16 cell indices; IDIV columnCount; return indexA%columns-indexB%columns',
                    'global_table_va': 0x26050dc}
            elif name == 'original_qsort':
                info['calling_convention'] = {'base': '[entry ESP+4]', 'count': '[entry ESP+8]',
                                               'element_bytes': '[entry ESP+12]', 'comparator': '[entry ESP+16]',
                                               'stack_cleanup': 'caller'}
                info['valid_input_helper_ranges'] = ['original_qsort_small_sort', 'original_qsort_swap']
                info['invalid_input_external_services'] = [0x96b04e, 0x96c0d4]
                info['limitations'] = 'Caller proves nonnull storage, positive stride, valid comparator; invalid-input CRT services not executed by locator.'
            info['decoded_direct_calls'] = [
                {'call_va': int(match.group(1), 16), 'target_va': int(match.group(2), 16)}
                for match in re.finditer(r'(?m)^\s*([0-9a-f]+):\s+call\s+0x([0-9a-f]+)\b', text)]
            info['decoded_direct_jumps'] = [
                {'jmp_va': int(match.group(1), 16), 'target_va': int(match.group(2), 16)}
                for match in re.finditer(r'(?m)^\s*([0-9a-f]+):\s+jmp\s+0x([0-9a-f]+)\b', text)]
            report['functions'][name] = info

        for label, target in [('first_hash_index', 0x8604b0), ('signed_comparator', 0x860500),
                              ('wrapping_comparator', 0x860560), ('signed_sort_wrapper', 0x5c03f0),
                              ('registration_sort_wrapper', 0x53edd0), ('signed_sort_global', 0x26050d8),
                              ('wrapping_sort_global', 0x26050dc), ('inline_pointer_loader', 0x604f50),
                              ('header_body_loader', 0x5e53e0)]:
            branch_refs = pe.branch_refs(target)
            for number, ref in enumerate(branch_refs):
                text = disassemble(f'{label}-branch-{number}', ref['va'], ref['va'] + 5)
                pattern = rf'(?m)^\s*{ref["va"]:x}:\s+{ref["kind"]}\s+0x{target:x}\b'
                ref['decoded_instruction_matches'] = bool(re.search(pattern, text))
                disassemble(f'{label}-branch-context-{number}', ref['va'] - 0x140, ref['va'] + 0x80)
            immediates = pe.immediate_refs(target)
            for number, ref in enumerate(immediates):
                if ref['section'] == '.text':
                    disassemble(f'{label}-immediate-context-{number}', ref['va'] - 0x100, ref['va'] + 0x80)
            report['instruction_refs'][label] = {'target_va': target, 'relative_branch_candidates': branch_refs,
                                                'immediate_va_candidates': immediates}
        report['loading_chain'] = {
            'verified_instruction_sequence': [
                {'caller': 0x604f50, 'call_va': 0x604fc2, 'callee': 0x5e53e0, 'role': 'read header/name/cells/original indices'},
                {'caller': 0x604f50, 'call_va': 0x604fce, 'callee': 0x53edd0, 'role': 'registration then signed-sort wrapper'},
                {'caller': 0x53edd0, 'call_va': 0x53eddd, 'callee': 0x7a3330, 'type_eax': 0x25,
                 'role': 'asset-add service candidate; returned pointer stored into caller slot'},
                {'caller': 0x53edd0, 'jmp_va': 0x53edec, 'callee': 0x5c03f0, 'role': 'tail-call sort on returned pointer'},
                {'caller': 0x5c03f0, 'call_va': 0x5c040c, 'callee': 0x967090,
                 'comparator': 0x860500, 'element_bytes': 2, 'count': 'columnCount*rowCount',
                 'role': 'qsort service candidate with signed-normalized comparator'},
            ],
            'original_global_lifetimes': {'current_pointer_slot': 0xc81620, 'current_header': 0xc8188c,
                                        'signed_sort_table': 0x26050d8, 'wrapping_sort_table': 0x26050dc},
            'ownership': 'Header/children load first; register returns possibly replaced pointer; signed sort operates on that returned header; -2 inserted slot updated afterward.',
            'scope_limit': 'Instruction mapping only; no original asset registry/async/decode services executed.'}
        report['reconstruction_source_provenance'] = [
            {'path': name, 'sha256': digest((ROOT / name).read_bytes())}
            for name in ['src/universal/com_stringtable.cpp', 'src/universal/com_stringtable_obj.cpp',
                         'src/database/db_registry.cpp', 'src/database/db_load.cpp', 'src/database/database.h']]
        report['findings'] = [
            'Original FirstHashIndex uses the reconstructed signed CMP/JGE lower bound.',
            'Original simple row-query leaf calls that signed lower bound and performs hash equality, column modulus and I_stricmp.',
            'Original type37 registration wrapper tail-calls a qsort using a signed normalized comparator.',
            'The -1/-2 inline StringTable pointer-loader explicitly invokes body loading then the type37 registration/sort wrapper before inserted-pointer completion.',
            'Reconstructed Load_StringTableAsset currently registers the asset but lacks the original signed-sort tail step; no reconstruction changes made by locator.',
            'The separate wrapping-subtraction comparator is passed by a larger parsing-context routine; full loader identity still requires caller review.',
            'Current raw loaded retail indices must not be re-sorted or query algorithms changed without integrating the original ownership/registration boundary.',
            'An earlier partial adjacent-window report wrongly assigned subtraction to0x860500; full verified windows distinguish0x860500 signed normalized and0x860560 wrapping subtraction.',
        ]
        report['all_locator_checks_completed'] = True
        write_json('results.json', report)
        print('PASS original query locator; candidate and confirmed instruction evidence:', output)
    except Exception as error:
        write_json('failure.json', {'error_type': type(error).__name__, 'error': str(error)})
        write_json('results.json', report)
        raise


if __name__ == '__main__':
    main()
