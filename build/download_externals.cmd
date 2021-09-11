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
   "C:\Program Files\7-Zip\7z.exe" x -y -ir!"%~4" "..\build\externals\%~3" || exit /B 1
  )
  if not "%~5" == "-" (
   "C:\Program Files\7-Zip\7z.exe" x -y -ir!"%~4" "..\build\externals\%~5" || exit /B 1
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

call :download_and_unpack "winamp"    "http://download.nullsoft.com/winamp/plugin-dev/WA5.55_SDK.exe"              "WA5.55_SDK.exe"                     "."                "-" || goto error

call :download_and_unpack "xmplay"    "https://www.un4seen.com/files/xmp-sdk.zip"                                  "xmp-sdk.zip"                        "."                "-" || goto error

call :download_and_unpack "ASIOSDK2"  "https://web.archive.org/web/20191011060157if_/https://www.steinberg.net/sdk_downloads/asiosdk2.3.zip"                     "asiosdk2.3.zip"                     "ASIOSDK2.3"       "-" || goto error

call :download_and_unpack "vstsdk2.4" "https://web.archive.org/web/20200502121256if_/https://www.steinberg.net/sdk_downloads/vstsdk367_03_03_2017_build_352.zip" "vstsdk367_03_03_2017_build_352.zip" "VST_SDK\VST2_SDK" "-" || goto error
rmdir /s /q include\VST_SDK || goto error

call :download_and_unpack "lame"      "https://netcologne.dl.sourceforge.net/project/lame/lame/3.100/lame-3.100.tar.gz"   "lame-3.100.tar.gz"                 "lame-3.100"      "lame-3.100.tar" || goto error
cd include\lame || goto error
rem PowerShell 3 on Windows 7 requires https://www.microsoft.com/en-us/download/details.aspx?id=34595
powershell -Version 3 -Command "(Get-Content include\lame.def -Raw).replace(\"libmp3lame.DLL\", \"\") | Set-Content include\lame.def -Force" || goto error
cd ..\.. || goto error

call :download_and_unpack "htmlhelp" "https://web.archive.org/web/20200918004813if_/http://download.microsoft.com/download/0/A/9/0A939EF6-E31C-430F-A3DF-DFAE7960D564/htmlhelp.exe" "htmlhelp.exe" "." "-" || goto error

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
