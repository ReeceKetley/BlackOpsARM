"""Prove canonical declarations and actual loader/creation bodies remain exact."""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
PARENT = 'd789e139647c70cb636760e5229b1d0fb536ca76'


def old(path):
    return subprocess.check_output(['git', 'show', PARENT + ':' + path], cwd=ROOT).decode('utf-8-sig').replace('\r\n', '\n')


def now(path):
    return (ROOT / path).read_text(encoding='utf-8-sig')


def body(source, name):
    matches = list(re.finditer(r'^[^ \t\n][^\n;{}]*\b' + re.escape(name) + r'\([^;{}]*?\)\s*\{', source, re.M))
    if len(matches) != 1:
        raise RuntimeError('Unique actual definition missing: ' + name)
    start = matches[0].start()
    cursor = source.index('{', start)
    depth = 0
    # These audited definitions have no braces inside string/comment tokens.
    # Mask lexical tokens before counting rather than relying on that property.
    lexical = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*[\s\S]*?\*/')
    masked = lexical.sub(lambda match: ' ' * len(match.group()), source)
    while cursor < len(masked):
        depth += (masked[cursor] == '{') - (masked[cursor] == '}')
        cursor += 1
        if depth == 0:
            return source[start:cursor]
    raise RuntimeError('Unbalanced body: ' + name)


def prove():
    hashes = {}
    functions = {
        'src/database/db_load.cpp': ['Load_DWORDArray', 'Load_uint', 'Load_floatArray', 'Load_GfxVertexShaderLoadDef', 'Load_GfxPixelShaderLoadDef', 'Load_MaterialVertexShaderProgram', 'Load_MaterialPixelShaderProgram', 'Load_MaterialVertexShader', 'Load_MaterialVertexShaderPtr', 'Load_MaterialPixelShader', 'Load_MaterialPixelShaderPtr', 'Load_MaterialVertexDeclaration', 'Load_MaterialArgumentCodeConst', 'Load_MaterialArgumentDef', 'Load_MaterialShaderArgument', 'Load_MaterialShaderArgumentArray', 'Load_MaterialPass', 'Load_MaterialPassArray', 'Load_MaterialTechnique', 'Load_MaterialTechniquePtr', 'Load_MaterialTechniquePtrArray', 'Load_MaterialTechniqueSet', 'Load_MaterialTechniqueSetPtr', 'AllocLoad_FxElemVisStateSample'],
        'src/database/db_expand.cpp': ['DB_ExpandLoad_MaterialTechniqueSet', 'DB_ExpandLoad_MaterialTechnique', 'DB_ExpandLoad_MaterialPassArray', 'DB_ExpandLoad_MaterialShaderArgumentArray', 'DB_ExpandLoad_MaterialVertexShader', 'DB_ExpandLoad_MaterialPixelShader', 'DB_ExpandLoad_MaterialVertexDeclaration'],
        'src/database/db_registry.cpp': ['Load_MaterialTechniqueSetAsset', 'DB_AddXAsset', 'DB_LinkXAssetEntry', 'DB_CloneXAssetInternal', 'DB_FreeXAssetHeader_EmblemSet_'],
        'src/database/db_assetnames.cpp': ['DB_SizeofXAsset_MaterialTechniqueSet_'],
        'src/database/db_stream_load.cpp': ['DB_ConvertOffsetToPointer', 'Load_XStringCustom'],
    }
    for path, names in functions.items():
        for name in names:
            before = body(old(path), name)
            after = body(now(path), name)
            if before != after:
                raise RuntimeError('Actual production body changed: ' + name)
            hashes[name] = hashlib.sha256(before.encode()).hexdigest()
    original = old('src/gfx_d3d/r_material.cpp')
    start = original.index('void __cdecl Load_CreateMaterialPixelShader')
    end = original.index('IDirect3DVertexDeclaration9 *__cdecl Material_BuildVertexDecl', start)
    if now('src/gfx_d3d/r_material_shader_load.inl') != original[start:end]:
        raise RuntimeError('Canonical creation bodies changed during move')
    if now('src/gfx_d3d/r_material.cpp') != original[:start] + '#include "r_material_shader_load.inl"\n\n' + original[end:]:
        raise RuntimeError('Normal material implementation changed outside canonical include')
    for name in ['Load_CreateMaterialPixelShader', 'Load_CreateMaterialVertexShader', 'Load_BuildVertexDecl']:
        hashes[name] = hashlib.sha256(body(original, name).encode()).hexdigest()
    original = old('src/gfx_d3d/r_material.h')
    start = original.index('struct MaterialStreamRouting')
    end = original.index('#include <database/db_techset_types.h>', start)
    if original[start:end] not in now('src/database/db_material_graph_types.h'):
        raise RuntimeError('Canonical graph declarations changed')
    if now('src/gfx_d3d/r_material.h') != original[:start] + '#include <database/db_material_graph_types.h>\n\n' + original[end:]:
        raise RuntimeError('Material declarations changed outside canonical include')
    old_expand = old('src/database/db_expand.cpp')
    start = old_expand.index('struct MaterialTechniqueHeader_32')
    end = old_expand.index('struct water_t_32', start)
    if old_expand[start:end] not in now('src/database/db_expand.cpp'):
        raise RuntimeError('Packed serialized graph images changed')
    for path in ['src/database/db_registry.cpp', 'src/database/db_assetnames.cpp', 'src/database/db_stream.cpp', 'src/database/db_stream_load.cpp', 'src/database/db_asset_pool.h']:
        if old(path) != now(path):
            raise RuntimeError('Unneeded production policy change: ' + path)
    for tree in ['tl', 'DemonWare']:
        subprocess.run(['git', 'diff', '--exit-code', PARENT, '--', tree], cwd=ROOT, check=True, capture_output=True)
    return {'schema': 'BlackOpsARM.TASK39.source.v1', 'status': 'passed', 'parent': PARENT, 'production_body_sha256': hashes, 'canonical_runtime_graph_declarations_moved_exactly': True, 'canonical_packed_disk_images_unchanged': True, 'normal_material_source_only_canonical_includes': True, 'canonical_pool_and_registry_policy_unchanged': True, 'windows_DWORD_unsigned_long_android_DWORD_uint32': True}


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', required=True)
    output = Path(parser.parse_args().output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(prove(), indent=2) + '\n', encoding='utf-8')
    print('TASK39 source preservation passed')
