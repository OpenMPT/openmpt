@echo off

set MPT_VS_VER=%1
set MPT_VS_TARGET=%2
set MPT_VS_ARCH=%3
set MPT_VS_CONF=%4

if "%MPT_VS_VER%" == "" goto setupargumentserror
if "%MPT_VS_TARGET%" == "" goto setupargumentserror
if "%MPT_VS_ARCH%" == "" goto setupargumentserror
if "%MPT_VS_CONF%" == "" goto setupargumentserror

goto setupargumentsstart

:setupargumentserror
echo "Usage: foo.cmd vs2015 winstore82 Win32 Release"
exit 1

:setupargumentsstart
