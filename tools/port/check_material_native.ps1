param([string]$VcVars = 'C:\BuildTools\VC\Auxiliary\Build\vcvarsall.bat', [switch]$NegativeControl)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot\..\..").Path
Set-Location -LiteralPath $root
$suffix = if ($NegativeControl) { '_baseline' } else { '' }
if ($NegativeControl) { python "$PSScriptRoot\check_material_native.py" --baseline }
else { python "$PSScriptRoot\check_material_native.py" }
if ($LASTEXITCODE -ne 0) { throw 'Material extraction failed' }
$results = @()
foreach ($arch in 'x86','x64') {
    $exe = "build\run\test_material_native${suffix}_$arch.exe"
    $log = "build\run\test_material_native${suffix}_$arch.log"
    $command = "`"$VcVars`" $arch >nul && cl /nologo /W3 /std:c++20 /EHsc /O2 build\run\test_material_native$suffix.cpp /Fe$exe /Fobuild\run\test_material_native${suffix}_$arch.obj"
    & cmd /d /s /c $command > $log 2>&1
    if ($LASTEXITCODE -ne 0) { Get-Content $log; throw "$arch material compilation failed" }
    foreach ($case in 'technique','techset','statemap','literal','combine','parse','state') {
        "CASE $arch $case" | Tee-Object -FilePath $log -Append
        & ".\$exe" $case 2>&1 | Tee-Object -FilePath $log -Append
        $result = $LASTEXITCODE
        "EXIT $result" | Tee-Object -FilePath $log -Append
        $mustFail = $NegativeControl -and ($arch -eq 'x64' -or $case -in 'parse','state')
        $expected = if ($mustFail) { $result -ne 0 } else { $result -eq 0 }
        $results += [ordered]@{ architecture=$arch; case=$case; exitCode=$result; expectedFailure=$mustFail; expectedResultObserved=$expected }
        if (!$expected) { throw "$arch $case produced an unexpected result: $result" }
    }
}
$results | ConvertTo-Json | Set-Content "build\run\material56${suffix}_cases.json"
