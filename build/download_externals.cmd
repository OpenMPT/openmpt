@echo off

if not "x%1" == "xauto" (
	echo "WARNING: This script will unconditionally remove all files from the destination directories."
	pause
)

if "x%2" == "xnodownload" (
 set MPT_DOWNLOAD=no
)
if not "x%2" == "xnodownload" (
 set MPT_DOWNLOAD=yes
)

set MY_DIR=%CD%
set BATCH_DIR=%~dp0
cd %BATCH_DIR% || goto error
cd .. || goto error
goto main

:killdir
 set MPT_KILLDIR_DIR="%~1"
 if exist %MPT_KILLDIR_DIR% rmdir /S /Q %MPT_KILLDIR_DIR%
exit /B 0
goto error

:main
if not exist "build\externals" mkdir "build\externals"
if not exist "build\tools"     mkdir "build\tools"

if "%MPT_DOWNLOAD%" == "yes" (

 call build\scriptlib\download.cmd "https://www.7-zip.org/a/7za920.zip"                                                              "build\externals\7za920.zip"      || goto error
 call build\scriptlib\download.cmd "https://www.7-zip.org/a/7z1805-extra.7z"                                                         "build\externals\7z1805-extra.7z" || goto error
 call build\scriptlib\download.cmd "https://www.7-zip.org/a/7z1805.exe"                                                              "build\externals\7z1805.exe"      || goto error

 rem call build\scriptlib\download.cmd "https://github.com/bkaradzic/GENie/archive/78817a9707c1a02e845fb38b3adcc5353b02d377.zip"         "build\externals\GENie-78817a9707c1a02e845fb38b3adcc5353b02d377.zip"        || goto error
 rem call build\scriptlib\download.cmd "https://github.com/premake/premake-core/archive/2e7ca5fb18acdbcd5755fb741710622b20f2e0f6.zip"    "build\externals\premake-core-2e7ca5fb18acdbcd5755fb741710622b20f2e0f6.zip" || goto error
 copy /Y "build\externals-mirror\genie\GENie-78817a9707c1a02e845fb38b3adcc5353b02d377.zip" "build\externals\GENie-78817a9707c1a02e845fb38b3adcc5353b02d377.zip"               || goto error
 copy /Y "build\externals-mirror\premake\premake-core-2e7ca5fb18acdbcd5755fb741710622b20f2e0f6.zip" "build\externals\premake-core-2e7ca5fb18acdbcd5755fb741710622b20f2e0f6.zip" || goto error

 call build\scriptlib\download.cmd "http://download.nullsoft.com/winamp/plugin-dev/WA5.55_SDK.exe"                                   "build\externals\WA5.55_SDK.exe"  || goto error
 call build\scriptlib\download.cmd "https://www.un4seen.com/files/xmp-sdk.zip"                                                       "build\externals\xmp-sdk.zip"     || goto error
 call build\scriptlib\download.cmd "https://www.steinberg.net/sdk_downloads/asiosdk2.3.zip"                                          "build\externals\asiosdk2.3.zip"  || goto error

 call build\scriptlib\download.cmd "https://download.microsoft.com/download/0/A/9/0A939EF6-E31C-430F-A3DF-DFAE7960D564/htmlhelp.exe" "build\externals\htmlhelp.exe"    || goto error

)

call :killdir "build\tools\7zipold" || goto error
call :killdir "build\tools\7zipa" || goto error
call :killdir "build\tools\7zip" || goto error
rem Get old 7zip distributed as zip and unpack with built-in zip depacker
rem Get current 7zip commandline version which can unpack 7zip and the 7zip installer but not other archive formats
rem Get 7zip installer and unpack it with current commandline 7zip
rem This is a mess for automatic. Oh well.
cscript build\scriptlib\unpack-zip.vbs "build\externals\7za920.zip" "build\tools\7zipold" || goto error
build\tools\7zipold\7za.exe x -y -obuild\tools\7zipa "build\externals\7z1805-extra.7z" || goto error
build\tools\7zipa\7za.exe x -y -obuild\tools\7zip "build\externals\7z1805.exe" || goto error

call build\scriptlib\unpack.cmd "build\tools\htmlhelp" "build\externals\htmlhelp.exe" "." || goto error

call build\scriptlib\unpack.cmd "include\winamp"   "build\externals\WA5.55_SDK.exe" "."          || goto error
call build\scriptlib\unpack.cmd "include\xmplay"   "build\externals\xmp-sdk.zip"    "."          || goto error
call build\scriptlib\unpack.cmd "include\ASIOSDK2" "build\externals\asiosdk2.3.zip" "ASIOSDK2.3" || goto error

goto ok

:ok
echo "All OK."
if "x%1" == "xauto" (
	exit 0
)
goto end
:error
echo "Error!"
if "x%1" == "xauto" (
	exit 1
)
goto end
:end
cd %MY_DIR%
pause
