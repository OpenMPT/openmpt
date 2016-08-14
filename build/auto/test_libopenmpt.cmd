@echo off

set BATCH_DIR=%~dp0
cd %BATCH_DIR%
cd ..\..

set MY_DIR=%CD%

set MPT_ARCH=%1
set MPT_VSVER=%2
set MPT_BIN_TARGET=%3

if "%MPT_ARCH%" == "" goto usage
if "%MPT_ARCH%" == "Win32" set MPT_BIN_ARCH=x86-32
if "%MPT_ARCH%" == "x64"   set MPT_BIN_ARCH=x86-64



"bin\release\%MPT_VSVER%-static\%MPT_BIN_ARCH%-%MPT_BIN_TARGET%\libopenmpt_test.exe" || goto error



goto noerror

:usage
echo "Usage: foo.cmd [Win32|x64]"
:error
cd "%MY_DIR%"
exit 1

:noerror
cd "%MY_DIR%"
exit 0
