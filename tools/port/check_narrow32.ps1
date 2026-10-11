param(
    [string]$VcVars = 'C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat',
    [string]$PythonExe = 'python'
)
$ErrorActionPreference = 'Stop'
$probeRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$probeOutput = Join-Path $probeRoot ('build-android-audit/host-narrow-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmssfff'))
New-Item -ItemType Directory -Path $probeOutput | Out-Null
& $PythonExe (Join-Path $PSScriptRoot 'generate_script_slot_probe.py') (Join-Path $probeOutput 'script_slot_probe.cpp')
if ($LASTEXITCODE) { throw 'Script declaration extraction failed' }
& $PythonExe (Join-Path $PSScriptRoot 'audit_native_width_flags.py') (Join-Path $probeOutput 'flag_guard_probe.cpp')
if ($LASTEXITCODE) { throw 'Flag guard probe generation failed' }
foreach ($arch in @('x86', 'x64')) {
    $archOutput = Join-Path $probeOutput $arch
    New-Item -ItemType Directory -Path $archOutput | Out-Null
    $defines = if ($arch -eq 'x64') { '/DBO1_PORT_X64=1' } else { '' }
    $command = '"' + $VcVars + '" ' + $arch + ' >nul && cl /nologo /W4 /WX /std:c++20 /EHsc /O2 ' + $defines + ' /I"' + (Join-Path $probeRoot 'src') + '" "' + (Join-Path $probeRoot 'tools/port/tests/narrow32_tests.cpp') + '" "' + (Join-Path $probeRoot 'src/universal/narrow32.cpp') + '" "' + (Join-Path $probeRoot 'tools/port/tests/narrow32_windows_test_support.cpp') + '" /Fe"' + (Join-Path $archOutput 'narrow32_tests.exe') + '" /Fo"' + $archOutput + '\\"'
    & cmd /d /s /c $command 2>&1 | Out-File (Join-Path $archOutput 'compile.log') -Encoding utf8
    if ($LASTEXITCODE) { Get-Content (Join-Path $archOutput 'compile.log'); throw "$arch compile failed" }
    & (Join-Path $archOutput 'narrow32_tests.exe') 2>&1 | Tee-Object -FilePath (Join-Path $archOutput 'test.log')
    if ($LASTEXITCODE) { throw "$arch narrow probe failed" }
    $slotCommand = '"' + $VcVars + '" ' + $arch + ' >nul && cl /nologo /W4 /WX /std:c++20 /EHsc /O2 ' + $defines + ' /I"' + (Join-Path $probeRoot 'src') + '" "' + (Join-Path $probeOutput 'script_slot_probe.cpp') + '" "' + (Join-Path $probeRoot 'src/universal/narrow32.cpp') + '" "' + (Join-Path $probeRoot 'tools/port/tests/narrow32_windows_test_support.cpp') + '" /Fe"' + (Join-Path $archOutput 'script_slot_probe.exe') + '" /Fo"' + $archOutput + '\\"'
    & cmd /d /s /c $slotCommand 2>&1 | Out-File (Join-Path $archOutput 'slot-compile.log') -Encoding utf8
    if ($LASTEXITCODE) { Get-Content (Join-Path $archOutput 'slot-compile.log'); throw "$arch script slot compile failed" }
    & (Join-Path $archOutput 'script_slot_probe.exe') 2>&1 | Tee-Object -FilePath (Join-Path $archOutput 'slot-test.log')
    if ($LASTEXITCODE) { throw "$arch script slot probe failed" }
    $guardCommand = '"' + $VcVars + '" ' + $arch + ' >nul && cl /nologo /W4 /WX /std:c++20 ' + $defines + ' /I"' + (Join-Path $probeRoot 'src') + '" /c "' + (Join-Path $probeOutput 'flag_guard_probe.cpp') + '" /Fo"' + (Join-Path $archOutput 'flag_guard_probe.obj') + '"'
    & cmd /d /s /c $guardCommand 2>&1 | Out-File (Join-Path $archOutput 'guard-compile.log') -Encoding utf8
    if ($LASTEXITCODE) { Get-Content (Join-Path $archOutput 'guard-compile.log'); throw "$arch guard parity failed" }
    if ($arch -eq 'x64') {
        $guardOffCommand = $guardCommand.Replace('/DBO1_PORT_X64=1', '').Replace('flag_guard_probe.obj', 'flag_guard_off.obj')
        & cmd /d /s /c $guardOffCommand 2>&1 | Out-File (Join-Path $archOutput 'guard-off-compile.log') -Encoding utf8
        if ($LASTEXITCODE) { Get-Content (Join-Path $archOutput 'guard-off-compile.log'); throw 'x64 flag-off guard parity failed' }
    }
}
Write-Output "Host probe evidence: $probeOutput"
