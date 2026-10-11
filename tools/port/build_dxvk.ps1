param(
    [string]$VcVars = 'C:\BuildTools\VC\Auxiliary\Build\vcvarsall.bat',
    [int]$Jobs = 4
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
Set-Location -LiteralPath $root
$source = "$root/build/deps/dxvk-src-3.1.1"
$output = "$root/build/deps/dxvk-win64-patched"
$runtime = "$root/build/deps/dxvk-tools"
$pin = 'b1a1c99ab52b687cf950d62c88bc2fa316b41663'
$patch = Join-Path $PSScriptRoot 'dxvk-patches/free-input-copy-pipelines.patch'
New-Item -ItemType Directory -Path "$root/build/deps" -Force | Out-Null
if (-not (Test-Path -LiteralPath $source)) {
    & git clone --depth 1 --branch v3.1.1 --recurse-submodules --shallow-submodules https://github.com/doitsujin/dxvk.git $source
    if ($LASTEXITCODE -ne 0) { throw 'DXVK clone failed' }
}
if ((& git -C $source rev-parse HEAD) -ne $pin) { throw 'DXVK source does not match the pinned release' }
& git -C $source submodule update --init --recursive --depth 1
if ($LASTEXITCODE -ne 0) { throw 'DXVK submodule checkout failed' }
& git -C $source apply --check $patch 2>$null
if ($LASTEXITCODE -eq 0) {
    & git -C $source apply $patch
    if ($LASTEXITCODE -ne 0) { throw 'DXVK lifecycle patch failed' }
} else {
    & git -C $source apply --reverse --check $patch
    if ($LASTEXITCODE -ne 0) { throw 'DXVK source is neither pristine nor already patched' }
}
$changed = @(& git -C $source diff --name-only)
if ($changed.Count -ne 1 -or $changed[0] -ne 'src/dxvk/dxvk_meta_copy.cpp') { throw 'Unexpected DXVK source edits' }
$actualPatch = ((& git -C $source diff -- src/dxvk/dxvk_meta_copy.cpp) -join "`n").TrimEnd()
$expectedPatch = (Get-Content -LiteralPath $patch -Raw).Replace("`r`n", "`n").TrimEnd()
if ($actualPatch -ne $expectedPatch) { throw 'DXVK source diff differs from the recorded patch' }
if (-not (Test-Path "$runtime/Scripts/python.exe")) {
    & python -m venv $runtime
    if ($LASTEXITCODE -ne 0) { throw 'DXVK tool environment creation failed' }
}
& "$runtime/Scripts/python.exe" -m pip install meson==1.12.1 > "$root/build/deps/dxvk-tools-install.log" 2>&1
if ($LASTEXITCODE -ne 0) { throw 'DXVK build tool install failed' }
$python = "$runtime/Scripts/python.exe"
$prefix = "`"$VcVars`" x64 >nul && `"$python`" -m mesonbuild.mesonmain"
$reconfigure = if (Test-Path "$output/meson-private/coredata.dat") { '--reconfigure' } else { '' }
& cmd /d /s /c "$prefix setup $reconfigure `"$output`" `"$source`" --buildtype=release -Denable_d3d8=false -Denable_d3d10=false -Denable_d3d11=false -Denable_dxgi=false" > "$root/build/deps/dxvk-configure.log" 2>&1
if ($LASTEXITCODE -ne 0) { Get-Content "$root/build/deps/dxvk-configure.log" -Tail 20; throw 'DXVK configure failed' }
& cmd /d /s /c "$prefix compile -C `"$output`" -j $Jobs" > "$root/build/deps/dxvk-build.log" 2>&1
if ($LASTEXITCODE -ne 0) { Get-Content "$root/build/deps/dxvk-build.log" -Tail 25; throw 'DXVK compile failed' }
$dll = "$output/src/d3d9/d3d9.dll"
if (-not (Test-Path -LiteralPath $dll)) { throw 'DXVK did not produce D3D9' }
$manifest = [ordered]@{
    version = '3.1.1 + BO1 input-copy cleanup'
    architecture = 'x64'; api = 'd3d9'
    source = 'https://github.com/doitsujin/dxvk/tree/v3.1.1'
    sourceCommit = $pin
    submodules = @(& git -C $source submodule status --recursive)
    patchSha256 = (Get-FileHash -LiteralPath $patch).Hash
    artifactPath = $dll
    dllSha256 = (Get-FileHash -LiteralPath $dll).Hash
    tools = @{meson='1.12.1'; ninja=(Select-String -Path "$root/build/deps/dxvk-configure.log" -Pattern '^Found ninja-' | ForEach-Object Line); vcvars=$VcVars}
    compilers = (Get-Content "$output/meson-info/intro-compilers.json" -Raw | ConvertFrom-Json)
    builtUtc = [DateTime]::UtcNow.ToString('o')
}
$manifest | ConvertTo-Json -Depth 8 | Set-Content "$output/package.json" -Encoding UTF8
"Built DXVK D3D9; manifest: $output/package.json"
