param([string]$VcVars = 'C:\BuildTools\VC\Auxiliary\Build\vcvarsall.bat')
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot\..\..").Path
Set-Location -LiteralPath $root
python "$PSScriptRoot\check_weapon_fields.py"
if ($LASTEXITCODE -ne 0) { throw 'Weapon field test generation failed' }
foreach ($arch in 'x86','x64') {
    $defines = if ($arch -eq 'x64') { '/DBO1_PORT_X64' } else { '' }
    $exe = "build\run\test_weapon_fields_$arch.exe"
    $obj = "build\run\test_weapon_fields_$arch.obj"
    $log = "build\run\test_weapon_fields_$arch.log"
    $command = "`"$VcVars`" $arch >nul && cl /nologo /w /std:c++20 /EHsc /O2 /Gy /Gw /DBO1_MP /DWIN32 /DNDEBUG $defines /Isrc /I. /I`"C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Include`" build\run\test_weapon_fields.cpp /Fe$exe /Fo$obj /link /OPT:REF"
    & cmd /d /s /c $command > $log 2>&1
    if ($LASTEXITCODE -ne 0) { Get-Content $log; throw "$arch field test compilation failed" }
    & ".\$exe" | Tee-Object -FilePath $log -Append
    if ($LASTEXITCODE -ne 0) { throw "$arch field test failed" }
}
