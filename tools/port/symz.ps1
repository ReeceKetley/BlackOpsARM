param([Parameter(Mandatory)][string]$Run, [string]$Build = 'build-x64-hi', [int]$Frames = 14)
$root = 'D:\Desktop\Black Ops 2 Android\BlackOps64'
$l = "$root\build\run\$Run\main\console_mp.log"
$pdb = "$root\$Build\Release\BO1Zombies.pdb"
if (Test-Path $pdb) { Copy-Item $pdb "$root\$Build\Release\%%%BO1Zombies.pdb%%%" -Force }
$crash = Get-Content $l | Where-Object { $_ -match '^BO1_HEADLESS CRASH' } | Select-Object -Last 1
if (-not $crash) { 'no crash line'; Get-Content $l | Where-Object { $_ -match 'Sys_Error|Com_ERROR|BO1_HEADLESS ASSERT' } | Select-Object -Last 3; return }
$st = (Get-Content $l | Where-Object { $_ -match '^BO1_HEADLESS STACK:' } | Select-Object -Last 1) -replace '^BO1_HEADLESS STACK:\s*', ''
$at = [regex]::Match($crash, 'BO1Zombies\.exe\+0x([0-9A-Fa-f]+)').Groups[1].Value
$fr = @($at) + (($st -split '\s+' | Where-Object { $_ -match '^\+0x' } | ForEach-Object { $_.Substring(3) }) | Select-Object -First $Frames)
$tmp = "$env:TEMP\rva_in.txt"
($fr | Where-Object { $_ -match '^[0-9a-fA-F]+$' } | ForEach-Object { '0x' + $_ }) | Set-Content $tmp -Encoding ascii
$sym = 'C:\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\llvm-symbolizer.exe'
$out = @(cmd /c "`"$sym`" --obj=`"$root\$Build\Release\BO1Zombies.exe`" --relative-address --no-inlines < `"$tmp`"" 2>&1 | Where-Object { $_ -ne '' })
($crash -replace 'thread \d+', '')
for ($i = 0; $i -lt $fr.Count; $i++) { '  #{0} +0x{1}  {2}' -f $i, $fr[$i], $out[2 * $i] }
