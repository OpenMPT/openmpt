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
echo "Usage: foo.cmd vs2019 xp Win32 Release 7z default"
rem vs2019     winxp         Win32       Release     x86-32       winxp          release      static
rem MPT_VS_VER MPT_VS_TARGET MPT_VS_ARCH MPT_VS_CONF MPT_BIN_ARCH MPT_BIN_TARGET MPT_BIN_CONF MPT_BIN_RUNTIME
exit 1

:setupargumentsstart


if "%MPT_VS_TARGET%" == "winxp"   set MPT_VS_WITHTARGET=%MPT_VS_VER%winxp
if "%MPT_VS_TARGET%" == "vista"   set MPT_VS_WITHTARGET=%MPT_VS_VER%winvista
if "%MPT_VS_TARGET%" == "win7"    set MPT_VS_WITHTARGET=%MPT_VS_VER%win7
if "%MPT_VS_TARGET%" == "win81"   set MPT_VS_WITHTARGET=%MPT_VS_VER%win81
if "%MPT_VS_TARGET%" == "win10"   set MPT_VS_WITHTARGET=%MPT_VS_VER%win10
if "%MPT_VS_TARGET%" == "default" set MPT_VS_WITHTARGET=%MPT_VS_VER%win81
if "%MPT_VS_TARGET%" == "win10clang" set MPT_VS_WITHTARGET=%MPT_VS_VER%win10clang


if "%MPT_VS_ARCH%" == "Win32" (
	if "%MPT_VS_TARGET%" == "winxp"   set MPT_BIN_TARGET=winxp
	if "%MPT_VS_TARGET%" == "vista"   set MPT_BIN_TARGET=vista
	if "%MPT_VS_TARGET%" == "win7"    set MPT_BIN_TARGET=win7
	if "%MPT_VS_TARGET%" == "win81"   set MPT_BIN_TARGET=win81
	if "%MPT_VS_TARGET%" == "win10"   set MPT_BIN_TARGET=win10
	if "%MPT_VS_TARGET%" == "default" set MPT_BIN_TARGET=win81
	if "%MPT_VS_TARGET%" == "win10clang" set MPT_BIN_TARGET=win10clang
)
if "%MPT_VS_ARCH%" == "x64" (
	if "%MPT_VS_TARGET%" == "winxp"   set MPT_BIN_TARGET=winxp
	if "%MPT_VS_TARGET%" == "vista"   set MPT_BIN_TARGET=vista
	if "%MPT_VS_TARGET%" == "win7"    set MPT_BIN_TARGET=win7
	if "%MPT_VS_TARGET%" == "win81"   set MPT_BIN_TARGET=win81
	if "%MPT_VS_TARGET%" == "win10"   set MPT_BIN_TARGET=win10
	if "%MPT_VS_TARGET%" == "default" set MPT_BIN_TARGET=win81
	if "%MPT_VS_TARGET%" == "win10clang" set MPT_BIN_TARGET=win10clang
)
if "%MPT_VS_ARCH%" == "ARM" (
	if "%MPT_VS_TARGET%" == "winxp"   set MPT_BIN_TARGET=winxp
	if "%MPT_VS_TARGET%" == "vista"   set MPT_BIN_TARGET=vista
	if "%MPT_VS_TARGET%" == "win7"    set MPT_BIN_TARGET=win7
	if "%MPT_VS_TARGET%" == "win81"   set MPT_BIN_TARGET=win81
	if "%MPT_VS_TARGET%" == "win10"   set MPT_BIN_TARGET=win10
	if "%MPT_VS_TARGET%" == "default" set MPT_BIN_TARGET=win81
	if "%MPT_VS_TARGET%" == "win10clang" set MPT_BIN_TARGET=win10clang
)
if "%MPT_VS_ARCH%" == "ARM64" (
	if "%MPT_VS_TARGET%" == "winxp"   set MPT_BIN_TARGET=winxp
	if "%MPT_VS_TARGET%" == "vista"   set MPT_BIN_TARGET=vista
	if "%MPT_VS_TARGET%" == "win7"    set MPT_BIN_TARGET=win7
	if "%MPT_VS_TARGET%" == "win81"   set MPT_BIN_TARGET=win81
	if "%MPT_VS_TARGET%" == "win10"   set MPT_BIN_TARGET=win10
	if "%MPT_VS_TARGET%" == "default" set MPT_BIN_TARGET=win81
	if "%MPT_VS_TARGET%" == "win10clang" set MPT_BIN_TARGET=win10clang
)

if "%MPT_VS_TARGET%" == "winxp"   set MPT_BIN_TARGET32=winxp
if "%MPT_VS_TARGET%" == "vista"   set MPT_BIN_TARGET32=vista
if "%MPT_VS_TARGET%" == "win7"    set MPT_BIN_TARGET32=win7
if "%MPT_VS_TARGET%" == "win81"   set MPT_BIN_TARGET32=win81
if "%MPT_VS_TARGET%" == "win10"   set MPT_BIN_TARGET32=win10
if "%MPT_VS_TARGET%" == "default" set MPT_BIN_TARGET32=win81
if "%MPT_VS_TARGET%" == "win10clang" set MPT_BIN_TARGET32=win10clang

if "%MPT_VS_TARGET%" == "winxp"   set MPT_BIN_TARGET64=winxp
if "%MPT_VS_TARGET%" == "vista"   set MPT_BIN_TARGET64=vista
if "%MPT_VS_TARGET%" == "win7"    set MPT_BIN_TARGET64=win7
if "%MPT_VS_TARGET%" == "win81"   set MPT_BIN_TARGET64=win81
if "%MPT_VS_TARGET%" == "win10"   set MPT_BIN_TARGET64=win10
if "%MPT_VS_TARGET%" == "default" set MPT_BIN_TARGET64=win81
if "%MPT_VS_TARGET%" == "win10clang" set MPT_BIN_TARGET64=win10clang

