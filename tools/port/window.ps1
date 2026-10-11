param(
    [Parameter(Mandatory)][string]$Name,
    [string]$Build = 'build-x64-hi',
    [string]$Map = 'zombie_pentagon',
    [string]$SnapshotRun = '',
    [string]$Plan = 'qr wall_rottweil fire3 ammo_rottweil door_conf debris_elev2 floor2 debris_stair debris_elev1 battery floor3 power jugg speedcola doubletap box pap',
    [int]$AutoquitMs = 600000,
    [int]$QuitDone = 3000,
    [switch]$NoSound,
    [int]$X = -1850,
    [int]$Y = 30,
    [switch]$NoGod,
    [switch]$ManualPlay,
    [string]$Extra = '',
    [switch]$Wait
)
$ErrorActionPreference = 'Stop'
$root = 'D:\Desktop\Black Ops 2 Android\BlackOps64'
$run = Join-Path "$root\build\run" $Name
if (Test-Path $run) { throw "Use a fresh run name: $run already exists." }
New-Item -ItemType Directory -Path "$run\main" -Force | Out-Null
foreach ($dir in 'zone','mods') {
    New-Item -ItemType Junction -Path "$run\$dir" -Target "$root\build\Release\$dir" | Out-Null
}
foreach ($file in 'BO1Zombies.exe','BO1Zombies.pdb') {
    $source = if ($SnapshotRun) { "$root\build\run\$SnapshotRun\$file" } else { "$root\$Build\Release\$file" }
    Copy-Item $source "$run\$file"
}
$sidecar = if ($Build -eq 'build') { "$root\build\run\w32_reg3" } else { "$root\build\run\x64_thief_escape" }
foreach ($file in 'Steam.dll','binkw32.dll','cod.bmp','codlogo.bmp','d3dx9_43.dll','localization.txt','msvcp140.dll','steam_api.dll','steamclient.dll','tier0_s.dll','vcruntime140.dll','vcruntime140_1.dll','vstdlib_s.dll','xinput1_3.dll') {
    Copy-Item "$sidecar\$file" "$run\$file"
}
$nosnd = [int]$NoSound.IsPresent
$audioArg = if ($NoSound) { '+set nosnd 1' } else { '' }
$god = [int](-not $NoGod.IsPresent -and -not $ManualPlay.IsPresent)
$mouse = [int]$ManualPlay.IsPresent
$harnessArg = if ($ManualPlay) {
    '+set bo1_testclient_realwindow 0 +set bo1_testclient 0 +set bo1_testclient_fight 0 +set bo1_testclient_god 0'
} else {
    "+set bo1_testclient_realwindow 1 +set bo1_testclient 1 +set bo1_testclient_fight 1 +set bo1_testclient_god $god +set bo1_testclient_score 10000 +set bo1_testclient_quitdone $QuitDone +set bo1_testclient_plan `"$Plan`""
}
$args = "+set fs_b `"$root\build\Release`" +set fs_h `"$run`" +set bo1_zombies 1 +set net_ip 127.0.0.1 +set logfile 2 +set bo1_autoquit $AutoquitMs +set QuitOnError 1 +set developer_script 1 +set r_fullscreen 0 +set r_borderless 0 +set r_customMode `"1024x576`" +set vid_xpos $X +set vid_ypos $Y +set in_mouse $mouse $audioArg $harnessArg $Extra +devmap $Map"
$previousDiag = $env:BO1_DIAG_LOG
try {
    $env:BO1_DIAG_LOG = "$run\x64_diag.log"
    $p = Start-Process -FilePath "$run\BO1Zombies.exe" -ArgumentList $args -WorkingDirectory $run -PassThru
} finally { $env:BO1_DIAG_LOG = $previousDiag }
"started pid $($p.Id), sound=$($nosnd -eq 0), god=$($god -eq 1), manual=$($ManualPlay.IsPresent), window position ($X,$Y), run=$run"
$p.Id | Set-Content "$run\pid.txt"
# Inspect any optimal-settings prompt with the Computer Use skill. This helper does no UI input.
if ($Wait) {
    $p.WaitForExit()
    $p.ExitCode | Set-Content "$run\exitcode.txt"
    "exit code $($p.ExitCode)"
}
