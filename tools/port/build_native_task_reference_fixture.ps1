param([string]$Ndk='D:/Android/Sdk/ndk/30.0.16248370')
$ErrorActionPreference='Stop'
$taskRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$taskOut=Join-Path $taskRoot 'build-android-audit/native_registry_closure'
$taskCompiler=Join-Path $Ndk 'toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe'
$taskArgs=@('--target=aarch64-none-linux-android28',"--sysroot=$Ndk/toolchains/llvm/prebuilt/windows-x86_64/sysroot",'-D_DEBUG=1','-std=gnu++20','-fms-extensions','-fsigned-char','-ffunction-sections','-fdata-sections','-O0','-g','-fsanitize=undefined','-fno-sanitize-recover=all','-include',"$taskRoot/src/platform/win32_compat/msvc_compat.h","-I$taskRoot/src","-I$taskRoot","-I$taskRoot/../DXVK-Native/include/native/directx","-I$taskRoot/../DXVK-Native/include/native/windows","$taskRoot/tools/port/tests/native_task_reference_tests.cpp","$taskRoot/src/win32/win_tasks.cpp",'-static-libstdc++','-Wl,--gc-sections','-o',"$taskOut/native-task-reference-tests")
& $taskCompiler @taskArgs *> "$taskOut/task-reference-build1.log"
if($LASTEXITCODE){Get-Content "$taskOut/task-reference-build1.log" -Tail 25;throw 'Task reference fixture failed to build'}
Write-Output "$taskOut/native-task-reference-tests"
