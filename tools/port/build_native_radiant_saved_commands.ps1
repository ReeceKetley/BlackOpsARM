param([string]$Ndk='D:/Android/Sdk/ndk/30.0.16248370')
$ErrorActionPreference='Stop'
$taskRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$taskOut=Join-Path $taskRoot 'build-android-audit/native_registry_closure'
$taskCompiler=Join-Path $Ndk 'toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe'
$taskArgs=@('--target=aarch64-none-linux-android28',"--sysroot=$Ndk/toolchains/llvm/prebuilt/windows-x86_64/sysroot",'-D_DEBUG=1','-std=gnu++20','-fms-extensions','-fsigned-char','-ffunction-sections','-fdata-sections','-O0','-g','-fsanitize=undefined','-fno-sanitize-recover=all','-include',"$taskRoot/src/platform/win32_compat/msvc_compat.h","-I$taskRoot/src","-I$taskRoot","-I$taskRoot/../DXVK-Native/include/native/directx","-I$taskRoot/../DXVK-Native/include/native/windows")
$taskSources=@('tools/port/tests/native_radiant_saved_commands_tests.cpp','src/qcommon/radiant_remote.cpp','src/qcommon/dl_main.cpp','src/universal/q_shared.cpp','src/platform/android/assertion_services.cpp','src/platform/android/stack_trace.cpp','src/platform/android/fatal_error.cpp')
$taskArgs+=$taskSources|ForEach-Object{Join-Path $taskRoot $_}
$taskArgs+=@('-static-libstdc++','-Wl,--gc-sections','-o',"$taskOut/native-radiant-saved-commands")
& $taskCompiler @taskArgs *> "$taskOut/radiant-saved-command-build2.log"
if($LASTEXITCODE){Get-Content "$taskOut/radiant-saved-command-build2.log" -Tail 20;throw 'Radiant fixture build failed'}
Write-Output "$taskOut/native-radiant-saved-commands"
