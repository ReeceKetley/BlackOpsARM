param(
    [string]$VcVars = 'C:\BuildTools\VC\Auxiliary\Build\vcvarsall.bat',
    [string]$DxSdk = 'C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)',
    [switch]$NegativeControl,
    [switch]$BuildGpu
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot/../..").Path
Set-Location -LiteralPath $root
$suffix = if ($NegativeControl) { '_baseline' } else { '' }
if ($NegativeControl) { python "$PSScriptRoot/check_material_shader.py" --baseline }
else { python "$PSScriptRoot/check_material_shader.py" }
if ($LASTEXITCODE -ne 0) { throw 'Shader fixture extraction failed' }
$results = @()
foreach ($arch in 'x86','x64') {
    $run = "build/run/shader57_tests/$arch"
    New-Item -ItemType Directory -Path $run -Force | Out-Null
    $sidecar = if ($arch -eq 'x86') { 'w32_reg3' } else { 'x64_thief_escape' }
    Copy-Item -LiteralPath "build/run/$sidecar/d3dx9_43.dll" -Destination "$run/d3dx9_43.dll" -Force
    $exe = "$run/test_material_shader$suffix.exe"
    $log = "build/run/test_material_shader${suffix}_$arch.log"
    $command = "`"$VcVars`" $arch >nul && cl /nologo /W3 /std:c++20 /EHsc /O2 /I`"$DxSdk/Include`" build/run/test_material_shader$suffix.cpp /Fe$exe /Fo$run/test_material_shader$suffix.obj /link /LIBPATH:`"$DxSdk/Lib/$arch`" d3dx9.lib"
    & cmd /d /s /c $command > $log 2>&1
    if ($LASTEXITCODE -ne 0) { Get-Content $log; throw "$arch shader fixture compilation failed" }
    foreach ($case in 'empty','cache','success','sw4','warning_output','compile_error','partial_failure','null_constants','null_shader','vertex_cache','pixel_cache','vertex_success','pixel_success','vertex_failure','pixel_failure') {
        "CASE $arch $case" | Tee-Object -FilePath $log -Append
        & ".\$exe" $case 2>&1 | Tee-Object -FilePath $log -Append
        $result = $LASTEXITCODE
        "EXIT $result" | Tee-Object -FilePath $log -Append
        $mustFail = $NegativeControl -and ($case -notin 'empty','cache') -and !($arch -eq 'x86' -and $case -eq 'pixel_cache')
        $expected = if ($mustFail) { $result -ne 0 } else { $result -eq 0 }
        $results += [ordered]@{architecture=$arch; case=$case; exitCode=$result; expectedFailure=$mustFail; expectedResultObserved=$expected}
        if (!$expected) { throw "$arch $case produced an unexpected result: $result" }
    }
    if ($BuildGpu) {
        if ($NegativeControl) { throw 'GPU build uses current source only' }
        $command = "`"$VcVars`" $arch >nul && cl /nologo /W3 /std:c++20 /EHsc /O2 /DSHADER_GPU /I`"$DxSdk/Include`" build/run/test_material_shader.cpp /Fe$run/test_material_shader_gpu.exe /Fo$run/test_material_shader_gpu.obj /link /LIBPATH:`"$DxSdk/Lib/$arch`" d3dx9.lib d3d9.lib user32.lib"
        & cmd /d /s /c $command > "build/run/test_material_shader_gpu_${arch}_compile.log" 2>&1
        if ($LASTEXITCODE -ne 0) { Get-Content "build/run/test_material_shader_gpu_${arch}_compile.log"; throw "$arch GPU fixture compilation failed" }
    }
}
$results | ConvertTo-Json | Set-Content "build/run/shader57${suffix}_cases.json"
