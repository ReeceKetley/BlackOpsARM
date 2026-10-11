param([string]$VcVars = 'C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat')
$ErrorActionPreference = 'Stop'
$probeRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$probeOutput = Join-Path $probeRoot ('build-android-audit/host-rng-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmssfff'))
New-Item -ItemType Directory -Path $probeOutput | Out-Null
foreach ($arch in @('x86','x64')) {
    $archOutput = Join-Path $probeOutput $arch
    New-Item -ItemType Directory -Path $archOutput | Out-Null
    $command = '"' + $VcVars + '" ' + $arch + ' >nul && cl /nologo /W4 /WX /std:c++20 /EHsc /MT /O2 /DBO1_CRT_RNG_HOST_TEST=1 /I"' + (Join-Path $probeRoot 'src') + '" "' + (Join-Path $probeRoot 'tools/port/tests/engine_rng_oracle.cpp') + '" "' + (Join-Path $probeRoot 'src/platform/android/engine_random.cpp') + '" /Fe"' + (Join-Path $archOutput 'rng_oracle.exe') + '" /Fo"' + $archOutput + '/"'
    $command | Out-File (Join-Path $archOutput 'command.txt') -Encoding utf8
    & cmd /d /s /c $command 2>&1 | Out-File (Join-Path $archOutput 'compile.log') -Encoding utf8
    if ($LASTEXITCODE) { Get-Content (Join-Path $archOutput 'compile.log'); throw "$arch RNG oracle compile failed" }
    & (Join-Path $archOutput 'rng_oracle.exe') 2>&1 | Tee-Object -FilePath (Join-Path $archOutput 'test.log')
    if ($LASTEXITCODE) { throw "$arch RNG oracle failed" }
    & (Join-Path $archOutput 'rng_oracle.exe') --emit-vectors | Out-File (Join-Path $archOutput 'windows_crt_rng_vectors.h') -Encoding utf8
    if ($LASTEXITCODE) { throw "$arch RNG capture failed" }
}
$left = Get-FileHash (Join-Path $probeOutput 'x86/windows_crt_rng_vectors.h')
$right = Get-FileHash (Join-Path $probeOutput 'x64/windows_crt_rng_vectors.h')
if ($left.Hash -ne $right.Hash) { throw 'Windows x86/x64 reference vectors differ' }
$fixture = Join-Path $PSScriptRoot 'tests/windows_crt_rng_vectors.h'
if (Test-Path -LiteralPath $fixture) {
    if ((Get-FileHash $fixture).Hash -ne $left.Hash) { throw 'Reference CRT differs from retained test fixture' }
}
Write-Output "Host RNG evidence: $probeOutput"
