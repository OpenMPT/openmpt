@echo off

set BATCH_DIR=%~dp0
cd %BATCH_DIR%
cd ..\..

set MY_DIR=%CD%

call build\auto\setup_vs2008.cmd





cd mptrack || goto error
devenv MPTRACK_08.sln /clean "Release|Win32" || goto error
devenv MPTRACK_08.sln /build "Release|Win32" || goto error
cd .. || goto error



goto noerror

:error
cd "%MY_DIR%"
exit 1

:noerror
cd "%MY_DIR%"
exit 0
