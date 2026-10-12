param([string]$SdkRoot='D:/Android/Sdk',[string]$VcVars='C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat',[Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$sortRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$sortOutput=[IO.Path]::GetFullPath((Join-Path $sortRoot $OutputDirectory))
if(!$sortOutput.StartsWith((Join-Path $sortRoot 'build-android-audit')+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Use fresh ignored Android scratch'}
if(Test-Path -LiteralPath $sortOutput){throw 'Use fresh output directory'}
New-Item -ItemType Directory -Path $sortOutput|Out-Null
$sortSource=Join-Path $sortRoot 'tools/port/tests/stringtable_crt_sort_probe.cpp'
foreach($sortArch in @('x86','x64')){
    $sortArchOutput=Join-Path $sortOutput $sortArch
    New-Item -ItemType Directory -Path $sortArchOutput|Out-Null
    $sortCommand='"'+$VcVars+'" '+$sortArch+' >nul && cl /nologo /W4 /std:c++20 /EHsc /O2 /MTd /D_DEBUG=1 "'+$sortSource+'" /Fe"'+(Join-Path $sortArchOutput 'crt_sort.exe')+'" /Fo"'+$sortArchOutput+'/"'
    $sortCommand|Out-File (Join-Path $sortArchOutput 'command.txt') -Encoding utf8
    Push-Location $sortArchOutput
    try{& cmd /d /s /c $sortCommand 2>&1|Out-File 'compile.log' -Encoding utf8;if($LASTEXITCODE){throw "$sortArch standalone CRT compile failed"}}
    finally{Pop-Location}
}
$sortAndroidOutput=Join-Path $sortOutput 'arm64'
New-Item -ItemType Directory -Path $sortAndroidOutput|Out-Null
$sortClang=Join-Path $SdkRoot 'ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin/aarch64-linux-android28-clang++.cmd'
$sortArgs=@('-std=c++20','-Wall','-Wextra','-Werror','-O3','-g','-UNDEBUG','-fsanitize=undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-static-libstdc++',$sortSource,'-o',(Join-Path $sortAndroidOutput 'crt_sort'))
@{compiler=$sortClang;argv=$sortArgs;android_api=28;abi='arm64-v8a';assertions=$true;ubsan=$true;optimization='O3'}|ConvertTo-Json|Out-File (Join-Path $sortAndroidOutput 'command.json') -Encoding utf8
& $sortClang @sortArgs 2>&1|Out-File (Join-Path $sortAndroidOutput 'compile.log') -Encoding utf8
if($LASTEXITCODE){throw 'ARM64 standalone CRT compile failed'}
Write-Output "Standalone CRT research binaries: $sortOutput"
