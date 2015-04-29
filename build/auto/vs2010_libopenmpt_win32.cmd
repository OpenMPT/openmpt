@echo off

set BATCH_DIR=%~dp0
cd %BATCH_DIR%
cd ..\..

set MY_DIR=%CD%

call build\auto\setup_vs2010.cmd





cd build\vs2010 || goto error

 devenv libopenmpt.sln /clean "Release|Win32" || goto error
 devenv openmpt123.sln /clean "Release|Win32" || goto error
 devenv in_openmpt.sln /clean "Release|Win32" || goto error
 devenv xmp-openmpt.sln /clean "Release|Win32" || goto error
 devenv foo_openmpt.sln /clean "Release|Win32" || goto error

 devenv libopenmpt.sln /build "Release|Win32" || goto error
 devenv openmpt123.sln /build "Release|Win32" || goto error
 devenv in_openmpt.sln /build "Release|Win32" || goto error
 devenv xmp-openmpt.sln /build "Release|Win32" || goto error
 devenv foo_openmpt.sln /build "Release|Win32" || goto error

cd ..\.. || goto error



goto noerror

:error
cd "%MY_DIR%"
exit 1

:noerror
cd "%MY_DIR%"
exit 0
