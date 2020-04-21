@echo off

set MPT_VS_VER=%1
set MPT_VS_TARGET=%2
set MPT_VS_ARCH=%3
set MPT_VS_CONF=%4
set MPT_PKG_FORMAT=%5
set MPT_VS_FLAVOUR=%6

if "%MPT_VS_VER%" == "" goto setupargumentserror
if "%MPT_VS_TARGET%" == "" goto setupargumentserror
if "%MPT_VS_ARCH%" == "" goto setupargumentserror
if "%MPT_VS_CONF%" == "" goto setupargumentserror
if "%MPT_PKG_FORMAT%" == "" goto setupargumentserror

goto setupargumentsstart

:setupargumentserror
echo "Usage: foo.cmd vs2019 winstore82 Win32 Release 7z default"
exit 1

:setupargumentsstart
