param(
    [Parameter(Mandatory)][string]$Name,
    [string]$Build = 'build-x64-hi',
    [string]$Map = 'zombie_pentagon',
    [string]$Weapons = 'catalog',
    [switch]$Catalog,
    [switch]$Debugger,
    [switch]$WaterCells,
    [int]$AutoquitMs = 3600000,
    [switch]$Wait
)
$ErrorActionPreference = 'Stop'
if ($WaterCells -and (!$Debugger -or !$Catalog)) { throw 'Water-cell inspection requires a debugger catalog run' }
$root = 'D:\Desktop\Black Ops 2 Android\BlackOps64'
if ($Map -notmatch '^(zombie_[a-z0-9_]+|zombietron)$' -or $Weapons -notmatch '^[a-zA-Z0-9_,: ]+$') { throw 'Invalid map/weapon name' }
$run = Join-Path "$root\build\run" $Name
if (Test-Path -LiteralPath $run) { throw 'Use a fresh run name' }
New-Item -ItemType Directory -Path "$run\main" -Force | Out-Null
foreach ($dir in 'zone','mods') { New-Item -ItemType Junction -Path "$run\$dir" -Target "$root\build\Release\$dir" | Out-Null }
foreach ($file in 'BO1Zombies.exe','BO1Zombies.pdb') { Copy-Item "$root\$Build\Release\$file" "$run\$file" }
$sidecar = if ($Build -eq 'build') { "$root\build\run\w32_reg3" } else { "$root\build\run\x64_thief_escape" }
foreach ($file in 'Steam.dll','binkw32.dll','cod.bmp','codlogo.bmp','d3dx9_43.dll','localization.txt','msvcp140.dll','steam_api.dll','steamclient.dll','tier0_s.dll','vcruntime140.dll','vcruntime140_1.dll','vstdlib_s.dll','xinput1_3.dll') { Copy-Item "$sidecar\$file" "$run\$file" }
# Catalogs only need the server's registered inventory. Runtime sweeps use the real rendered client and sound.
$mode = if ($Catalog) { '+set bo1_headless 1' } else { '+set bo1_testclient_realwindow 1' }
# Fresh test profiles receive the hardware values already observed in the successful play32 log.
# This avoids the optimal-settings dialog without UI automation or altering the user's game profile.
$hardware = '+set sys_configureGHz 4.8 +set sys_sysMB 1024 +set sys_gpu "Unknown video card" +set sys_configSum 0'
$args = "+set fs_b `"$root\build\Release`" +set fs_h `"$run`" +set bo1_zombies 1 +set net_ip 127.0.0.1 +set logfile 2 +set bo1_autoquit $AutoquitMs +set QuitOnError 1 +set developer_script 1 $mode +set bo1_testclient 1 +set bo1_testclient_fight 1 +set bo1_testclient_god 1 +set bo1_weaponsweep `"$Weapons`" +set r_fullscreen 0 +set r_borderless 0 +set r_customMode 1024x576 +set vid_xpos 60 +set vid_ypos 60 +set in_mouse 0 $hardware +devmap $Map"
# The engine copies its command line into a fixed buffer. A truncated tail can
# silently lose devmap and run the frontend instead of the requested map.
if ($args.Length -gt 1800) { throw 'Weapon command line is too long; use all or split the explicit list into shorter batches.' }
$previousDiag = $env:BO1_DIAG_LOG
try {
    $env:BO1_DIAG_LOG = "$run\x64_diag.log"
    $windowStyle = if ($Catalog) { 'Hidden' } else { 'Normal' }
    if ($Debugger) {
        Copy-Item "$run\BO1Zombies.pdb" "$run\%%%BO1Zombies.pdb%%%"
        $debugArch = if ($Build -eq 'build') { 'x86' } else { 'x64' }
        $debugExe = "C:\Program Files (x86)\Windows Kits\10\Debuggers\$debugArch\cdb.exe"
        $debugRegister = if ($Build -eq 'build') { 'eip' } else { 'rip' }
        if ($WaterCells) {
            $worldExpression = if ($Build -eq 'build') { 'poi(poi(@esp+4))' } else { 'poi(@rcx)' }
            $countOffset = if ($Build -eq 'build') { '0x20' } else { '0x28' }
            $headerOffset = if ($Build -eq 'build') { '0x10' } else { '0x18' }
            $cellsOffset = if ($Build -eq 'build') { '0x24' } else { '0x30' }
            $pointerCommand = if ($Build -eq 'build') { 'dd' } else { 'dq' }
            $captureCommand = 'bm *!Load_ComWorldAsset ".printf \"WATER_MAP_CELLS count=%u\n\", dwo(' + $worldExpression + '+' + $countOffset + '); da poi(' + $worldExpression + ') L80; db poi(' + $worldExpression + ') L30; dd ' + $worldExpression + '+' + $headerOffset + ' L4; ' + $pointerCommand + ' ' + $worldExpression + '+' + $cellsOffset + ' L1; g"'
            @('.reload /f', $captureCommand,
              'g','.lastevent','qd') | Set-Content "$run\_weapon_debug.cdb" -Encoding ascii
        } else {
            @('g','.lastevent','.ecxr','r','k 24',"ub @$debugRegister L8", ".dump /mi weapon_crash.dmp",'q') | Set-Content "$run\_weapon_debug.cdb" -Encoding ascii
        }
        $debugArgs = "-y `"$run`" -cf `"$run\_weapon_debug.cdb`" `"$run\BO1Zombies.exe`" $args"
        $p = Start-Process -FilePath $debugExe -ArgumentList $debugArgs -WorkingDirectory $run -WindowStyle Hidden -RedirectStandardOutput "$run\debug.log" -PassThru
    } else {
        $p = Start-Process -FilePath "$run\BO1Zombies.exe" -ArgumentList $args -WorkingDirectory $run -WindowStyle $windowStyle -PassThru
    }
} finally { $env:BO1_DIAG_LOG = $previousDiag }
$null = $p.Handle # Retain the launch handle so ExitCode remains available after short-lived processes exit.
$p.Id | Set-Content "$run\pid.txt"
"$Name pid=$($p.Id) build=$Build map=$Map catalog=$Catalog weapons=$Weapons"
if ($Wait) {
    $runWatch = [Diagnostics.Stopwatch]::StartNew()
    while (!$p.WaitForExit(1000)) {
        if ($Debugger) {
            if ($runWatch.ElapsedMilliseconds -le ($AutoquitMs + 60000)) { continue }
            if ($p.Path -ne $debugExe) { throw 'Unexpected weapon debugger path' }
            # Bound debugger hangs too, preserving unrelated debug/game sessions.
            Get-CimInstance Win32_Process -Filter "ParentProcessId=$($p.Id)" |
                Where-Object { $_.ExecutablePath -eq "$run\BO1Zombies.exe" } |
                ForEach-Object { Stop-Process -Id $_.ProcessId -ErrorAction SilentlyContinue }
            'Ending timed-out weapon debugger'
            $p.Kill()
            $p.WaitForExit()
            break
        }
        $emergency = "$run\main\bo1_emergency.log"
        $crashed = (Test-Path -LiteralPath $emergency) -and
            (Select-String -LiteralPath $emergency -Pattern '^BO1_HEADLESS CRASH:' -Quiet)
        if ($crashed -or $runWatch.ElapsedMilliseconds -gt ($AutoquitMs + 60000)) {
            if ($p.Path -ne "$run\BO1Zombies.exe") { throw 'Unexpected weapon process path' }
            "Ending failed test process: crash=$crashed elapsed=$($runWatch.ElapsedMilliseconds)"
            $p.Kill()
            $p.WaitForExit()
            break
        }
    }
    $p.WaitForExit()
    $p.Refresh()
    $p.ExitCode | Set-Content "$run\exitcode.txt"
    "exit=$($p.ExitCode)"
    if ($p.ExitCode -ne 0) { throw "Weapon run failed: $Name, exit $($p.ExitCode)" }
}
