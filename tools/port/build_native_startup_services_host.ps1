param([Parameter(Mandatory=$true)][string]$OutputDirectory,[switch]$OriginalWindowsPolicy,[string]$VcVars='C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat')
$ErrorActionPreference='Stop'
$taskRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$taskOutput=[IO.Path]::GetFullPath((Join-Path $taskRoot $OutputDirectory))
if(!$taskOutput.StartsWith($taskRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Output must stay in Android worktree'}
if(Test-Path -LiteralPath $taskOutput){throw 'Use fresh output directory'}
New-Item -ItemType Directory -Path $taskOutput|Out-Null
$taskFiles=@('src/universal/dvar.cpp','src/qcommon/cmd.cpp','src/qcommon/dvar_cmds.cpp','src/qcommon/common.cpp','src/client/cl_keys.cpp','src/universal/q_parse.cpp','src/universal/q_shared.cpp','src/universal/com_memory.cpp','src/universal/com_math.cpp','src/universal/com_shared.cpp','src/universal/com_hash.cpp','src/clientscript/cscr_stringlist.cpp','src/clientscript/cscr_memorytree.cpp','src/clientscript/cscr_instance.cpp','src/clientscript/cscr_debugger.cpp','src/platform/android/script_pool_bridge.cpp','src/platform/android/script_string_services.cpp','src/platform/android/thread_services.cpp','src/platform/android/legacy_crt.cpp','src/platform/android/fast_critical_section_services.cpp')
$taskHeaders=@('src/platform/startup_services_api.h','src/platform/android/legacy_crt.h','src/qcommon/cmd.h','src/universal/thread_data.h','src/qcommon/cmd_args.h','src/qcommon/thread_context.h','src/qcommon/trace_thread_info.h','src/universal/dvar.h','src/universal/q_parse.h','src/qcommon/dvar_cmds.h','src/clientscript/cscr_debugger.h','src/platform/fast_critical_section.h','android/cmake/native-startup-services/CMakeLists.txt')
foreach($taskArch in @('x86','x64')){
    $taskArchOutput=Join-Path $taskOutput $taskArch;New-Item -ItemType Directory -Path $taskArchOutput|Out-Null
    $taskHashes=@{}
    foreach($unit in ($taskFiles+$taskHeaders+@('src/database/db_registry.cpp','tools/port/tests/native_startup_services_tests.cpp','tools/port/build_native_startup_services_host.ps1'))){$taskHashes[$unit]=(Get-FileHash -LiteralPath (Join-Path $taskRoot $unit) -Algorithm SHA256).Hash}
    $taskHashes|ConvertTo-Json|Out-File (Join-Path $taskArchOutput 'source-manifest.json') -Encoding utf8
    $taskFlags='/D_DEBUG=1 /DBO1_STARTUP_SERVICES=1 /DBO1_STRING_POOL_GATE=1 /DBO1_SCRIPT_STRING_SERVICES_HOST_TEST=1 /DBO1_FAST_CRIT_SECT_NATIVE_HOST_TEST=1 /DBO1_NATIVE_THREAD_SERVICES_HOST_TEST=1'
    if(!$OriginalWindowsPolicy){$taskFlags+=' /DBO1_STARTUP_SERVICES_NATIVE_HOST_TEST=1'}
    if($taskArch -eq 'x64'){$taskFlags+=' /DBO1_PORT_X64=1'}
    $taskCommon='/nologo /W4 /std:c++20 /EHsc /MTd /O2 /Gy /I"'+(Join-Path $taskRoot 'src')+'" '
    $taskWriterCommand='"'+$VcVars+'" '+$taskArch+' >nul && cl '+$taskCommon+' /c /DBO1_FAST_CRIT_SECT_GATE=1 /DBO1_FAST_CRIT_SECT_NATIVE_HOST_TEST=1 "'+(Join-Path $taskRoot 'src/database/db_registry.cpp')+'" /Fo"'+(Join-Path $taskArchOutput 'startup_writer.obj')+'"'
    $taskWriterCommand|Out-File (Join-Path $taskArchOutput 'writer-command.txt') -Encoding utf8
    & cmd /d /s /c $taskWriterCommand *> (Join-Path $taskArchOutput 'writer-compile.log')
    if($LASTEXITCODE){throw 'Actual startup writer compilation failed'}
    $taskSources=(($taskFiles+@('tools/port/tests/native_startup_services_tests.cpp'))|ForEach-Object{'"'+(Join-Path $taskRoot $_)+'"'}) -join ' '
    $taskCommand='"'+$VcVars+'" '+$taskArch+' >nul && cl '+$taskCommon+$taskFlags+' '+$taskSources+' /Fe"'+(Join-Path $taskArchOutput 'native_startup_services.exe')+'" /Fo"'+$taskArchOutput+'/" /link /OPT:REF "'+(Join-Path $taskArchOutput 'startup_writer.obj')+'"'
    $taskCommand|Out-File (Join-Path $taskArchOutput 'command.txt') -Encoding utf8
    & cmd /d /s /c $taskCommand *> (Join-Path $taskArchOutput 'compile.log')
    $taskExit=$LASTEXITCODE
    if(!$taskExit){
        $taskObjects=@((Join-Path $taskArchOutput 'startup_writer.obj'))
        foreach($unit in $taskFiles){$taskObjects+=Join-Path $taskArchOutput ([IO.Path]::GetFileNameWithoutExtension($unit)+'.obj')}
        $taskArchiveCommand='"'+$VcVars+'" '+$taskArch+' >nul && lib /nologo /out:"'+(Join-Path $taskArchOutput 'native_startup_services.lib')+'" '+(($taskObjects|ForEach-Object{'"'+$_+'"'}) -join ' ')
        $taskArchiveCommand|Out-File (Join-Path $taskArchOutput 'library-command.txt') -Encoding utf8
        & cmd /d /s /c $taskArchiveCommand *> (Join-Path $taskArchOutput 'library-build.log')
        $taskExit=$LASTEXITCODE
    }
    foreach($unit in $taskHashes.Keys){if((Get-FileHash -LiteralPath (Join-Path $taskRoot $unit) -Algorithm SHA256).Hash -ne $taskHashes[$unit]){throw 'Source changed during build'}}
    @{architecture=$taskArch;exit=$taskExit;completed=($taskExit -eq 0);original_windows_policy=[bool]$OriginalWindowsPolicy}|ConvertTo-Json|Out-File (Join-Path $taskArchOutput 'build-result.json') -Encoding utf8
    if($taskExit){Get-Content (Join-Path $taskArchOutput 'compile.log') -Tail 30;throw 'Startup services build failed'}
}
