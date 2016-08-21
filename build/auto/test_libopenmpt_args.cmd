@echo off

set BATCH_DIR=%~dp0
cd %BATCH_DIR%
cd ..\..

set MY_DIR=%CD%

call "build\auto\setup_arguments.cmd" %1 %2 %3 %4

call "build\auto\setup_%MPT_VS_VER%.cmd"



if "%MPT_BIN_ARCH%" == "x86-32" (
	"bin\%MPT_BIN_CONF%\%MPT_VS_VER%-%MPT_BIN_RUNTIME%\%MPT_BIN_ARCH%-%MPT_BIN_TARGET32%\libopenmpt_test.exe" || goto error
)

if "%MPT_BIN_ARCH%" == "x86-64" (
	if "%MPT_HOST_BITNESS%" == "64" (
		"bin\%MPT_BIN_CONF%\%MPT_VS_VER%-%MPT_BIN_RUNTIME%\%MPT_BIN_ARCH%-%MPT_BIN_TARGET64%\libopenmpt_test.exe" || goto error
	) else (
		echo "Warning: Host is not 64 bit. Skipping 64 bit test suite."
	)
)



goto noerror

:error
cd "%MY_DIR%"
exit 1

:noerror
cd "%MY_DIR%"
exit 0
