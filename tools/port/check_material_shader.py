"""Extract production shader loading/compiler functions for real D3DX tests."""
import argparse
import subprocess
from pathlib import Path

repo = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--baseline', action='store_true')
args = parser.parse_args()

def extract(text, signature):
    start = text.index(signature)
    end = text.index('{', start) + 1
    depth = 1
    while depth:
        if text[end:end+2] == '//':
            end = text.index('\n', end)
            continue
        if text[end:end+2] == '/*':
            end = text.index('*/', end) + 2
            continue
        if text[end] in '\"\'':
            quote = text[end]
            end += 1
            while text[end] != quote:
                end += 2 if text[end] == '\\' else 1
            end += 1
            continue
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]

source = (subprocess.check_output(['git', 'show', '1fca062:src/gfx_d3d/r_material_load_obj.cpp'], cwd=repo)
          if args.baseline else (repo/'src/gfx_d3d/r_material_load_obj.cpp').read_bytes()).decode('latin1')
types = []
for name, signatures in [
    ('r_material.h', ['struct __declspec(align(4)) GfxVertexShaderLoadDef ', 'struct MaterialVertexShaderProgram ',
                        'struct MaterialVertexShader ', 'struct __declspec(align(4)) GfxPixelShaderLoadDef ',
                        'struct MaterialPixelShaderProgram ', 'struct MaterialPixelShader ']),
    ('r_material_load_obj.h', ['enum MaterialShaderType ', 'struct GfxAssembledShaderTextFile ', 'struct GfxAssembledShaderText '])]:
    text = (repo/'src/gfx_d3d'/name).read_bytes().decode('latin1')
    types += [extract(text, signature)+';' for signature in signatures]
functions = [extract(source, signature) for signature in [
    'MaterialVertexShader *__cdecl Material_LoadVertexShader(', 'ID3DXBuffer *__cdecl Material_CompileShader(',
    'MaterialPixelShader *__cdecl Material_LoadPixelShader(', 'void __cdecl Material_FileIncludeFileAndLineNumber(',
    'bool __cdecl Material_ParseLineNumber(']]
declarations = '\n'.join(fn[:fn.index('{')].strip()+';' for fn in functions)
fixture = (repo/'tools/port/test_material_shader.cpp').read_text()
fixture = fixture.replace('// PRODUCTION_TYPES', '\n'.join(types)).replace('// PRODUCTION_DECLARATIONS', declarations)
fixture = fixture.replace('// PRODUCTION_FUNCTIONS', '\n'.join(functions))
suffix = '_baseline' if args.baseline else ''
path = repo/f'build/run/test_material_shader{suffix}.cpp'
path.write_text(fixture)
print(path)
