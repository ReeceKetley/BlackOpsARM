param([string]$Dir = 'build-x64-hi', [string]$Log = '', [switch]$Rebuild, [int]$Jobs = 4)
$sd = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location 'D:\Desktop\Black Ops 2 Android\BlackOps64'
if (-not $Log) { $Log = "$sd\inc_$Dir.log" }
[string[]]$t = @(); if ($Rebuild) { $t = @('/t:Rebuild') }
$sw = [Diagnostics.Stopwatch]::StartNew()
$previousClSuffix = $env:_CL_
try {
    # _CL_ appends compiler options after the project's /MP setting.
    $env:_CL_ = "$previousClSuffix /MP$Jobs"
    cmake --build $Dir --config Release -- /m:2 /v:m @t > $Log 2>&1
    $buildExitCode = $LASTEXITCODE
} finally {
    $env:_CL_ = $previousClSuffix
}
$errs = Select-String -Path $Log -Pattern 'error C[0-9]+|error LNK|fatal error' | Select-Object -First 8
"$Dir built in $([int]$sw.Elapsed.TotalSeconds)s; errors: $((Select-String -Path $Log -Pattern 'error C[0-9]+|error LNK|fatal error').Count)"
$errs | ForEach-Object { $_.Line.Substring(0, [Math]::Min(230, $_.Line.Length)) }
$w = @(Select-String -Path $Log -Pattern 'warning C43(11|12|02)' | ForEach-Object {
    if ($_.Line -match '([A-Z]:\\[^\r\n]+?)\((\d+),\d+\): warning C43') { "$($Matches[1]):$($Matches[2])" }
} | Sort-Object -Unique)
"truncation warnings in recompiled files: $($w.Count)"
if ($buildExitCode -ne 0) { throw "Build $Dir failed with exit code $buildExitCode; see $Log" }
