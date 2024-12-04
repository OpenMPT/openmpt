@echo off

if not "x%1" == "xauto" (
 echo "WARNING: This script will unconditionally remove all files from the destination directories."
 pause
)

if "x%2" == "xnodownload" (
 set MPT_DOWNLOAD_DO=no
)
if not "x%2" == "xnodownload" (
 set MPT_DOWNLOAD_DO=yes
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

rem download
for /f "delims=" %%a in ('type "build\download_externals.txt"') do ( call build\scriptlib\download.cmd %%a || goto error )

call :killdir "build\tools\7zipold" || goto error
call :killdir "build\tools\7zipa" || goto error
call :killdir "build\tools\7zip" || goto error
rem Get old 7zip distributed as zip and unpack with built-in zip depacker
rem Get current 7zip commandline version which can unpack 7zip and the 7zip installer but not other archive formats
rem Get 7zip installer and unpack it with current commandline 7zip
rem This is a mess for automation. Oh well.
cscript build\scriptlib\unpack-zip.vbs "build\externals\7za920.zip" "build\tools\7zipold" || goto error
build\tools\7zipold\7za.exe x -y -obuild\tools\7zipa "build\externals\7z2409-extra.7z" || goto error
build\tools\7zipa\7za.exe x -y -obuild\tools\7zip "build\externals\7z2409-x64.exe" || goto error

call build\scriptlib\unpack.cmd "build\tools\htmlhelp" "build\externals\htmlhelp.exe" "." || goto error

call build\scriptlib\unpack.cmd "include\winamp"   "build\externals\WA5.55_SDK.exe" "."          || goto error
call build\scriptlib\unpack.cmd "include\xmplay"   "build\externals\xmp-sdk.zip"    "."          || goto error

call build\scriptlib\unpack.cmd "build\tools\python3" "build\externals\python-3.13.1-embed-amd64.zip" "." || goto error


call :killdir "build\tools\innosetup" || goto error
mkdir "build\tools\innosetup" || goto error
"build\externals\innosetup-6.3.3.exe" /PORTABLE=1 /CURRENTUSER /DIR="%CD%\build\tools\innosetup\{app}" /LOG="%CD%\build\tools\innosetup\setup.log" /SILENT || goto error

call :killdir "build\tools\innounp"   || goto error
call :killdir "build\tools\innosetup5" || goto error
call build\scriptlib\unpack.cmd "build\tools\innounp" "build\externals\innounp050.rar" "." || goto error
build\tools\innounp\innounp.exe -x -dbuild\tools\innosetup5 "build\externals\isetup-5.5.8-unicode.exe" || goto error

call build\scriptlib\unpack.cmd "packageTemplate\ExampleSongs" "build\externals\example_songs_ompt_1_30.7z" "." || goto error


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
