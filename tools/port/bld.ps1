param([string]$Dir = 'build-x64-hi', [string]$Log = '', [switch]$Rebuild)
$sd = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location 'D:\Desktop\Black Ops 2 Android\BlackOps64'
if (-not $Log) { $Log = "$sd\inc_$Dir.log" }
[string[]]$t = @(); if ($Rebuild) { $t = @('/t:Rebuild') }
$sw = [Diagnostics.Stopwatch]::StartNew()
cmake --build $Dir --config Release -- /m /v:n @t > $Log 2>&1
$errs = Select-String -Path $Log -Pattern 'error C[0-9]+|error LNK|fatal error' | Select-Object -First 8
"$Dir built in $([int]$sw.Elapsed.TotalSeconds)s; errors: $((Select-String -Path $Log -Pattern 'error C[0-9]+|error LNK|fatal error').Count)"
$errs | ForEach-Object { $_.Line.Substring(0, [Math]::Min(230, $_.Line.Length)) }
$w = Select-String -Path $Log -Pattern 'warning C43(11|12|02)' | ForEach-Object { ($_.Line -replace ' \[.*$', '') } | Sort-Object -Unique
"truncation warnings in recompiled files: $($w.Count)"
