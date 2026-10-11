param(
    [Parameter(Mandatory)][string]$Name,
    [string]$Build = 'build-x64',
    [Parameter(Mandatory)][string]$Plan,
    [int]$AutoquitMs = 300000,
    [string]$Extra = '',
    [switch]$God,
    [int]$Score = 10000,
    [int]$QuitDone = 5000,
    [switch]$Wait
)
$root = 'D:\Desktop\Black Ops 2 Android\BlackOps64'
$run = "$root\build\run\$Name"
$src = "$root\$Build\Release"
if (Get-Process BO1Zombies -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq "$run\BO1Zombies.exe" }) {
    throw "A game is already running in $run; use another run name."
}
if (-not (Test-Path $run)) {
    New-Item -ItemType Directory -Path $run | Out-Null
    New-Item -ItemType Directory -Path "$run\main" | Out-Null
    cmd /c "mklink /J `"$run\zone`" `"$root\build\Release\zone`"" | Out-Null
    cmd /c "mklink /J `"$run\mods`" `"$root\build\Release\mods`"" | Out-Null
}
foreach ($f in 'BO1Zombies.exe','BO1Zombies.pdb') { Copy-Item "$src\$f" "$run\$f" -Force }
$sidecar = if ($Build -eq 'build') { "$root\build\run\w32_reg3" } else { "$root\build\run\x64_thief_escape" }
foreach ($f in 'Steam.dll','binkw32.dll','cod.bmp','codlogo.bmp','d3dx9_43.dll','localization.txt','msvcp140.dll','steam_api.dll','steamclient.dll','tier0_s.dll','vcruntime140.dll','vcruntime140_1.dll','vstdlib_s.dll','xinput1_3.dll') {
    if (-not (Test-Path "$run\$f")) { Copy-Item "$sidecar\$f" "$run\$f" -Force }
}
$log = "$run\main\console_mp.log"
if (Test-Path $log) { [IO.File]::Delete($log) }
$godArg = if ($God) { '+set bo1_testclient_god 1' } else { '' }
$args = "+set fs_b `"$root\build\Release`" +set fs_h `"$run`" +set bo1_headless 1 +set bo1_zombies 1 +set net_ip 127.0.0.1 +set logfile 2 +set bo1_autoquit $AutoquitMs +set QuitOnError 1 +set developer_script 1 +set bo1_testclient 1 +set bo1_testclient_fight 1 $godArg +set bo1_testclient_score $Score +set bo1_testclient_quitdone $QuitDone +set bo1_testclient_plan `"$Plan`" $Extra +devmap zombie_pentagon"
$p = Start-Process -FilePath "$run\BO1Zombies.exe" -ArgumentList $args -WorkingDirectory $run -WindowStyle Hidden -PassThru
"started pid $($p.Id) in $run"
if ($Wait) { $p.WaitForExit(); "exit code $($p.ExitCode)" }
