param([string]$VcVars = 'C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat', [string]$PythonExe = 'python')
$ErrorActionPreference = 'Stop'
$probeRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$probeOutput = Join-Path $probeRoot ('build-android-audit/host-debugger-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmssfff'))
New-Item -ItemType Directory -Path $probeOutput | Out-Null
& $PythonExe (Join-Path $PSScriptRoot 'generate_evaluator_test_services.py') (Join-Path $probeOutput 'evaluator_test_services.cpp')
if ($LASTEXITCODE) { throw 'Test endpoint generation failed' }
foreach ($arch in @('x86', 'x64')) {
    $archOutput = Join-Path $probeOutput $arch
    New-Item -ItemType Directory -Path $archOutput | Out-Null
    $defines = if ($arch -eq 'x64') { '/DBO1_PORT_X64=1' } else { '' }
    foreach ($name in @('debugger', 'firstfit')) {
        $files = @("tools/port/tests/${name}_tests.cpp", 'tools/port/tests/hunk_test_support.cpp',
            'tools/port/tests/hunk_test_memory_windows.cpp', 'src/clientscript/cscr_parsetree.cpp',
            'src/universal/mem_userhunk.cpp', 'src/universal/narrow32.cpp')
        if ($name -eq 'debugger') {
            $files += @('src/clientscript/cscr_evaluate.cpp', 'src/platform/android/zone_heap.cpp', (Join-Path $probeOutput 'evaluator_test_services.cpp'))
            $gateDefines = '/DBO1_DEBUGGER_GATE=1 /DBO1_ZONE_HEAP_HOST_TEST=1'
        } else {
            $files += @('src/universal/mem_firstfit.cpp', 'tools/port/tests/firstfit_windows_services.cpp')
            $gateDefines = '/DBO1_REAL_FIRSTFIT_GATE=1'
        }
        $sources = ($files | ForEach-Object { '"' + $(if ([IO.Path]::IsPathRooted($_)) { $_ } else { Join-Path $probeRoot $_ }) + '"' }) -join ' '
        $command = '"' + $VcVars + '" ' + $arch + ' >nul && cl /nologo /W4 /std:c++20 /EHsc /MTd /O2 /Gy /D_DEBUG=1 ' + $defines + ' ' + $gateDefines + ' /I"' + (Join-Path $probeRoot 'src') + '" /I"' + $probeRoot + '" ' + $sources + ' /Fe"' + (Join-Path $archOutput ($name + '.exe')) + '" /Fo"' + $archOutput + '/" /link /OPT:REF'
        $compileLog = Join-Path $archOutput ($name + '-compile.log')
        $command | Out-File (Join-Path $archOutput ($name + '-command.txt')) -Encoding utf8
        & cmd /d /s /c $command 2>&1 | Out-File $compileLog -Encoding utf8
        if ($LASTEXITCODE) { Get-Content $compileLog; throw "$arch $name compile failed" }
        & (Join-Path $archOutput ($name + '.exe')) 2>&1 | Tee-Object -FilePath (Join-Path $archOutput ($name + '-test.log'))
        if ($LASTEXITCODE) { throw "$arch $name test failed" }
    }
}
Write-Output "Host debugger/first-fit evidence: $probeOutput"
