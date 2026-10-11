param(
    [string]$VcVars = 'C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat',
    [string]$PythonExe = 'python',
    [string[]]$GenuineFiles = @()
)
$ErrorActionPreference = 'Stop'
$probeRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$probeOutput = Join-Path $probeRoot ('build-android-audit/host-inflate-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmssfff'))
New-Item -ItemType Directory -Path $probeOutput | Out-Null
foreach ($arch in @('x86','x64')) {
    $archOutput = Join-Path $probeOutput $arch
    New-Item -ItemType Directory -Path $archOutput | Out-Null
    & $PythonExe (Join-Path $PSScriptRoot 'generate_inflate_fixtures.py') (Join-Path $archOutput 'inflate-fixtures') |
        Out-File (Join-Path $archOutput 'fixtures.log') -Encoding utf8
    if ($LASTEXITCODE) { throw 'Inflate fixture generation failed' }
    $defines = if ($arch -eq 'x64') { '/DBO1_PORT_X64=1' } else { '' }
    $files = @('tools/port/tests/inflate_tests.cpp','src/platform/fastfile_inflate.cpp',
        'src/zlib/inflate.c','src/zlib/inffast.c','src/zlib/inftrees.c','src/zlib/adler32.c','src/zlib/crc32.c','src/zlib/zutil.c')
    $sources = ($files | ForEach-Object { '"' + (Join-Path $probeRoot $_) + '"' }) -join ' '
    $command = '"' + $VcVars + '" ' + $arch + ' >nul && cl /nologo /W4 /std:c++20 /EHsc /MT /O2 ' + $defines +
        ' /I"' + (Join-Path $probeRoot 'src') + '" ' + $sources + ' /Fe"' + (Join-Path $archOutput 'inflate.exe') + '" /Fo"' + $archOutput + '/"'
    $command | Out-File (Join-Path $archOutput 'command.txt') -Encoding utf8
    & cmd /d /s /c $command 2>&1 | Out-File (Join-Path $archOutput 'compile.log') -Encoding utf8
    if ($LASTEXITCODE) { Get-Content (Join-Path $archOutput 'compile.log'); throw "$arch inflate compile failed" }
    $arguments = @($GenuineFiles | ForEach-Object { [IO.Path]::GetFullPath($_) })
    (& (Join-Path $archOutput 'inflate.exe') @arguments 2>&1) | Tee-Object -FilePath (Join-Path $archOutput 'test.log')
    if ($LASTEXITCODE) { throw "$arch inflate test failed" }
}
Write-Output "Host inflate evidence: $probeOutput"
