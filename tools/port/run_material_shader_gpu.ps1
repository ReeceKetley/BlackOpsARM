param(
    [string]$NamePrefix = 'shader57_gpu',
    [Parameter(Mandatory)][string]$PackageFile,
    [string]$VulkanSdk = 'C:\VulkanSDK\1.4.357.0'
)
$ErrorActionPreference = 'Stop'
if ($NamePrefix -notmatch '^[a-zA-Z0-9_-]+$') { throw 'Invalid test prefix' }
$root = (Resolve-Path "$PSScriptRoot/../..").Path
Set-Location -LiteralPath $root
$package = Get-Content -LiteralPath $PackageFile -Raw | ConvertFrom-Json
if ((Get-FileHash -LiteralPath $package.artifactPath -Algorithm SHA256).Hash -ne $package.dllSha256) { throw 'DXVK artifact checksum mismatch' }
$results = @()
foreach ($spec in @(@('x86','native'),@('x64','native'),@('x64','dxvk'))) {
    $arch,$backend = $spec
    $name = "${NamePrefix}_${arch}_$backend"
    $run = Join-Path "$root/build/run" $name
    if (Test-Path -LiteralPath $run) { throw 'Use a fresh GPU test prefix' }
    New-Item -ItemType Directory -Path $run | Out-Null
    Copy-Item -LiteralPath "build/run/shader57_tests/$arch/test_material_shader_gpu.exe" -Destination "$run/shader_check.exe"
    $compilerDll = "build/run/shader57_tests/$arch/d3dx9_43.dll"
    if (!(Test-Path -LiteralPath $compilerDll)) {
        $systemDir = if ($arch -eq 'x86') { 'SysWOW64' } else { 'System32' }
        $compilerDll = Join-Path "$env:WINDIR/$systemDir" 'd3dx9_43.dll'
    }
    if (!(Test-Path -LiteralPath $compilerDll)) { throw "Install the matching $arch D3DX9_43 runtime or stage its DLL alongside the fixture" }
    Copy-Item -LiteralPath $compilerDll -Destination "$run/d3dx9_43.dll"
    if ($backend -eq 'dxvk') { Copy-Item -LiteralPath $package.artifactPath -Destination "$run/d3d9.dll" }
    $envValues = @{
        VK_INSTANCE_LAYERS = 'VK_LAYER_KHRONOS_validation'
        VK_LAYER_PATH = "$VulkanSdk/Bin"
        VK_LOADER_LAYERS_DISABLE = '~implicit~'
        DXVK_LOG_PATH = $run
        DXVK_LOG_LEVEL = 'info'
        DXVK_DEBUG = 'validation'
    }
    $previous = @{}
    try {
        foreach ($key in $envValues.Keys) { $previous[$key] = [Environment]::GetEnvironmentVariable($key,'Process');[Environment]::SetEnvironmentVariable($key,$envValues[$key],'Process') }
        $p = Start-Process -FilePath "$run/shader_check.exe" -ArgumentList 'gpu' -WorkingDirectory $run -WindowStyle Hidden -RedirectStandardOutput "$run/stdout.log" -RedirectStandardError "$run/stderr.log" -PassThru
    } finally {
        foreach ($key in $previous.Keys) { [Environment]::SetEnvironmentVariable($key,$previous[$key],'Process') }
    }
    $null = $p.Handle
    $watch = [Diagnostics.Stopwatch]::StartNew()
    while (!$p.WaitForExit(1000)) {
        if ($watch.Elapsed.TotalSeconds -gt 60) {
            if ($p.Path -ne "$run/shader_check.exe".Replace('/','\')) { throw 'Unexpected GPU test process path' }
            $p.Kill();$p.WaitForExit();throw 'GPU shader test timed out'
        }
    }
    $p.WaitForExit();$p.Refresh()
    $stdout = Get-Content -LiteralPath "$run/stdout.log" -Raw
    $stderr = Get-Content -LiteralPath "$run/stderr.log" -Raw -ErrorAction SilentlyContinue
    $vkLog = "$run/shader_check_d3d9.log"
    $errors = @()
    if ($backend -eq 'dxvk') { $errors = @(Select-String -LiteralPath $vkLog -Pattern '^(err|error):' | ForEach-Object Line) }
    $validationEnabled = $backend -eq 'dxvk' -and (Select-String -LiteralPath $vkLog -Pattern '^info:    VK_LAYER_KHRONOS_validation$' -Quiet)
    $row = [ordered]@{name=$name; architecture=$arch; backend=$backend; exitCode=$p.ExitCode; stdout=$stdout; stderr=$stderr; vulkanErrors=$errors;
        validationLayerEnabled=$validationEnabled;
        exeSha256=(Get-FileHash -LiteralPath "$run/shader_check.exe" -Algorithm SHA256).Hash;
        d3dxSha256=(Get-FileHash -LiteralPath "$run/d3dx9_43.dll" -Algorithm SHA256).Hash}
    if ($backend -eq 'dxvk') { $row.dxvkSha256 = (Get-FileHash -LiteralPath "$run/d3d9.dll" -Algorithm SHA256).Hash }
    $row.loadedModules = @()
    foreach ($match in [regex]::Matches([string]$stdout, '(?m)^MODULE (\S+) (.+)\r?$')) {
        $modulePath = $match.Groups[2].Value.TrimEnd("`r")
        $hashPath = $modulePath
        # A Win32 child can report System32 while WOW64 loaded SysWOW64.
        $systemPrefix = Join-Path $env:WINDIR 'System32\'
        if ($arch -eq 'x86' -and $modulePath.StartsWith($systemPrefix, [StringComparison]::OrdinalIgnoreCase)) {
            $hashPath = Join-Path "$env:WINDIR/SysWOW64" $modulePath.Substring($systemPrefix.Length)
        }
        $row.loadedModules += [ordered]@{ name=$match.Groups[1].Value; path=$modulePath; hashPath=$hashPath;
            sha256=(Get-FileHash -LiteralPath $hashPath -Algorithm SHA256).Hash }
    }
    $row.modulesConfirmed = @($row.loadedModules | Where-Object { $_.name -in 'd3d9.dll','d3dx9_43.dll' }).Count -eq 2
    if ($backend -eq 'dxvk') {
        $row.modulesConfirmed = $row.modulesConfirmed -and
            @($row.loadedModules | Where-Object { $_.name -eq 'vulkan-1.dll' }).Count -eq 1 -and
            @($row.loadedModules | Where-Object { $_.name -eq 'd3d9.dll' -and $_.sha256 -eq $package.dllSha256 }).Count -eq 1
    }
    $results += $row
    $results | ConvertTo-Json -Depth 6 | Set-Content "build/run/${NamePrefix}_results.json"
    "GPU $arch/$backend exit=$($p.ExitCode) validationErrors=$($errors.Count)"
    $stdout

}

if (@($results | Where-Object { $_.exitCode -ne 0 -or $_.vulkanErrors.Count -or $_.stdout -notmatch 'PASS gpu:' -or !$_.modulesConfirmed -or ($_.backend -eq 'dxvk' -and !$_.validationLayerEnabled) }).Count) { throw 'GPU shader checks failed; all results retained' }
