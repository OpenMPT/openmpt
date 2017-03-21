@echo off

if not "x%1" == "xauto" (
	echo "WARNING: This script will unconditionally remove all files from the destination directories."
	echo "This script requires Windows 7 or later (because of PowerShell)."
	echo "This script requires 7-zip in 'C:\Program Files\7-Zip\' (the default path for a native install)."
	echo "When running from a Subversion working copy, this script requires at least Subversion 1.7 (because it removes subdirectories which should not contain .svn metadata)."
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

:download_and_unpack
 set MPT_GET_DESTDIR="%~1"
 set MPT_GET_URL="%~2"
 set MPT_GET_FILE="%~3"
 set MPT_GET_SUBDIR="%~4"
 set MPT_GET_UNPACK_INTERMEDIATE="%~5"
 if "%MPT_DOWNLOAD%" == "yes" (
  if not exist "build\externals\%~3" (
   powershell -Command "(New-Object Net.WebClient).DownloadFile('%MPT_GET_URL%', 'build/externals/%~3')" || exit /B 1
  )
 )
 cd build\externals || exit /B 1
 if not "%~5" == "-" (
  "C:\Program Files\7-Zip\7z.exe" x -y "%~3" || exit /B 1
 )
 cd ..\.. || exit /B 1
 cd include || exit /B 1
 if exist %MPT_GET_DESTDIR% rmdir /S /Q %MPT_GET_DESTDIR%
 if "%~4" == "." (
  mkdir %MPT_GET_DESTDIR%
  cd %MPT_GET_DESTDIR% || exit /B 1
  if "%~5" == "-" (
   "C:\Program Files\7-Zip\7z.exe" x -y "..\..\build\externals\%~3" || exit /B 1
  )
  if not "%~5" == "-" (
   "C:\Program Files\7-Zip\7z.exe" x -y "..\..\build\externals\%~5" || exit /B 1
  )
  cd .. || exit /B 1
 )
 if not "%~4" == "." (
  if "%~5" == "-" (
   "C:\Program Files\7-Zip\7z.exe" x -y "..\build\externals\%~3" || exit /B 1
  )
  if not "%~5" == "-" (
   "C:\Program Files\7-Zip\7z.exe" x -y "..\build\externals\%~5" || exit /B 1
  )
  choice /C y /N /T 2 /D y
  if not "%~4" == "%~1" (
   move /Y "%~4" %MPT_GET_DESTDIR% || exit /B 1
  )
 )
 cd .. || exit /B 1
exit /B 0
goto error

:main
if not exist "build\externals" mkdir "build\externals"

call :download_and_unpack "winamp"    "http://download.nullsoft.com/winamp/plugin-dev/WA5.55_SDK.exe"             "WA5.55_SDK.exe"                    "."             "-" || goto error
call :download_and_unpack "xmplay"    "http://us.un4seen.com/files/xmp-sdk.zip"                                   "xmp-sdk.zip"                       "."             "-" || goto error
call :download_and_unpack "ASIOSDK2"  "https://www.steinberg.net/sdk_downloads/asiosdk2.3.zip"                    "asiosdk2.3.zip"                    "ASIOSDK2.3"    "-" || goto error
call :download_and_unpack "vstsdk2.4" "https://www.steinberg.net/sdk_downloads/vstsdk365_12_11_2015_build_67.zip" "vstsdk365_12_11_2015_build_67.zip" "VST3 SDK"      "-" || goto error
call :download_and_unpack "lame"      "https://sourceforge.net/projects/lame/files/lame/3.99/lame-3.99.5.tar.gz"  "lame-3.99.5.tar.gz"                "lame-3.99.5"   "lame-3.99.5.tar" || goto error
call :download_and_unpack "mpg123"    "http://www.mpg123.de/download/mpg123-1.23.8.tar.bz2"                       "mpg123-1.23.8.tar.bz2"             "mpg123-1.23.8" "mpg123-1.23.8.tar" || goto error
rem call :download_and_unpack "minimp3"   "http://keyj.emphy.de/files/projects/minimp3.tar.gz"                         "minimp3.tar.gz"                     "minimp3"    "minimp3.tar" || goto error

rem workaround https://sourceforge.net/p/mpg123/bugs/243/
cd include\mpg123 || goto error
rem PowerShell 3 on Windows 7 requires https://www.microsoft.com/en-us/download/details.aspx?id=34595
powershell -Version 3 -Command "(Get-Content src\compat\compat.h -Raw).replace(\"typedef long ssize_t\", \"typedef ptrdiff_t ssize_t\") | Set-Content src\compat\compat.h -Force" || goto error
cd ..\.. || goto error

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