if "%MPT_VS_ARCH%" == "Win32" set MPT_BIN_ARCH_TRK=x86
if "%MPT_VS_ARCH%" == "x64"   set MPT_BIN_ARCH_TRK=amd64
if "%MPT_VS_ARCH%" == "ARM"   set MPT_BIN_ARCH_TRK=arm
if "%MPT_VS_ARCH%" == "ARM64" set MPT_BIN_ARCH_TRK=arm64

if "%MPT_VS_ARCH%" == "Win32" set MPT_BIN_ARCH=x86
if "%MPT_VS_ARCH%" == "x64"   set MPT_BIN_ARCH=amd64
if "%MPT_VS_ARCH%" == "ARM"   set MPT_BIN_ARCH=arm
if "%MPT_VS_ARCH%" == "ARM64" set MPT_BIN_ARCH=arm64

if "%MPT_VS_CONF%" == "Release"       set MPT_BIN_CONF=release
if "%MPT_VS_CONF%" == "ReleaseShared" set MPT_BIN_CONF=release

if "%MPT_VS_CONF%" == "Release"       set MPT_BIN_RUNTIME=static
if "%MPT_VS_CONF%" == "ReleaseShared" set MPT_BIN_RUNTIME=shared


if "%MPT_VS_ARCH%" == "Win32" set MPT_DIST_VARIANT_ARCH=x86
if "%MPT_VS_ARCH%" == "x64"   set MPT_DIST_VARIANT_ARCH=amd64
if "%MPT_VS_ARCH%" == "ARM"   set MPT_DIST_VARIANT_ARCH=arm
if "%MPT_VS_ARCH%" == "ARM64" set MPT_DIST_VARIANT_ARCH=arm64

if "%MPT_VS_TARGET%" == "winxp"   set MPT_DIST_VARIANT_OS=winxp
if "%MPT_VS_TARGET%" == "vista"   set MPT_DIST_VARIANT_OS=winvista
if "%MPT_VS_TARGET%" == "win7"    set MPT_DIST_VARIANT_OS=win7
if "%MPT_VS_TARGET%" == "win81"   set MPT_DIST_VARIANT_OS=win81
if "%MPT_VS_TARGET%" == "win10"   set MPT_DIST_VARIANT_OS=win10
if "%MPT_VS_TARGET%" == "default" set MPT_DIST_VARIANT_OS=win81
if "%MPT_VS_TARGET%" == "win10clang" set MPT_DIST_VARIANT_OS=win10clang

set MPT_DIST_VARIANT=%MPT_DIST_VARIANT_OS%-%MPT_DIST_VARIANT_ARCH%


if "%MPT_VS_TARGET%" == "winxp"   set MPT_DIST_VARIANT_PREFIX=retro-winxp
if "%MPT_VS_TARGET%" == "vista"   set MPT_DIST_VARIANT_PREFIX=retro-winvista
if "%MPT_VS_TARGET%" == "win7"    set MPT_DIST_VARIANT_PREFIX=win7
if "%MPT_VS_TARGET%" == "win81"   set MPT_DIST_VARIANT_PREFIX=win81
if "%MPT_VS_TARGET%" == "win10"   set MPT_DIST_VARIANT_PREFIX=win10
if "%MPT_VS_TARGET%" == "default" set MPT_DIST_VARIANT_PREFIX=
if "%MPT_VS_TARGET%" == "win10clang" set MPT_DIST_VARIANT_PREFIX=win10clang

if "%MPT_VS_ARCH%" == "Win32" set MPT_DIST_VARIANT_SUFFIX=x86
if "%MPT_VS_ARCH%" == "x64"   set MPT_DIST_VARIANT_SUFFIX=amd64
if "%MPT_VS_ARCH%" == "ARM"   set MPT_DIST_VARIANT_SUFFIX=arm
if "%MPT_VS_ARCH%" == "ARM64" set MPT_DIST_VARIANT_SUFFIX=arm64

set MPT_DIST_VARIANT_TRK=%MPT_DIST_VARIANT_PREFIX%-%MPT_DIST_VARIANT_SUFFIX%


rem Environment Variable \ Program Bitness 32bit Native 64bit Native WOW64
rem PROCESSOR_ARCHITECTURE                 x86          AMD64        x86
rem PROCESSOR_ARCHITEW6432                 undefined    undefined    AMD64

set MPT_HOST_BITNESS=32
if "%PROCESSOR_ARCHITECTURE%" == "x86" (
	if "%PROCESSOR_ARCHITEW6432%" == "AMD64" (
		set MPT_HOST_BITNESS=64
	) else (
		set MPT_HOST_BITNESS=32
	)
)
if "%PROCESSOR_ARCHITECTURE%" == "AMD64" (
	set MPT_HOST_BITNESS=64
)


if "%MPT_PKG_FORMAT%" == "" set MPT_PKG_FORMAT=zip
if "%MPT_PKG_FORMAT%" == "7z" set MPT_PKG_FORMAT_SYMBOLS=xz
if "%MPT_PKG_FORMAT%" == "zip" set MPT_PKG_FORMAT_SYMBOLS=zip
if "%MPT_PKG_FORMAT_SYMBOLS%" == "" set MPT_PKG_FORMAT_SYMBOLS=zip


if "%MPT_VS_FLAVOUR%" == "default" set MPT_VS_FLAVOUR=
