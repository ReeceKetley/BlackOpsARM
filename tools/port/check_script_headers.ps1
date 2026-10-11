param(
    [string]$VcVars = 'C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat',
    [string]$PythonExe = 'python'
)
$ErrorActionPreference = 'Stop'
$probeRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$probeOutput = Join-Path $probeRoot ('build-android-audit/host-script-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmssfff'))
New-Item -ItemType Directory -Path $probeOutput | Out-Null
& $PythonExe (Join-Path $PSScriptRoot 'generate_math_oracle.py') (Join-Path $probeOutput 'legacy_axis_oracle.inc')
if ($LASTEXITCODE) { throw 'Windows maths oracle generation failed' }
& $PythonExe (Join-Path $PSScriptRoot 'generate_rng_probe.py') (Join-Path $probeOutput 'rng_probe.cpp')
if ($LASTEXITCODE) { throw 'Production RNG probe generation failed' }
foreach ($arch in @('x86', 'x64')) {
    $archOutput = Join-Path $probeOutput $arch
    New-Item -ItemType Directory -Path $archOutput | Out-Null
    $defines = if ($arch -eq 'x64') { '/DBO1_PORT_X64=1' } else { '' }
    $tests = @{
        script_headers = @('tools/port/tests/script_headers_tests.cpp', 'src/clientscript/cscr_instance.cpp', 'src/universal/narrow32.cpp', 'tools/port/tests/narrow32_windows_test_support.cpp')
        profile = @('tools/port/tests/profile_tests.cpp', 'src/universal/profile.cpp')
        math_axis = @('tools/port/tests/math_axis_tests.cpp', 'src/universal/com_math_anglevectors.cpp')
        rng = @($probeOutput + '/rng_probe.cpp')
        engine_rng = @('tools/port/tests/engine_rng_tests.cpp')
        script_lookup = @('tools/port/tests/script_lookup_tests.cpp', 'src/clientscript/cscr_readwrite.cpp')
    }
    foreach ($name in @('script_headers', 'profile', 'math_axis', 'rng', 'engine_rng', 'script_lookup')) {
        $sources = ($tests[$name] | ForEach-Object { '"' + $(if ([IO.Path]::IsPathRooted($_)) { $_ } else { Join-Path $probeRoot $_ }) + '"' }) -join ' '
        $mathDefines = if ($name -eq 'math_axis') { '/DBO1_MATH_WINDOWS_ORACLE=1' } else { '' }
        $command = '"' + $VcVars + '" ' + $arch + ' >nul && cl /nologo /W4 /std:c++20 /EHsc /MTd /O2 /Gy /fp:strict /D_DEBUG=1 ' + $defines + ' ' + $mathDefines + ' /I"' + (Join-Path $probeRoot 'src') + '" /I"' + (Join-Path $probeRoot 'tools/port/tests') + '" /I"' + $probeOutput + '" ' + $sources + ' /Fe"' + (Join-Path $archOutput ($name + '.exe')) + '" /Fo"' + $archOutput + '/" /link /OPT:REF'
        $compileLog = Join-Path $archOutput ($name + '-compile.log')
        $command | Out-File (Join-Path $archOutput ($name + '-command.txt')) -Encoding utf8
        & cmd /d /s /c $command 2>&1 | Out-File $compileLog -Encoding utf8
        if ($LASTEXITCODE) { Get-Content $compileLog; throw "$arch $name compile failed" }
        & (Join-Path $archOutput ($name + '.exe')) 2>&1 | Tee-Object -FilePath (Join-Path $archOutput ($name + '-test.log'))
        if ($LASTEXITCODE) { throw "$arch $name test failed" }
    }
}
Write-Output "Host script/profiler/maths evidence: $probeOutput"
