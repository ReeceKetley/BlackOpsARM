param([int]$WaitSec = 40, [string]$Build = 'build-x64', [string]$Extra = '+devmap zombie_pentagon', [switch]$NoBuild, [int]$Tail = 12)
$root = 'D:\Desktop\Black Ops 2 Android\BlackOps64'
$run = "$root\build\run\x64smoke"
$diag = 'D:\Desktop\Black Ops 2 Android\docs\port\x64_diag.log'
Get-Process BO1Zombies -ErrorAction SilentlyContinue | Stop-Process -Force
if (-not $NoBuild) {
    Set-Location $root
    $r = cmake --build $Build --config Release -- /m /v:q 2>&1 | Select-String -Pattern ' error '
    $r | Select-Object -First 10 | ForEach-Object { $_.Line.Substring(0, [Math]::Min(250, $_.Line.Length)) }
}
Copy-Item "$root\$Build\Release\BO1Zombies.exe" "$run\BO1Zombies.exe" -Force
Copy-Item "$root\$Build\Release\BO1Zombies.pdb" "$run\BO1Zombies.pdb" -Force
Copy-Item "$root\$Build\Release\BO1Zombies.pdb" "$root\$Build\Release\%%%BO1Zombies.pdb%%%" -Force
Set-Location $run
$log = "$run\main\console_mp.log"
if (Test-Path $log) { [IO.File]::Delete($log) }
if (Test-Path $diag) { [IO.File]::Delete($diag) }
$env:BO1_DIAG_LOG = $diag
$p = Start-Process -FilePath "$run\BO1Zombies.exe" -ArgumentList "+set fs_b `"$root\build\Release`" +set fs_h `"$run`" +set bo1_zombies 1 +set logfile 2 +set r_fullscreen 0 $Extra" -WorkingDirectory $run -PassThru
$sh = New-Object -ComObject WScript.Shell
for ($t = 0; $t -lt $WaitSec; $t += 2) {
    Start-Sleep 2
    $p.Refresh()
    if ($p.HasExited) { break }
    if ($p.MainWindowTitle -like 'Set Optimal*') { if ($sh.AppActivate($p.Id)) { Start-Sleep 1; $sh.SendKeys('n'); 'answered optimal-settings prompt: No' } }
}
$p.Refresh()
"exited: $($p.HasExited)" + $(if ($p.HasExited) { " code $($p.ExitCode)" } else { " title [$($p.MainWindowTitle)] responding $($p.Responding)" })
Get-Content $log -ErrorAction SilentlyContinue | Select-Object -Last $Tail | ForEach-Object { if ($_.Length -gt 190) { $_.Substring(0,190) } else { $_ } }
if (Test-Path $diag) { '--- diag'; Get-Content $diag | Select-Object -First 8 | ForEach-Object { if ($_.Length -gt 300) { $_.Substring(0,300) } else { $_ } } }
# symbolize the engine's crash line + stack
$crash = Get-Content $log -ErrorAction SilentlyContinue | Where-Object { $_ -match '^BO1_HEADLESS CRASH' } | Select-Object -Last 1
if ($crash) {
    $sym = 'C:\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\llvm-symbolizer.exe'
    $at = [regex]::Match($crash, 'BO1Zombies\.exe\+0x([0-9A-Fa-f]+)').Groups[1].Value
    $st = (Get-Content $log | Where-Object { $_ -match '^BO1_HEADLESS STACK:' } | Select-Object -Last 1) -replace '^BO1_HEADLESS STACK:\s*', ''
    $fr = @($at) + (($st -split '\s+' | Where-Object { $_ -match '^\+0x' } | ForEach-Object { $_.Substring(3) }) | Select-Object -First 14)
    $tmp = Join-Path $env:TEMP 'rva_in.txt'
    ($fr | Where-Object { $_ -match '^[0-9a-fA-F]+$' } | ForEach-Object { '0x' + $_ }) | Set-Content $tmp -Encoding ascii
    '--- crash: ' + ($crash -replace 'thread \d+', '')
    $out = @(cmd /c "`"$sym`" --obj=`"$root\$Build\Release\BO1Zombies.exe`" --relative-address --no-inlines < `"$tmp`"" 2>&1 | Where-Object { $_ -ne '' })
    for ($i = 0; $i -lt $fr.Count; $i++) { '  #{0} +0x{1}  {2}' -f $i, $fr[$i], $out[2 * $i] }
}
