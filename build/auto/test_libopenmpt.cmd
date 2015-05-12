@echo off

set BATCH_DIR=%~dp0
cd %BATCH_DIR%
cd ..\..

set MY_DIR=%CD%

set MPT_ARCH=%1

if "%MPT_ARCH%" == "" goto usage



"bin\%MPT_ARCH%\libopenmpt_test.exe" || goto error



goto noerror

:usage
echo "Usage: foo.cmd [Win32|x64]"
:error
cd "%MY_DIR%"
exit 1

:noerror
cd "%MY_DIR%"
exit 0
