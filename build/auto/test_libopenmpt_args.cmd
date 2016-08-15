@echo off

set BATCH_DIR=%~dp0
cd %BATCH_DIR%
cd ..\..

set MY_DIR=%CD%

call "build\auto\setup_arguments.cmd" %1 %2 %3 %4

call "build\auto\setup_%MPT_VS_VER%.cmd"



"bin\%MPT_BIN_CONF%\%MPT_VS_VER%-%MPT_BIN_RUNTIME%\%MPT_BIN_ARCH%-%MPT_BIN_TARGET%\libopenmpt_test.exe" || goto error



goto noerror

:error
cd "%MY_DIR%"
exit 1

:noerror
cd "%MY_DIR%"
exit 0
