param([string]$Plan = 'qr wall_rottweil fire3', [int]$Quit = 60000, [string]$Name = 'x64hi_t', [string]$Build = 'build-x64-hi', [switch]$NoBuild, [string]$Extra = '')
$sd = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $NoBuild) {
    $b = & "$sd\bld.ps1" -Dir $Build
    $b | Select-Object -First 6
    if ($b -match 'errors: [1-9]') { return }
}
$l = "D:\Desktop\Black Ops 2 Android\BlackOps64\build\run\$Name\main\console_mp.log"
& "$sd\hrun.ps1" -Name $Name -Build $Build -God -AutoquitMs $Quit -QuitDone 3000 -Plan $Plan -Extra $Extra -Wait
$pass = (Select-String -Path $l -Pattern 'bo1_plan:.* PASS' -ErrorAction SilentlyContinue | Measure-Object).Count
$fail = (Select-String -Path $l -Pattern 'bo1_plan:.* FAIL' -ErrorAction SilentlyContinue | Measure-Object).Count
"plan PASS $pass FAIL $fail"
& "$sd\symz.ps1" -Run $Name -Build $Build
Select-String -Path $l -Pattern 'script runtime error|Bo1_NarrowFail|HEADLESS ASSERT' -ErrorAction SilentlyContinue | Select-Object -First 3 | ForEach-Object { $_.Line.Substring(0, [Math]::Min(200, $_.Line.Length)) }
