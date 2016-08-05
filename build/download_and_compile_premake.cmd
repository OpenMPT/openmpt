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
 set MPT_GET_UNPACK_INTERMEDIATE="%~5"
 if not exist "build\externals\%~3" (
  powershell -Command "(New-Object Net.WebClient).DownloadFile('%MPT_GET_URL%', 'build/externals/%~3')" || exit /B 1
  cd build\externals || exit /B 1
  if not "%~5" == "-" (
   "C:\Program Files\7-Zip\7z.exe" x -y "%~3" || exit /B 1
  )
  cd ..\.. || exit /B 1
 )
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

call :download_and_unpack "premake" "https://github.com/premake/premake-core/releases/download/v5.0.0-alpha9/premake-5.0.0-alpha9-src.zip" "premake-5.0-alpha9-src.zip" "premake-5.0.0-alpha9" "-" || goto error

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

if exist "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" (
 call build\auto\setup_vs2015.cmd || goto error
 cd include\premake\build\vs2013 || goto error
 devenv Premake5.sln /build "Release|Win32" || goto error
 cd ..\..\..\.. || goto error
 goto premakedone
)
if exist "C:\Program Files\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" (
 call build\auto\setup_vs2015.cmd || goto error
 cd include\premake\build\vs2013 || goto error
 devenv Premake5.sln /build "Release|Win32" || goto error
 cd ..\..\..\.. || goto error
 goto premakedone
)

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

goto error

:premakedone

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
