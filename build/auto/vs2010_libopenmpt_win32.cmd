@echo off

set MY_DIR=%CD%

if exist "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" goto msvc64bit
if exist "C:\Program Files\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" goto msvc32bit

goto error

:msvc32bit
call "C:\Program Files\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" x86
goto compile

:msvc64bit
call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" x86
goto compile

:compile



xcopy /e /y /c ..\..\externals\*.* include

cd libopenmpt || goto error
 devenv libopenmpt.sln /clean "Release|Win32" || goto error
 devenv foo_openmpt.sln /clean "Release|Win32" || goto error
cd .. || goto error
cd openmpt123 || goto error
 devenv openmpt123.sln /clean "Release|Win32" || goto error
cd .. || goto error

cd libopenmpt || goto error
 devenv libopenmpt.sln /build "Release|Win32" || goto error
 devenv foo_openmpt.sln /build "Release|Win32" || goto error
cd .. || goto error
cd openmpt123 || goto error
 devenv openmpt123.sln /build "Release|Win32" || goto error
cd .. || goto error



goto noerror

:error
cd "%MY_DIR%"
exit 1

:noerror
cd "%MY_DIR%"
exit 0
