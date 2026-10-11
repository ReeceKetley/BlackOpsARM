param(
    [Parameter(Mandatory)][ValidatePattern('^[a-zA-Z0-9_-]+$')][string]$Name,
    [string]$Build = 'build-x64-hi',
    [string]$PackageFile = '',
    [ValidatePattern('^zombie_[a-z0-9_]+$')][string]$Map = 'zombie_pentagon',
    [string]$Plan = 'qr throw_frag throw_frag',
    [ValidatePattern('^[a-zA-Z0-9_,: ]*$')][string]$Weapons = '',
    [int]$AutoquitMs = 600000,
    [int]$QuitDone = 20000,
    [int]$X = 60,
    [int]$Y = 60,
    [switch]$NativeD3D9,
    [switch]$Validation,
    [switch]$AllowImplicitLayers,
    [string]$Hud = 'devinfo,version,api,fps,compiler',
    [string]$CaptureFrames = '',
    [string]$Extra = '',
    [switch]$ManualPlay,
    [switch]$Wait
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
if ($ManualPlay -and $Weapons) { throw 'Choose manual play or an automated weapon sweep' }
if (-not $PackageFile) { $PackageFile = Join-Path $PSScriptRoot 'dxvk-package.json' }
$package = Get-Content -LiteralPath $PackageFile -Raw | ConvertFrom-Json
if ($package.architecture -ne 'x64' -or $package.api -ne 'd3d9') { throw 'Expected an x64 D3D9 package manifest' }
$run = Join-Path "$root/build/run" $Name
if (Test-Path -LiteralPath $run) { throw 'Use a fresh run name' }
function Assert-X64PE([string]$Path) {
    $bytes = [IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 64 -or [BitConverter]::ToUInt16($bytes, 0) -ne 0x5a4d) { throw "Not a PE file: $Path" }
    $offset = [BitConverter]::ToInt32($bytes, 0x3c)
    if ($offset -lt 0 -or $offset + 6 -gt $bytes.Length -or
        [BitConverter]::ToUInt32($bytes, $offset) -ne 0x4550 -or
        [BitConverter]::ToUInt16($bytes, $offset + 4) -ne 0x8664) { throw "Expected x64 PE: $Path" }
}
$exeSource = Join-Path "$root/$Build/Release" 'BO1Zombies.exe'
Assert-X64PE $exeSource
if (-not $NativeD3D9) {
  if ($package.artifactPath) {
    $dllSource = $package.artifactPath
    if ((Get-FileHash -LiteralPath $dllSource -Algorithm SHA256).Hash -ne $package.dllSha256) { throw 'Built DXVK DLL checksum mismatch' }
    Assert-X64PE $dllSource
  } else {
    $deps = "$root/build/deps"
    New-Item -ItemType Directory -Path $deps -Force | Out-Null
    $archive = "$deps/dxvk-$($package.version).tar.gz"
    if (-not (Test-Path -LiteralPath $archive)) { Invoke-WebRequest -Uri $package.url -OutFile $archive }
    if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant() -ne $package.archiveSha256) { throw 'DXVK archive checksum mismatch' }
    # Extract again from the verified archive, rather than trust an old extracted DLL.
    & tar -xzf $archive -C $deps
    if ($LASTEXITCODE -ne 0) { throw 'DXVK extraction failed' }
    $dllSource = "$deps/dxvk-$($package.version)/x64/d3d9.dll"
    Assert-X64PE $dllSource
  }
}
New-Item -ItemType Directory -Path "$run/main" -Force | Out-Null
# Startup config is executed after early command-line variables, and before the
# recommended-settings check. Seed only this disposable profile's hardware state.
New-Item -ItemType Directory -Path "$run/players" -Force | Out-Null
@('seta com_recommendedSet "1"', 'seta sys_configureGHz "4.8"',
  'seta sys_sysMB "1024"', 'seta sys_gpu "Unknown video card"',
  'seta sys_configSum "0"') | Set-Content "$run/players/config.cfg" -Encoding ASCII
