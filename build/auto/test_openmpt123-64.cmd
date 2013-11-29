@echo off

set GOT_REVISION=%1%
set MY_DIR=%CD%



cd libopenmpt || goto error
bin\libopenmpt64-test.exe || goto error
cd .. || goto error



goto noerror

:error
cd "%MY_DIR%"
exit 1

:noerror
cd "%MY_DIR%"
exit 0
