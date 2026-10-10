param([int]$WaitSec = 100, [string]$Cmds = 'g;.lastevent;r;ub rip L8;u rip L4;k 10;q', [switch]$NoBuild)
$sd = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = 'D:\Desktop\Black Ops 2 Android\BlackOps64'
Get-Process BO1Zombies,cdb -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep 1
if (-not $NoBuild) {
    Set-Location $root
    $o = cmake --build build-x64 --config Release -- /m /v:q 2>&1
    $e = $o | Select-String ' error ' | Select-Object -First 10
    if ($e) { $e | ForEach-Object { $_.Line.Substring(0, [Math]::Min(260, $_.Line.Length)) }; return }
}
$diag = "$root\build\run\x64smoke\x64_diag.log"
if (Test-Path $diag) { [IO.File]::Delete($diag) }
& "$sd\cdbwin.ps1" -WaitSec $WaitSec -Cmds $Cmds | Select-Object -Last 40
'--- diag tail'
if (Test-Path $diag) { Get-Content $diag | Where-Object { $_ -match 'ASSERT|CRASH|DPVS' } | Select-Object -Last 6 | ForEach-Object { $_.Substring(0, [Math]::Min(280, $_.Length)) } }
