param(
    [Parameter(Mandatory=$true)][string]$Serial,
    [Parameter(Mandatory=$true)][string]$BuildDirectory,
    [string]$SdkRoot = 'D:/Android/Sdk'
)
$ErrorActionPreference = 'Stop'
$auditRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$auditBuild = [IO.Path]::GetFullPath((Join-Path $auditRoot $BuildDirectory))
if (!$auditBuild.StartsWith($auditRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw 'Build must be inside the Android worktree' }
$adb = Join-Path $SdkRoot 'platform-tools/adb.exe'
$deviceDir = '/data/local/tmp/blackopsarm-phase1-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmssfff')
$log = Join-Path $auditBuild 'device-tests.log'
if (Test-Path -LiteralPath $log) { throw 'Device test evidence exists; use a fresh build directory' }
& $adb -s $Serial shell "test ! -e $deviceDir && mkdir $deviceDir"
if ($LASTEXITCODE) { throw 'Could not create a fresh device test directory' }
"Device test directory: $deviceDir" | Out-File $log -Encoding utf8
& $adb -s $Serial shell 'getprop ro.product.model; getprop ro.product.cpu.abilist; getprop ro.build.version.sdk' | Tee-Object -FilePath $log -Append
$testNames = @('bo1_win32_compat_tests', 'bo1_arm64_narrow_tests', 'bo1_script_slot_tests')
if (Test-Path -LiteralPath (Join-Path $auditBuild 'bo1_script_headers_tests')) {
    $testNames += @('bo1_script_headers_tests', 'bo1_profile_tests', 'bo1_math_axis_tests', 'bo1_rng_tests')
}
if (Test-Path -LiteralPath (Join-Path $auditBuild 'bo1_script_lookup_tests')) {
    $testNames += @('bo1_engine_rng_tests', 'bo1_script_lookup_tests')
}
if (Test-Path -LiteralPath (Join-Path $auditBuild 'bo1_parsetree_tests')) {
    $testNames += @('bo1_parsetree_tests', 'bo1_hunk_tests')
}
if (Test-Path -LiteralPath (Join-Path $auditBuild 'bo1_debugger_tests')) {
    $testNames += @('bo1_debugger_tests', 'bo1_firstfit_tests')
}
foreach ($name in $testNames) {
    $binary = Join-Path $auditBuild $name
    if (!(Test-Path -LiteralPath $binary)) { throw "Missing test executable: $binary" }
    & $adb -s $Serial push $binary "$deviceDir/$name" 2>&1 | Out-File $log -Append -Encoding utf8
    if ($LASTEXITCODE) { throw "Push failed: $name" }
    & $adb -s $Serial shell "chmod 700 $deviceDir/$name && $deviceDir/$name" 2>&1 | Tee-Object -FilePath $log -Append
    $result = $LASTEXITCODE
    "${name}: exit=$result" | Tee-Object -FilePath $log -Append
    if ($result) { throw "Device test failed: $name" }
}
# Leave this run's native binaries for reproducibility. No package install,
# persistent device setting, or deletion of another run is performed.
