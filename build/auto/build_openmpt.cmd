@echo off

set BATCH_DIR=%~dp0
cd %BATCH_DIR%
cd ..\..

set MY_DIR=%CD%

set MPT_VSVER=%1
set MPT_ARCH=%2
set MPT_ARCH2=%3

if "%MPT_VSVER%" == "" goto usage
if "%MPT_ARCH%" == "" goto usage
if "%MPT_ARCH2%" == "" goto usage

call "build\auto\setup_%MPT_VSVER%.cmd"



cd "build\%MPT_VSVER%" || goto error

devenv OpenMPT.sln /clean "Release|%MPT_ARCH%" || goto error
devenv PluginBridge.sln /clean "Release|%MPT_ARCH2%" || goto error

devenv OpenMPT.sln /build "Release|%MPT_ARCH%" || goto error
devenv PluginBridge.sln /build "Release|%MPT_ARCH2%" || goto error

cd ..\.. || goto error



goto noerror

:usage
echo "Usage: foo.cmd vs2010 [Win32|x64] [Win32|x64]"
:error
cd "%MY_DIR%"
exit 1

:noerror
cd "%MY_DIR%"
exit 0
