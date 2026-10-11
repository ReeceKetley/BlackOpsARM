param([Parameter(Mandatory)][ValidatePattern('^[a-zA-Z0-9_-]+$')][string]$Name,
      [string]$Fixture = 'build/run/water_retail49.bin', [switch]$NativeD3D9,
      [string]$PackageFile = 'build/deps/dxvk-win64-patched/package.json')
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot/../..").Path
$run = "$root/build/run/$Name"
$exePath = [IO.Path]::GetFullPath("$run/test_water_skin_gpu_x64.exe")
if (Test-Path -LiteralPath $run) { throw 'Use a fresh run name' }
New-Item -ItemType Directory -Path $run | Out-Null
Copy-Item -LiteralPath "$root/build/run/test_water_skin_gpu_x64.exe" -Destination "$run/test_water_skin_gpu_x64.exe"
$package = $null
if (!$NativeD3D9) {
    $package = Get-Content -LiteralPath "$root/$PackageFile" -Raw | ConvertFrom-Json
    if ($package.architecture -ne 'x64' -or $package.api -ne 'd3d9') { throw 'Expected x64 D3D9 package' }
    if ((Get-FileHash -LiteralPath $package.artifactPath).Hash -ne $package.dllSha256) { throw 'DXVK hash mismatch' }
    Copy-Item -LiteralPath $package.artifactPath -Destination "$run/d3d9.dll"
}
$environment = @{
    DXVK_LOG_LEVEL = 'info'; DXVK_LOG_PATH = $run; DXVK_HUD = ''; DXVK_DEBUG = 'validation'
    DXVK_CONFIG = $null; DXVK_CONFIG_FILE = $null; DXVK_SHADER_CACHE_PATH = "$run/cache"
    VK_INSTANCE_LAYERS = 'VK_LAYER_KHRONOS_validation'; VK_LOADER_LAYERS_DISABLE = '~implicit~'
    VK_LOADER_LAYERS_ENABLE = $null; VK_LOADER_LAYERS_ALLOW = $null
    VK_SCREENSHOT_FRAMES = $null; VK_SCREENSHOT_DIR = $null
}
[ordered]@{ backend = $(if ($NativeD3D9) { 'system-d3d9' } else { 'dxvk-vulkan' });
    executableSha256 = (Get-FileHash -LiteralPath "$run/test_water_skin_gpu_x64.exe").Hash;
    fixtureSha256 = (Get-FileHash -LiteralPath "$root/$Fixture").Hash; package = $package;
    environment = $environment } | ConvertTo-Json -Depth 6 | Set-Content "$run/launch.json"
$saved = @{}
try {
    foreach ($key in $environment.Keys) {
        $saved[$key] = [Environment]::GetEnvironmentVariable($key, 'Process')
        [Environment]::SetEnvironmentVariable($key, $environment[$key], 'Process')
    }
    $p = Start-Process -FilePath $exePath -ArgumentList "`"$root/$Fixture`"" -WorkingDirectory $run -WindowStyle Hidden -PassThru -RedirectStandardOutput "$run/stdout.log" -RedirectStandardError "$run/stderr.log"
    $null = $p.Handle
} finally {
    foreach ($key in $saved.Keys) { [Environment]::SetEnvironmentVariable($key, $saved[$key], 'Process') }
}
if (!$p.WaitForExit(30000)) {
    if ($p.Path -ne $exePath) { throw 'Unexpected helper process path' }
    $p.Kill(); $p.WaitForExit(); throw 'Water GPU fixture timed out'
}
$p.WaitForExit(); $p.Refresh(); [IO.File]::WriteAllText("$run/exitcode.txt", [string]$p.ExitCode)
Get-Content "$run/stdout.log"
Get-Content "$run/stderr.log"
if ($p.ExitCode -ne 0) { throw "Water GPU fixture failed: $($p.ExitCode)" }
if (!$NativeD3D9) {
    $errors = @(Select-String -LiteralPath "$run/test_water_skin_gpu_x64_d3d9.log" -Pattern '^(err|error):')
    if ($errors.Count) { $errors; throw 'DXVK/Vulkan validation failed' }
    if (!(Select-String -LiteralPath "$run/stdout.log" -Pattern 'MODULE vulkan-1.dll:' -Quiet)) { throw 'Vulkan module not confirmed' }
}
"PASS ${Name}: exit $($p.ExitCode), real vertex buffer output matches retail"
