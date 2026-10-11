param(
    [string]$SdkRoot = 'D:/Android/Sdk',
    [string]$BuildDirectory = ('build-android-audit/run-' + (Get-Date -Format yyyyMMddTHHmmssfff)),
    [switch]$Raw
)
$ErrorActionPreference = 'Stop'
$auditRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$auditBuild = [IO.Path]::GetFullPath((Join-Path $auditRoot $BuildDirectory))
if (-not $auditBuild.StartsWith($auditRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Audit output must be inside this Android worktree.'
}
$auditNdk = Join-Path $SdkRoot 'ndk/30.0.16248370'
$auditCmake = Join-Path $SdkRoot 'cmake/3.22.1/bin/cmake.exe'
$auditNinja = Join-Path $SdkRoot 'cmake/3.22.1/bin/ninja.exe'
$auditToolchain = Join-Path $auditNdk 'build/cmake/android.toolchain.cmake'
foreach ($required in @($auditCmake, $auditNinja, $auditToolchain)) {
    if (-not (Test-Path -LiteralPath $required)) { throw "Missing tool: $required" }
}
New-Item -ItemType Directory -Path $auditBuild -Force | Out-Null
$auditConfigureLog = Join-Path $auditBuild 'configure.log'
$auditBuildLog = Join-Path $auditBuild 'build.log'
foreach ($log in @($auditConfigureLog, $auditBuildLog)) {
    if (Test-Path -LiteralPath $log) { throw "Evidence exists; choose a fresh BuildDirectory: $log" }
}
$auditCompat = if ($Raw) { 'OFF' } else { 'ON' }
& $auditCmake -S (Join-Path $auditRoot 'android/cmake/arm64-audit') -B $auditBuild -G Ninja `
    "-DCMAKE_MAKE_PROGRAM=$auditNinja" "-DCMAKE_TOOLCHAIN_FILE=$auditToolchain" `
    '-DANDROID_ABI=arm64-v8a' '-DANDROID_PLATFORM=android-28' '-DCMAKE_BUILD_TYPE=Debug' `
    "-DBO1_ARM64_COMPAT=$auditCompat" 2>&1 | Out-File -LiteralPath $auditConfigureLog -Encoding utf8
if ($LASTEXITCODE) { Get-Content -LiteralPath $auditConfigureLog; exit $LASTEXITCODE }
& $auditCmake --build $auditBuild --verbose -- -k 0 2>&1 | Out-File -LiteralPath $auditBuildLog -Encoding utf8
$auditExitCode = $LASTEXITCODE
$auditText = Get-Content -LiteralPath $auditBuildLog -Raw
$auditErrors = ([regex]::Matches($auditText, '(?m)^.+?:\d+:\d+: (?:fatal )?error:')).Count
$auditWarnings = ([regex]::Matches($auditText, '(?m)^.+?:\d+:\d+: warning:')).Count
Write-Output "Build exit=$auditExitCode, errors=$auditErrors, warnings=$auditWarnings"
Write-Output "Full configure/build logs: $auditBuild"
if ($auditExitCode) { exit $auditExitCode }
