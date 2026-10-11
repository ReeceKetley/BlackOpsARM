param([string]$VcVars = 'C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat')
$ErrorActionPreference = 'Stop'
$probeRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$probeOutput = Join-Path $probeRoot ('build-android-audit/host-fastfile-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmssfff'))
New-Item -ItemType Directory -Path $probeOutput | Out-Null
foreach ($arch in @('x86', 'x64')) {
    $archOutput = Join-Path $probeOutput $arch
    New-Item -ItemType Directory -Path $archOutput | Out-Null
    $defines = if ($arch -eq 'x64') { '/DBO1_PORT_X64=1' } else { '' }
    $command = '"' + $VcVars + '" ' + $arch + ' >nul && cl /nologo /W4 /WX /std:c++20 /EHsc /O2 ' + $defines + ' /I"' + (Join-Path $probeRoot 'src') + '" "' + (Join-Path $probeRoot 'tools/port/tests/fastfile_tests.cpp') + '" /Fe"' + (Join-Path $archOutput 'fastfile_tests.exe') + '" /Fo"' + $archOutput + '/"'
    $command | Out-File (Join-Path $archOutput 'fastfile-command.txt') -Encoding utf8
    & cmd /d /s /c $command 2>&1 | Out-File (Join-Path $archOutput 'fastfile-compile.log') -Encoding utf8
    if ($LASTEXITCODE) { Get-Content (Join-Path $archOutput 'fastfile-compile.log'); throw "$arch fastfile compile failed" }
    & (Join-Path $archOutput 'fastfile_tests.exe') 2>&1 | Tee-Object -FilePath (Join-Path $archOutput 'fastfile-test.log')
    if ($LASTEXITCODE) { throw "$arch fastfile test failed" }
}
Write-Output "Host fastfile evidence: $probeOutput"
