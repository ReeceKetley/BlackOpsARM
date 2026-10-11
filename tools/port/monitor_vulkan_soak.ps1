param(
    [Parameter(Mandatory)][ValidatePattern('^[a-zA-Z0-9_-]+$')][string]$Name,
    [int]$IntervalSeconds = 30,
    [int]$MaxMinutes = 50
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$run = Join-Path "$root/build/run" $Name
$gamePath = Join-Path $run 'BO1Zombies.exe'
$gamePid = [int](Get-Content -LiteralPath "$run/pid.txt" -Raw)
$output = Join-Path $run 'soak-samples.jsonl'
if (Test-Path -LiteralPath $output) { throw 'Keep existing samples; use a fresh monitor output/run' }
$watch = [Diagnostics.Stopwatch]::StartNew()
function LastLine([string[]]$Lines, [string]$Pattern) {
    @($Lines | Where-Object { $_ -match $Pattern } | Select-Object -Last 1) -join ''
}
while ($watch.Elapsed.TotalMinutes -lt $MaxMinutes) {
    $process = Get-Process -Id $gamePid -ErrorAction SilentlyContinue
    if (!$process) { break }
    if ($process.Path -ne $gamePath) { throw 'Process identity changed; refusing to sample another application' }
    $lines = @(Get-Content -LiteralPath "$run/main/console_mp.log" -Tail 1500 -ErrorAction SilentlyContinue)
    $sample = [ordered]@{
        observedUtc = [DateTime]::UtcNow.ToString('o')
        processId = $gamePid
        processStartedUtc = $process.StartTime.ToUniversalTime().ToString('o')
        elapsedMs = [long]$watch.ElapsedMilliseconds
        privateBytes = $process.PrivateMemorySize64
        workingSetBytes = $process.WorkingSet64
        handleCount = $process.HandleCount
        threadCount = $process.Threads.Count
        client = LastLine $lines '^bo1_testclient:'
        actors = LastLine $lines '^bo1_actors:'
        fight = LastLine $lines '^bo1_fight:'
        roundNotify = LastLine $lines '^bo1_feature:.* level notify .* round '
        plan = LastLine $lines '^bo1_plan:.*(end (recovery )?(PASS|FAIL)|complete,|quit)'
    }
    [IO.File]::AppendAllText($output, ($sample | ConvertTo-Json -Compress) + "`n")
    Start-Sleep -Seconds $IntervalSeconds
}
[IO.File]::WriteAllText((Join-Path $run 'soak-monitor-ended.txt'), [DateTime]::UtcNow.ToString('o'))
"monitor ended for exact pid $gamePid; samples in $output"
