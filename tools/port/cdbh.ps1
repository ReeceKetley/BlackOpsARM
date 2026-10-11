param([string]$Name = 'x64hi_t', [string]$Build = 'build-x64-hi', [string]$Plan = 'qr wall_rottweil fire3', [int]$WaitSec = 120,
       [string]$Cmds = 'g;.lastevent;r;ub rip L10;u rip L4;k 14;q', [string]$Extra = '')
$root = 'D:\Desktop\Black Ops 2 Android\BlackOps64'
$run = "$root\build\run\$Name"
if (Get-Process BO1Zombies -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq "$run\BO1Zombies.exe" }) {
    throw "A game is already running in $run; use another run name."
}
foreach ($f in 'BO1Zombies.exe', 'BO1Zombies.pdb') { Copy-Item "$root\$Build\Release\$f" "$run\$f" -Force }
Copy-Item "$root\$Build\Release\BO1Zombies.pdb" "$root\$Build\Release\%%%BO1Zombies.pdb%%%" -Force
$log = "$run\main\console_mp.log"; if (Test-Path $log) { [IO.File]::Delete($log) }
Set-Location $run
$pre = ".sympath $root\$Build\Release`n.reload /f`n"
($pre + ($Cmds -replace ';', "`n")) | Set-Content "$run\_cdbh.txt" -Encoding ascii
$a = "-cf _cdbh.txt `"$run\BO1Zombies.exe`" +set fs_b `"$root\build\Release`" +set fs_h `"$run`" +set bo1_headless 1 +set bo1_zombies 1 +set net_ip 127.0.0.1 +set logfile 2 +set bo1_autoquit 90000 +set QuitOnError 1 +set developer_script 1 +set bo1_testclient 1 +set bo1_testclient_fight 1 +set bo1_testclient_god 1 +set bo1_testclient_score 10000 +set bo1_testclient_plan `"$Plan`" $Extra +devmap zombie_pentagon"
$out = "$run\cdb_out.txt"
$p = Start-Process -FilePath 'C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\cdb.exe' -ArgumentList $a -WorkingDirectory $run -RedirectStandardOutput $out -NoNewWindow -PassThru
$t = 0
while (-not $p.HasExited -and $t -lt $WaitSec) { Start-Sleep 2; $t += 2 }
if (-not $p.HasExited) { $p.Kill(); 'killed (timeout)' }
Get-Content $out | Where-Object { $_ -match '^\(|Access violation|exception|^___BO1|^BO1Zombies!|^00000000|rip=|^\s*r[a-z0-9]+=|^Last event|^\s+r[0-9a-z]+=' } | Select-Object -Last 44 | ForEach-Object { if ($_.Length -gt 200) { $_.Substring(0, 200) } else { $_ } }
