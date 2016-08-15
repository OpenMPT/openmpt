@echo off

set BATCH_DIR=%~dp0
cd %BATCH_DIR%
cd ..\..

set MY_DIR=%CD%

set MPT_VSVER=%1
set MPT_ARCH=%2
set MPT_ARCH2=%3
set MPT_CONFIG=%4
set MPT_TARGET=%5

if "%MPT_VSVER%" == "" goto usage
if "%MPT_ARCH%" == "" goto usage
if "%MPT_ARCH2%" == "" goto usage
if "%MPT_CONFIG%" == "" set MPT_CONFIG=Release
rem if "%MPT_TARGET%" == "" set MPT_TARGET=""

call "build\auto\setup_%MPT_VSVER%.cmd"



cd "build\%MPT_VSVER%%MPT_TARGET%" || goto error

devenv OpenMPT.sln /clean "%MPT_CONFIG%|%MPT_ARCH%" || goto error
devenv PluginBridge.sln /clean "%MPT_CONFIG%|%MPT_ARCH2%" || goto error

devenv OpenMPT.sln /build "%MPT_CONFIG%|%MPT_ARCH%" || goto error
devenv PluginBridge.sln /build "%MPT_CONFIG%|%MPT_ARCH2%" || goto error

cd ..\.. || goto error



goto noerror

:usage
echo "Usage: foo.cmd vs2010 [Win32|x64] [Win32|x64] [Release]"
:error
cd "%MY_DIR%"
exit 1

:noerror
cd "%MY_DIR%"
exit 0