foreach ($dir in 'zone','mods') { New-Item -ItemType Junction -Path "$run/$dir" -Target "$root/build/Release/$dir" | Out-Null }
foreach ($file in 'BO1Zombies.exe','BO1Zombies.pdb') { Copy-Item "$root/$Build/Release/$file" "$run/$file" }
foreach ($file in 'Steam.dll','binkw32.dll','cod.bmp','codlogo.bmp','d3dx9_43.dll','localization.txt','msvcp140.dll','steam_api.dll','steamclient.dll','tier0_s.dll','vcruntime140.dll','vcruntime140_1.dll','vstdlib_s.dll','xinput1_3.dll') {
    Copy-Item "$root/build/run/x64_thief_escape/$file" "$run/$file"
}
if (-not $NativeD3D9) { Copy-Item $dllSource "$run/d3d9.dll" }
$hardware = '+set com_recommendedSet 1 +set sys_configureGHz 4.8 +set sys_sysMB 1024 +set sys_gpu "Unknown video card" +set sys_configSum 0'
$mode = if ($ManualPlay) { '+set bo1_testclient 0 +set bo1_testclient_realwindow 0 +set in_mouse 1' } elseif ($Weapons) {
    "+set bo1_testclient_realwindow 1 +set bo1_testclient 1 +set bo1_testclient_fight 1 +set bo1_testclient_god 1 +set bo1_weaponsweep `"$Weapons`" +set in_mouse 0"
} else {
    "+set bo1_testclient_realwindow 1 +set bo1_testclient 1 +set bo1_testclient_fight 1 +set bo1_testclient_god 1 +set bo1_testclient_score 10000 +set bo1_testclient_quitdone $QuitDone +set bo1_testclient_plan `"$Plan`" +set in_mouse 0"
}
# Sound enabled by omitting nosnd entirely (the engine tests its raw command line).
$gameArgs = "+set fs_b `"$root/build/Release`" +set fs_h `"$run`" +set bo1_zombies 1 +set net_ip 127.0.0.1 +set logfile 2 +set bo1_autoquit $AutoquitMs +set QuitOnError 1 +set developer_script 1 +set r_fullscreen 0 +set r_borderless 0 +set r_customMode 1024x576 +set vid_xpos $X +set vid_ypos $Y $mode $hardware $Extra +devmap $Map"
if ($gameArgs.Length -gt 1023) { throw 'Game command line exceeds WinMain buffer capacity' }
$envValues = @{
    BO1_DIAG_LOG = "$run/x64_diag.log"
    BO1_X64_GLASS = [Environment]::GetEnvironmentVariable('BO1_X64_GLASS', 'Process')
    DXVK_HUD = $Hud
    DXVK_LOG_LEVEL = 'info'
    DXVK_LOG_PATH = $run
    DXVK_SHADER_CACHE_PATH = "$run/shader-cache"
    DXVK_CONFIG = $null
    DXVK_CONFIG_FILE = "$run/dxvk.conf"
    DXVK_DEBUG = $(if ($Validation) { 'validation' } else { $null })
    VK_INSTANCE_LAYERS = $null
    # Isolate graphics tests from system-wide capture/overlay hooks. No registry changes.
    VK_LOADER_LAYERS_DISABLE = $(if ($AllowImplicitLayers) { $null } else { '~implicit~' })
    VK_LOADER_LAYERS_ENABLE = $null
    VK_LOADER_LAYERS_ALLOW = $null
    VK_SCREENSHOT_FRAMES = $null
    VK_SCREENSHOT_DIR = $null
}
$layers = @()
if ($Validation) { $layers += 'VK_LAYER_KHRONOS_validation' }
if ($CaptureFrames) {
    if ($NativeD3D9) { throw 'Vulkan frame capture requires DXVK' }
    if ($CaptureFrames -notmatch '^[0-9,-]+$') { throw 'Invalid capture frame list' }
    $layers += 'VK_LAYER_LUNARG_screenshot'
    New-Item -ItemType Directory -Path "$run/frames" | Out-Null
    $envValues.VK_SCREENSHOT_FRAMES = $CaptureFrames
    $envValues.VK_SCREENSHOT_DIR = "$run/frames"
}
if ($layers.Count) { $envValues.VK_INSTANCE_LAYERS = $layers -join ';' }
[IO.File]::WriteAllText("$run/dxvk.conf", "# Upstream defaults; deliberately no vendor-specific workarounds.`n")
$manifest = [ordered]@{
    backend = $(if ($NativeD3D9) { 'system-d3d9' } else { 'dxvk-vulkan' })
    package = $(if ($NativeD3D9) { $null } else { $package })
    sourceHead = (& git -C $root rev-parse HEAD)
    sourceDirty = [bool](& git -C $root status --porcelain --untracked-files=no)
    exeSha256 = (Get-FileHash "$run/BO1Zombies.exe").Hash
    dllSha256 = $(if ($NativeD3D9) { $null } else { (Get-FileHash "$run/d3d9.dll").Hash })
    map = $Map; plan = $(if ($Weapons -or $ManualPlay) { '' } else { $Plan })
    weapons = $Weapons; sound = $true; god = -not $ManualPlay.IsPresent
    gameArgs = $gameArgs; environment = $envValues; launchedUtc = [DateTime]::UtcNow.ToString('o')
}
$manifest | ConvertTo-Json -Depth 6 | Set-Content "$run/launch.json" -Encoding UTF8
$previousEnv = @{}
try {
    foreach ($key in $envValues.Keys) {
        $previousEnv[$key] = [Environment]::GetEnvironmentVariable($key, 'Process')
        [Environment]::SetEnvironmentVariable($key, $envValues[$key], 'Process')
    }
    $p = Start-Process -FilePath "$run/BO1Zombies.exe" -ArgumentList $gameArgs -WorkingDirectory $run -WindowStyle Normal -PassThru
    $null = $p.Handle
} finally {
    foreach ($key in $previousEnv.Keys) { [Environment]::SetEnvironmentVariable($key, $previousEnv[$key], 'Process') }
}
[IO.File]::WriteAllText("$run/pid.txt", [string]$p.Id)
"$Name pid=$($p.Id) backend=$($manifest.backend), sound ON, god=$($manifest.god), Asus position ($X,$Y)"
if ($Wait) {
    $watch = [Diagnostics.Stopwatch]::StartNew()
    $nextModuleSample = 1000
    while (!$p.WaitForExit(1000)) {
        if ($watch.ElapsedMilliseconds -ge $nextModuleSample) {
            $p.Refresh()
            $allModules = $p.Modules | Select-Object ModuleName,FileName
            $allModules | ConvertTo-Json | Set-Content "$run/all-modules.json" -Encoding UTF8
            $modules = $allModules | Where-Object { $_.ModuleName -match '^(d3d9|vulkan-1|nvoglv64|VkLayer_.*|graphics-hook.*|ow.*|Twitch.*|RTSS.*)\.dll$' }
            $modules | ConvertTo-Json | Set-Content "$run/modules.json" -Encoding UTF8
            $nextModuleSample += 15000
        }
        $emergency = "$run/main/bo1_emergency.log"
        $crashed = (Test-Path -LiteralPath $emergency) -and (Select-String -LiteralPath $emergency -Pattern '^BO1_HEADLESS CRASH:' -Quiet)
        if ($crashed -or $watch.ElapsedMilliseconds -gt ($AutoquitMs + 60000)) {
            if ($p.Path -ne "$run/BO1Zombies.exe".Replace('/', '\')) { throw 'Unexpected DXVK process path' }
            "Ending failed test: crash=$crashed elapsed=$($watch.ElapsedMilliseconds)"
            # The crash handler may exit between the timeout check and Kill.
            if (!$p.WaitForExit(250)) {
                try { $p.Kill() } catch {
                    if (!$p.WaitForExit(1000)) { throw }
                }
            }
            $p.WaitForExit(); break
        }
    }
    $p.WaitForExit(); $p.Refresh()
    [IO.File]::WriteAllText("$run/exitcode.txt", [string]$p.ExitCode)
    "exit=$($p.ExitCode)"
    if ($p.ExitCode -ne 0) { throw "Renderer test failed: $Name, exit $($p.ExitCode)" }
    if ($Validation -and -not $NativeD3D9) {
        $validationErrors = @(Select-String -LiteralPath "$run/BO1Zombies_d3d9.log" -Pattern '^(err|error):' | ForEach-Object Line)
        [IO.File]::WriteAllText("$run/validation-errors.txt", ($validationErrors -join "`n"))
        if ($validationErrors.Count) { throw "Vulkan validation failed: $Name, $($validationErrors.Count) error log lines (engine exit was 0)" }
        'Vulkan validation: no error messages, including teardown'
    }
}
