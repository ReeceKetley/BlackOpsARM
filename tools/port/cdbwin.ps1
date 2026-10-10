param([int]$WaitSec = 120, [string]$Extra = '+set nosnd 1 +devmap zombie_pentagon', [string]$Cmds = 'g;.lastevent;r;k 20;q', [string]$Build = 'build-x64')
$root = 'D:\Desktop\Black Ops 2 Android\BlackOps64'
$run = "$root\build\run\x64smoke"
Get-Process BO1Zombies,cdb -ErrorAction SilentlyContinue | Stop-Process -Force
Copy-Item "$root\$Build\Release\BO1Zombies.exe" "$run\BO1Zombies.exe" -Force
Copy-Item "$root\$Build\Release\BO1Zombies.pdb" "$run\BO1Zombies.pdb" -Force
Copy-Item "$root\$Build\Release\BO1Zombies.pdb" "$root\$Build\Release\%%%BO1Zombies.pdb%%%" -Force # percent_pdb
Set-Location $run
$pre = ".sympath $root\$Build\Release`n.reload /f`n"
($pre + ($Cmds -replace ';', "`n")) | Set-Content "$run\_cdbw.txt" -Encoding ascii
$a = @('-g','-cf','_cdbw.txt',"$run\BO1Zombies.exe",'+set','fs_b',"$root\build\Release",'+set','fs_h',$run,'+set','bo1_zombies','1','+set','logfile','2','+set','r_fullscreen','0') + ($Extra -split ' ')
$a2 = $a | ForEach-Object { if ($_ -match ' ') { '"' + $_ + '"' } else { $_ } }
$out = "$run\..\cdbw_out.txt"
$p = Start-Process -FilePath 'C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\cdb.exe' -ArgumentList ($a2 -join ' ') -WorkingDirectory $run -RedirectStandardOutput $out -NoNewWindow -PassThru
$sh = New-Object -ComObject WScript.Shell
$t = 0
while (-not $p.HasExited -and $t -lt $WaitSec) {
    Start-Sleep 2; $t += 2
    $w = Get-Process BO1Zombies -ErrorAction SilentlyContinue
    if ($w -and $w.MainWindowTitle -like 'Set Optimal*') { if ($sh.AppActivate($w.Id)) { Start-Sleep 1; $sh.SendKeys('n') } }
}
if (-not $p.HasExited) { $p.Kill(); 'killed (timeout)' }
Get-Content $out | Where-Object { $_ -match '^\(|Access violation|exception|^BO1Zombies!|^___BO1|^00000000|rip=|^\s*r[a-z0-9]+=|^Last event' } | Select-Object -Last 36 | ForEach-Object { if ($_.Length -gt 200) { $_.Substring(0,200) } else { $_ } }

