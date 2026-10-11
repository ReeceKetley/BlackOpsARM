param([string]$VcVars = 'C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat')
$ErrorActionPreference = 'Stop'
$probeRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$probeOutput = Join-Path $probeRoot ('build-android-audit/host-zone-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmssfff'))
New-Item -ItemType Directory -Path $probeOutput | Out-Null
foreach ($arch in @('x86', 'x64')) {
    $archOutput = Join-Path $probeOutput $arch
    New-Item -ItemType Directory -Path $archOutput | Out-Null
    $defines = if ($arch -eq 'x64') { '/DBO1_PORT_X64=1' } else { '' }
    $files = @('tools/port/tests/zone_tests.cpp', 'tools/port/tests/zone_test_support.cpp',
        'tools/port/tests/hunk_test_support.cpp', 'tools/port/tests/hunk_test_memory_windows.cpp',
        'tools/port/tests/firstfit_windows_services.cpp', 'src/database/db_memory.cpp',
        'src/database/db_stream.cpp', 'src/universal/mem_userhunk.cpp', 'src/universal/mem_firstfit.cpp',
        'src/universal/narrow32.cpp', 'src/platform/android/zone_memory.cpp', 'src/platform/android/zone_heap.cpp')
    $sources = ($files | ForEach-Object { '"' + (Join-Path $probeRoot $_) + '"' }) -join ' '
    $command = '"' + $VcVars + '" ' + $arch + ' >nul && cl /nologo /W4 /std:c++20 /EHsc /MTd /O2 /Gy /D_DEBUG=1 ' + $defines +
        ' /DBO1_ZONE_MEMORY_HOST_TEST=1 /DBO1_ZONE_HEAP_HOST_TEST=1 /DBO1_ZONE_GATE=1 /DBO1_REAL_FIRSTFIT_GATE=1 /DBO1_DEBUGGER_GATE=1' +
        ' /I"' + (Join-Path $probeRoot 'src') + '" /I"' + $probeRoot + '" ' + $sources +
        ' /Fe"' + (Join-Path $archOutput 'zone.exe') + '" /Fo"' + $archOutput + '/" /link /OPT:REF'
    $command | Out-File (Join-Path $archOutput 'command.txt') -Encoding utf8
    & cmd /d /s /c $command 2>&1 | Out-File (Join-Path $archOutput 'compile.log') -Encoding utf8
    if ($LASTEXITCODE) { Get-Content (Join-Path $archOutput 'compile.log'); throw "$arch zone compile failed" }
    & (Join-Path $archOutput 'zone.exe') 2>&1 | Tee-Object -FilePath (Join-Path $archOutput 'test.log')
    if ($LASTEXITCODE) { throw "$arch zone test failed" }
}
Write-Output "Host zone evidence: $probeOutput"
