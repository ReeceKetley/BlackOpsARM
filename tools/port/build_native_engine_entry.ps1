param([Parameter(Mandatory=$true)][string]$BuildDirectory,
      [switch]$FullDatabase,
      [string]$SdkRoot='D:/Android/Sdk')
$ErrorActionPreference='Stop'
$taskRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$taskBuild=[IO.Path]::GetFullPath((Join-Path $taskRoot $BuildDirectory))
if(!$taskBuild.StartsWith($taskRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Build must stay in Android worktree'}
if(Test-Path -LiteralPath $taskBuild){throw 'Use fresh output directory'}
New-Item -ItemType Directory -Path $taskBuild|Out-Null
$taskCmake=Join-Path $SdkRoot 'cmake/3.22.1/bin/cmake.exe'
$taskArgs=@('-S',(Join-Path $taskRoot 'android/cmake/native-engine-entry'),'-B',$taskBuild,'-G','Ninja',
    "-DCMAKE_MAKE_PROGRAM=$SdkRoot/cmake/3.22.1/bin/ninja.exe",
    "-DCMAKE_TOOLCHAIN_FILE=$SdkRoot/ndk/30.0.16248370/build/cmake/android.toolchain.cmake",
    '-DANDROID_ABI=arm64-v8a','-DANDROID_PLATFORM=android-28','-DCMAKE_BUILD_TYPE=Debug',
    '-DCMAKE_CXX_FLAGS_DEBUG=-O0 -g')
$taskArgs += ('-DBO1_NATIVE_FULL_DATABASE='+$(if($FullDatabase){'ON'}else{'OFF'}))
$taskArgs|ConvertTo-Json|Out-File (Join-Path $taskBuild 'configure-command.json') -Encoding utf8
$ErrorActionPreference='Continue'
& $taskCmake @taskArgs *> (Join-Path $taskBuild 'configure.log')
$taskConfigureExit=$LASTEXITCODE
$ErrorActionPreference='Stop'
if($taskConfigureExit){throw 'Engine entry configure failed; evidence retained'}
$ErrorActionPreference='Continue'
& $taskCmake --build $taskBuild --target bo1_native_engine_entry --verbose *> (Join-Path $taskBuild 'build.log')
$taskBuildExit=$LASTEXITCODE
$ErrorActionPreference='Stop'
@{configure_exit=$taskConfigureExit;build_exit=$taskBuildExit;completed=($taskBuildExit -eq 0)}|ConvertTo-Json|Out-File (Join-Path $taskBuild 'build-result.json') -Encoding utf8
if($taskBuildExit){Get-Content (Join-Path $taskBuild 'build.log') -Tail 35;throw 'Engine entry compile/link failed; evidence retained'}
