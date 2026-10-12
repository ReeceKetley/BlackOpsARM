"""Prove canonical selected loader bodies and normal Windows read/dispatch paths."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess
import verify_bootstrap_default_source as extraction

ROOT = Path(__file__).resolve().parents[2]
PARENT = 'bf199b7'


def normal_selection(body):
    # Select only the new native-load condition. Preserve every other directive
    # verbatim so unrelated ABI/Windows branches must still match the baseline.
    out, stack, active = [], [], True
    for line in body.splitlines(keepends=True):
        directive = line.strip()
        if directive.startswith(('#if ', '#ifdef ', '#ifndef ')):
            native = 'BO1_NATIVE_ZONE_LOAD' in directive
            if native:
                if directive == '#if defined(BO1_NATIVE_ZONE_LOAD)':
                    branch = False
                elif directive == '#if !defined(BO1_NATIVE_ZONE_LOAD)':
                    branch = True
                else:
                    raise RuntimeError('Unexpected native body condition: ' + directive)
            else:
                branch = True
                if active:
                    out.append(line)
            stack.append((native, branch, active))
            active = active and branch
        elif directive == '#else':
            native, branch, parent = stack[-1]
            if native:
                branch = not branch
                stack[-1] = (native, branch, parent)
                active = parent and branch
            elif active:
                out.append(line)
        elif directive.startswith('#elif'):
            if stack[-1][0]:
                raise RuntimeError('Unexpected native elif')
            if active:
                out.append(line)
        elif directive.startswith('#endif'):
            native, _, parent = stack.pop()
            if not native and active:
                out.append(line)
            active = parent
        elif active:
            out.append(line)
    if stack:
        raise RuntimeError('Unbalanced body preprocessor')
    return ''.join(out)


def prove():
    proofs = {}
    groups = {
        'src/database/db_load.cpp': ['AllocLoad_XBlendInfo', 'AllocLoad_FxElemVisStateSample',
            'Load_RawFile', 'Load_RawFilePtr', 'Load_StringTableCell',
            'Load_StringTableCellArray', 'Load_StringTable', 'Load_StringTablePtr',
            'Load_XAsset', 'Load_XAssetHeader'],
        'src/database/db_registry.cpp': ['Load_RawFileAsset', 'Load_StringTableAsset',
            'DB_AddXAsset', 'DB_LinkXAssetEntry', 'DB_FreeXAssetEntry'],
        'src/database/db_file_load.cpp': ['DB_LoadXFileData', 'DB_LoadXFileDataNullTerminated'],
        'src/database/db_stream_load.cpp': ['DB_ConvertOffsetToPointer', 'Load_XStringCustom'],
        'src/universal/com_stringtable.cpp': ['StringTable_GetColumnValueForRow'],
    }
    for path, names in groups.items():
        before = subprocess.check_output(['git', 'show', PARENT + ':' + path], cwd=ROOT).decode('utf-8-sig').replace('\r\n', '\n')
        after = (ROOT / path).read_text(encoding='utf-8-sig')
        for name in names:
            original = extraction.body(before, name)
            current = extraction.body(after, name)
            if normal_selection(current) != original:
                raise RuntimeError('Canonical/normal body changed: ' + name)
            proofs[name] = {'original_sha256': hashlib.sha256(original.encode()).hexdigest(),
                            'normal_body_unchanged': True,
                            'native_selection_added': current != original}
    for path in ('src/platform/fastfile_inflate.cpp', 'src/platform/fastfile_inflate.h',
                 'src/database/db_stream.cpp',
                 'src/database/db_asset_entry_types.h', 'src/database/db_assetlist_types.h',
                 'src/universal/physicalmemory.cpp', 'src/universal/physicalmemory.h'):
        original = subprocess.check_output(['git', 'show', PARENT + ':' + path], cwd=ROOT).replace(b'\r\n', b'\n')
        current = (ROOT / path).read_bytes().replace(b'\r\n', b'\n')
        if current != original:
            raise RuntimeError('Unexpected policy/type/reader edit: ' + path)
    reader_delta = {}
    for path in ('src/platform/native_zone_file.cpp', 'src/platform/native_zone_file.h'):
        before = subprocess.check_output(['git', 'show', PARENT + ':' + path], cwd=ROOT).decode('utf-8-sig').replace('\r\n', '\n')
        after = (ROOT / path).read_text(encoding='utf-8-sig')
        if path.endswith('.cpp'):
            expected = before.replace('    if (!open_) return NativeZoneResult::Ok;\n',
                '    if (!open_) return NativeZoneResult::Ok;\n    if (childrenRetained_) return NativeZoneResult::BorrowedChildren;\n')
        else:
            expected = before.replace('AllocationFailed, StreamsAttached };', 'AllocationFailed, StreamsAttached, BorrowedChildren };')
            expected = expected.replace('private:\n', '''private:
    friend class NativeZoneLoad;
    // One production loader retains this owner until scoped removal completes.
    bool RetainChildren() { if (!open_ || childrenRetained_) return false; childrenRetained_=true; return true; }
    void ReleaseChildren() { childrenRetained_=false; }
''')
            expected = expected.replace('    bool open_{}, ownsAllocation_{};\n',
                '    bool open_{}, ownsAllocation_{};\n    bool childrenRetained_{};\n')
        if after != expected:
            raise RuntimeError('Reader differs beyond explicit child-lease lifetime change: ' + path)
        reader_delta[path] = {'before_sha256': hashlib.sha256(before.encode()).hexdigest(),
                              'after_sha256': hashlib.sha256(after.encode()).hexdigest()}
    return {'schema': 'BlackOpsARM.native-zone-load.source.v1', 'passed': True,
            'parent': PARENT, 'canonical_bodies': proofs,
            'reviewed_reader_child_lease_delta': reader_delta,
            'scope': 'Body/normal branch proof; normal builds and runtime independently required'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True)
    output = Path(parser.parse_args().output)
    if output.exists():
        raise SystemExit('Use fresh source proof path')
    result = prove()
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print('Canonical loader and normal Windows read/dispatch bodies preserved')
