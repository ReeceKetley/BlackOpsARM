param([string]$VcVars = 'C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat')
$ErrorActionPreference = 'Stop'
$probeRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$probeOutput = Join-Path $probeRoot ('build-android-audit/host-parsetree-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmssfff'))
New-Item -ItemType Directory -Path $probeOutput | Out-Null
foreach ($arch in @('x86', 'x64')) {
    $archOutput = Join-Path $probeOutput $arch
    New-Item -ItemType Directory -Path $archOutput | Out-Null
    $defines = if ($arch -eq 'x64') { '/DBO1_PORT_X64=1' } else { '' }
    foreach ($name in @('parsetree', 'hunk')) {
        $files = @("tools/port/tests/${name}_tests.cpp", 'tools/port/tests/hunk_test_support.cpp',
            'tools/port/tests/hunk_test_memory_windows.cpp', 'src/clientscript/cscr_parsetree.cpp',
            'src/universal/mem_userhunk.cpp', 'src/universal/narrow32.cpp')
        $sources = ($files | ForEach-Object { '"' + (Join-Path $probeRoot $_) + '"' }) -join ' '
        $command = '"' + $VcVars + '" ' + $arch + ' >nul && cl /nologo /W4 /std:c++20 /EHsc /MTd /O2 /Gy /D_DEBUG=1 ' + $defines + ' /I"' + (Join-Path $probeRoot 'src') + '" /I"' + $probeRoot + '" ' + $sources + ' /Fe"' + (Join-Path $archOutput ($name + '.exe')) + '" /Fo"' + $archOutput + '/" /link /OPT:REF'
        $compileLog = Join-Path $archOutput ($name + '-compile.log')
        $command | Out-File (Join-Path $archOutput ($name + '-command.txt')) -Encoding utf8
        & cmd /d /s /c $command 2>&1 | Out-File $compileLog -Encoding utf8
        if ($LASTEXITCODE) { Get-Content $compileLog; throw "$arch $name compile failed" }
        & (Join-Path $archOutput ($name + '.exe')) 2>&1 | Tee-Object -FilePath (Join-Path $archOutput ($name + '-test.log'))
        if ($LASTEXITCODE) { throw "$arch $name test failed" }
    }
}
Write-Output "Host parse-tree/allocator evidence: $probeOutput"
