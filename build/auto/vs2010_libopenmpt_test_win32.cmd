@echo off

set BATCH_DIR=%~dp0
cd %BATCH_DIR%
cd ..\..

set MY_DIR=%CD%

call build\auto\setup_vs2010.cmd





cd libopenmpt || goto error
 devenv libopenmpt_test.sln /clean "Release|Win32" || goto error
 devenv libopenmpt_test.sln /build "Release|Win32" || goto error
cd .. || goto error



goto noerror

:error
cd "%MY_DIR%"
exit 1

:noerror
cd "%MY_DIR%"
exit 0
