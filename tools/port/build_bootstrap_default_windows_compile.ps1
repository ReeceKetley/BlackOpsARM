param([string]$VcVars='C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat',[Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$probeRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$probeOutput=[IO.Path]::GetFullPath((Join-Path $probeRoot $OutputDirectory))
if(!$probeOutput.StartsWith($probeRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Only Android worktree outputs allowed'}
if(Test-Path -LiteralPath $probeOutput){throw 'Use fresh output directory'}
New-Item -ItemType Directory -Path $probeOutput|Out-Null
$units=@('src/database/db_load.cpp','src/database/db_expand.cpp','src/gfx_d3d/r_material.cpp','src/database/db_registry.cpp','src/database/db_assetnames.cpp')
foreach($arch in @('x86','x64')){
 $output=Join-Path $probeOutput $arch
 New-Item -ItemType Directory -Path $output|Out-Null
 $defines=if($arch -eq 'x64'){'/DBO1_PORT_X64=1'}else{''}
 $hashes=@{}
 foreach($unit in ($units+@('src/gfx_d3d/r_material.h','src/database/db_material_graph_types.h','src/database/db_material_graph_load.h','src/gfx_d3d/r_material_shader_load.inl'))){$hashes[$unit]=(Get-FileHash -LiteralPath (Join-Path $probeRoot $unit) -Algorithm SHA256).Hash}
 $hashes|ConvertTo-Json -Depth 3|Out-File (Join-Path $output 'source-manifest.json') -Encoding utf8
 $sources=($units|ForEach-Object{'"'+(Join-Path $probeRoot $_)+'"'}) -join ' '
 $command='"'+$VcVars+'" '+$arch+' >nul && cl /nologo /W4 /std:c++20 /EHsc /MTd /O2 /Gy /D_DEBUG=1 '+$defines+' /I"'+(Join-Path $probeRoot 'src')+'" /I"'+$probeRoot+'" /I"C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Include" /c '+$sources+' /Fo"'+$output+'/"'
 $command|Out-File (Join-Path $output 'command.txt') -Encoding utf8
 & cmd /d /s /c $command 2>&1|Out-File (Join-Path $output 'compile.log') -Encoding utf8
 $compileExit=$LASTEXITCODE
 @{exit=$compileExit;completed=($compileExit -eq 0)}|ConvertTo-Json|Out-File (Join-Path $output 'build-result.json') -Encoding utf8
 foreach($unit in $hashes.Keys){if((Get-FileHash -LiteralPath (Join-Path $probeRoot $unit) -Algorithm SHA256).Hash -ne $hashes[$unit]){throw 'Source changed during normal compile'}}
 if($compileExit){Select-String -Path (Join-Path $output 'compile.log') -Pattern 'error ';throw "$arch normal compile failed"}
}
Write-Output "Normal Windows full load/expand/material/registry/asset-size compile: $probeOutput"
