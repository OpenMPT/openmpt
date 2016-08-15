@echo off

set %MPT_VS_VER%=%1
set %MPT_VS_TARGET%=%2
set %MPT_VS_ARCH%=%3
set %MPT_VS_CONF%=%4

if "%MPT_VS_VER%" == "" goto setupargumentserror
if "%MPT_VS_TARGET%" == "" goto setupargumentserror
if "%MPT_VS_ARCH%" == "" goto setupargumentserror
if "%MPT_VS_CONF%" == "" goto setupargumentserror

goto setupargumentsstart

:setupargumentserror
echo "Usage: foo.cmd vs2010 xp Win32 Release"
rem vs2010     xp            Win32       Release     x86-32       winxp          release      static
rem MPT_VS_VER MPT_VS_TARGET MPT_VS_ARCH MPT_VS_CONF MPT_BIN_ARCH MPT_BIN_TARGET MPT_BIN_CONF MPT_BIN_RUNTIME
exit 1

:setupargumentsstart


if "%MPT_VS_TARGET%" == "w2k"     set MPT_VS_WITHTARGET=%MPT_VS_VER%w2k
if "%MPT_VS_TARGET%" == "xp"      set MPT_VS_WITHTARGET=%MPT_VS_VER%xp
if "%MPT_VS_TARGET%" == "vista"   set MPT_VS_WITHTARGET=%MPT_VS_VER%
if "%MPT_VS_TARGET%" == "default" set MPT_VS_WITHTARGET=%MPT_VS_VER%

if "%MPT_VS_ARCH%" == "Win32" set MPT_VS_ARCH_OTHER=x64
if "%MPT_VS_ARCH%" == "x64"   set MPT_VS_ARCH_OTHER=Win32


if "%MPT_VS_TARGET%" == "w2k"     set MPT_BIN_TARGET=win2000
if "%MPT_VS_TARGET%" == "xp"      set MPT_BIN_TARGET=winxp
if "%MPT_VS_TARGET%" == "vista"   set MPT_BIN_TARGET=vista
if "%MPT_VS_TARGET%" == "default" set MPT_BIN_TARGET=win7

if "%MPT_VS_ARCH%" == "Win32" set MPT_BIN_ARCH=x86-32
if "%MPT_VS_ARCH%" == "x64"   set MPT_BIN_ARCH=x86-64

if "%MPT_VS_CONF%" == "Release"       set MPT_BIN_CONF=release
if "%MPT_VS_CONF%" == "ReleaseShared" set MPT_BIN_CONF=release
if "%MPT_VS_CONF%" == "ReleaseLTCG"   set MPT_BIN_CONF=release-LTCG

if "%MPT_VS_CONF%" == "Release"       set MPT_BIN_RUNTIME=static
if "%MPT_VS_CONF%" == "ReleaseShared" set MPT_BIN_RUNTIME=shared
if "%MPT_VS_CONF%" == "ReleaseLTCG"   set MPT_BIN_RUNTIME=static


if "%MPT_VS_ARCH%" == "Win32" set MPT_DIST_VARIANT_PREFIX=win32
if "%MPT_VS_ARCH%" == "x64"   set MPT_DIST_VARIANT_PREFIX=win64

if "%MPT_VS_TARGET%" == "w2k"     set MPT_DIST_VARIANT_SUFFIX=old
if "%MPT_VS_TARGET%" == "xp"      set MPT_DIST_VARIANT_SUFFIX=old
if "%MPT_VS_TARGET%" == "vista"   set MPT_DIST_VARIANT_SUFFIX=
if "%MPT_VS_TARGET%" == "default" set MPT_DIST_VARIANT_SUFFIX=

set MPT_DIST_VARIANT=%MPT_DIST_VARIANT_PREFIX%%MPT_DIST_VARIANT_SUFFIX%
