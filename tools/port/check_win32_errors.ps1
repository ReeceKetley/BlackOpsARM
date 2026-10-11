param([string]$VcVars = 'C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat')
$ErrorActionPreference = 'Stop'
$testRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$testOutput = Join-Path $testRoot 'build-android-audit/host-errors'
if (Test-Path -LiteralPath $testOutput) { throw 'Test evidence exists; retain it and choose a fresh output manually.' }
New-Item -ItemType Directory -Path $testOutput | Out-Null
$testCommand = '"' + $VcVars + '" x64 >nul && cl /nologo /W4 /WX /std:c++20 /EHsc /O2 /DBO1_WIN32_COMPAT_HOST_TEST=1 /DBO1_ERROR_TEST_MAIN=1 /I"' + (Join-Path $testRoot 'src') + '" "' + (Join-Path $testRoot 'tools/port/tests/win32_error_tests.cpp') + '" "' + (Join-Path $testRoot 'src/platform/win32_compat/win32_errors.cpp') + '" /Fe"' + (Join-Path $testOutput 'win32_error_tests.exe') + '" /Fo"' + $testOutput + '\\"'
& cmd /d /s /c $testCommand 2>&1 | Out-File -LiteralPath (Join-Path $testOutput 'compile.log') -Encoding utf8
if ($LASTEXITCODE) { Get-Content (Join-Path $testOutput 'compile.log'); throw 'Host TLS test compile failed' }
& (Join-Path $testOutput 'win32_error_tests.exe') 2>&1 | Tee-Object -FilePath (Join-Path $testOutput 'test.log')
if ($LASTEXITCODE) { throw 'Host TLS test failed' }
