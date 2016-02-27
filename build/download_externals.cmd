@echo off

echo WARNING: This script will unconditionally remove all files from the destination directories.
echo This script requires Windows 7 or later (because of PowerShell).
echo This script requires 7-zip in "C:\Program Files\7-Zip\" (the default path for a native install).
echo When running from a Subversion working copy, this script requires at least Subversion 1.7 (because it removes subdirectories which should not contain .svn metadata).

pause

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
 if not exist "build\externals\%~3" (
  powershell -Command "(New-Object Net.WebClient).DownloadFile('%MPT_GET_URL%', 'build/externals/%~3')" || exit /B 1
 )
 cd include || exit /B 1
 if exist %MPT_GET_DESTDIR% rmdir /S /Q %MPT_GET_DESTDIR%
 if "%~4" == "." (
  mkdir %MPT_GET_DESTDIR%
  cd %MPT_GET_DESTDIR% || exit /B 1
  "C:\Program Files\7-Zip\7z.exe" x -y "..\..\build\externals\%~3" || exit /B 1
  cd .. || exit /B 1
 )
 if not "%~4" == "." (
  "C:\Program Files\7-Zip\7z.exe" x -y "..\build\externals\%~3" || exit /B 1
  choice /C y /N /T 2 /D y
  move /Y "%~4" %MPT_GET_DESTDIR% || exit /B 1
 )
 cd .. || exit /B 1
exit /B 0
goto error

:main
if not exist "build\externals" mkdir "build\externals"

call :download_and_unpack "premake" "https://github.com/premake/premake-core/releases/download/v5.0.0-alpha8/premake-5.0.0-alpha8-src.zip" "premake-5.0-alpha8-src.zip" "premake-5.0.0-alpha8" || goto error

if exist "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" (
 call build\auto\setup_vs2013.cmd || goto error
 cd include\premake\build\vs2013 || goto error
 devenv Premake5.sln /build "Release|Win32" || goto error
 cd ..\..\..\.. || goto error
 goto premakedone
)
if exist "C:\Program Files\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" (
 call build\auto\setup_vs2013.cmd || goto error
 cd include\premake\build\vs2013 || goto error
 devenv Premake5.sln /build "Release|Win32" || goto error
 cd ..\..\..\.. || goto error
 goto premakedone
)

if exist "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\vcvarsall.bat" (
 call build\auto\setup_vs2012.cmd || goto error
 cd include\premake\build\vs2012 || goto error
 devenv Premake5.sln /build "Release|Win32" || goto error
 cd ..\..\..\.. || goto error
 goto premakedone
)
if exist "C:\Program Files\Microsoft Visual Studio 11.0\VC\vcvarsall.bat" (
 call build\auto\setup_vs2012.cmd || goto error
 cd include\premake\build\vs2012 || goto error
 devenv Premake5.sln /build "Release|Win32" || goto error
 cd ..\..\..\.. || goto error
 goto premakedone
)

if exist "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" (
 call build\auto\setup_vs2010.cmd || goto error
 cd include\premake\build\vs2010 || goto error
 devenv Premake5.sln /build "Release|Win32" || goto error
 cd ..\..\..\.. || goto error
 goto premakedone
)
if exist "C:\Program Files\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" (
 call build\auto\setup_vs2010.cmd || goto error
 cd include\premake\build\vs2010 || goto error
 devenv Premake5.sln /build "Release|Win32" || goto error
 cd ..\..\..\.. || goto error
 goto premakedone
)

if exist "C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\vcvarsall.bat" (
 call build\auto\setup_vs2008.cmd || goto error
 cd include\premake\build\vs2008 || goto error
 devenv Premake5.sln /build "Release|Win32" || goto error
 cd ..\..\..\.. || goto error
 goto premakedone
)
if exist "C:\Program Files\Microsoft Visual Studio 9.0\VC\vcvarsall.bat" (
 call build\auto\setup_vs2008.cmd || goto error
 cd include\premake\build\vs2008 || goto error
 devenv Premake5.sln /build "Release|Win32" || goto error
 cd ..\..\..\.. || goto error
 goto premakedone
)

goto error

:premakedone

call :download_and_unpack "winamp"    "http://download.nullsoft.com/winamp/plugin-dev/WA5.55_SDK.exe"              "WA5.55_SDK.exe"                     "."          || goto error
call :download_and_unpack "xmplay"    "http://us.un4seen.com/files/xmp-sdk.zip"                                    "xmp-sdk.zip"                        "."          || goto error
call :download_and_unpack "ASIOSDK2"  "https://www.steinberg.net/sdk_downloads/asiosdk2.3.zip"                     "asiosdk2.3.zip"                     "ASIOSDK2.3" || goto error
call :download_and_unpack "vstsdk2.4" "https://www.steinberg.net/sdk_downloads/vstsdk365_12_11_2015_build_67.zip"  "vstsdk365_12_11_2015_build_67.zip"  "VST3 SDK"   || goto error

goto ok

:ok
echo "All OK."
goto end
:error
echo "Error!"
goto end
:end
cd %MY_DIR%
pause
