@echo off

set BATCH_DIR=%~dp0
cd %BATCH_DIR%
cd ..\..

set GOT_REVISION=%1%
set MY_DIR=%CD%



bin\Win32\libopenmpt_test.exe || goto error



goto noerror

:error
cd "%MY_DIR%"
exit 1

:noerror
cd "%MY_DIR%"
exit 0
