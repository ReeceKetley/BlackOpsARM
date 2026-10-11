"""Extract production material types, allocation/argument functions and state logic."""
import argparse
import subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser();parser.add_argument('--baseline',action='store_true');args=parser.parse_args()
def extract(text,sig):
    start=text.index(sig);pos=text.index('{',start);end=pos+1;depth=1
    while depth:
        if text[end:end+2]=='//':
            end=text.index('\n',end);continue
        if text[end:end+2]=='/*':
            end=text.index('*/',end)+2;continue
        if text[end] in '\"\'':
            quote=text[end];end+=1
            while text[end]!=quote:
                end+=2 if text[end]=='\\' else 1
            end+=1;continue
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[start:end]
source=(subprocess.check_output(['git','show','1fca062:src/gfx_d3d/r_material_load_obj.cpp'],cwd=root) if args.baseline
        else (root/'src/gfx_d3d/r_material_load_obj.cpp').read_bytes()).decode('latin1')
types=[]
for file,signatures in [
 ('r_gfx.h',['struct GfxDrawSurfFields ', 'union GfxDrawSurf ']),
 ('r_material.h',['struct __declspec(align(8)) MaterialInfo ', 'struct MaterialStreamRouting ',
 'struct MaterialVertexStreamRouting ', 'struct MaterialVertexDeclaration ',
 'struct __declspec(align(4)) GfxVertexShaderLoadDef ', 'struct MaterialVertexShaderProgram ', 'struct MaterialVertexShader ',
 'struct __declspec(align(4)) GfxPixelShaderLoadDef ', 'struct MaterialPixelShaderProgram ', 'struct MaterialPixelShader ',
 'struct MaterialArgumentCodeConst ', 'union MaterialArgumentDef ', 'struct MaterialShaderArgument ', 'struct MaterialPass ',
 'struct MaterialTechnique ', 'struct MaterialTechniqueSet ', 'struct GfxStateBits ', 'struct Material //']),
 ('r_material_load_obj.h',['enum MaterialUpdateFrequency ', 'enum ShaderParamType ', 'enum MaterialShaderType ',
 'struct MaterialStateMapRule ', 'struct MaterialStateMapRuleSet ', 'struct MaterialStateMap ', 'struct ShaderVaryingDef ',
 'struct __declspec(align(4)) ShaderUniformDef ', 'struct ShaderParameterSet ', 'struct __declspec(align(4)) ShaderIndexRange ',
 'struct ShaderArgumentSource ', 'struct ShaderArgumentDest '])]:
    text=(root/'src/gfx_d3d'/file).read_bytes().decode('latin1')
    for sig in signatures:types.append(extract(text,sig)+';')
state=(root/'src/gfx_d3d/r_state.h').read_bytes().decode('latin1')
for sig in ['static const MaterialUpdateFrequency s_codeSamplerUpdateFreq[','static const MaterialUpdateFrequency s_codeConstUpdateFreq[']:
    types.append(extract(state,sig)+';')
for sig in ['unsigned int g_customSamplerSrc[','unsigned int g_customSamplerDest[']:
    types.append(extract(source,sig)+';')
sigs=['MaterialTechniqueSet *__cdecl Material_LoadTechniqueSet(', 'MaterialTechnique *__cdecl Material_LoadTechnique(',
      'char __cdecl Material_LoadPass(', 'MaterialStateMap *__cdecl Material_LoadStateMap(',
      'char __cdecl Material_ParseShaderArguments(', 'char __cdecl Material_SetShaderArguments(',
      'int __cdecl Material_CompareShaderArgumentsForCombining(', 'unsigned int __cdecl Material_CombineShaderArguments(',
      'char __cdecl Material_AttemptCombineShaderArguments(', 'char __cdecl Material_AddShaderArgumentFromLiteral(',
      'char __cdecl MaterialAddShaderArgument(', 'int __cdecl Material_CompareShaderArgumentsForRuntime(',
      'int __cdecl Material_GetArgUpdateFrequency(', 'unsigned __int8 __cdecl Material_CountArgsWithUpdateFrequency(',
      'void __cdecl Material_BuildStateBitsTable(', 'void __cdecl Material_RemapStateBits(',
      'void __cdecl Material_ApplyStateBitsRemapRuleSet(', 'unsigned __int8 __cdecl Material_AddStateBitsArrayToTable(',
      'void __cdecl Material_SetStateBits(']
functions=[extract(source,sig) for sig in sigs]
decls='\n'.join(fn[:fn.index('{')].strip()+';' for fn in functions)
test=(root/'tools/port/test_material_native.cpp').read_text().replace('// PRODUCTION_TYPES','\n'.join(types)).replace('// PRODUCTION_DECLARATIONS',decls).replace('// PRODUCTION_FUNCTIONS','\n'.join(functions))
suffix='_baseline' if args.baseline else ''
path=root/f'build/run/test_material_native{suffix}.cpp';path.write_text(test)
print(path)
