@echo off

setlocal enableextensions enabledelayedexpansion

if "%1" == "" goto setupargumentserror

set MY_DIR=%CD%

set BATCH_DIR=%~dp0
cd %BATCH_DIR%
cd ..\..

set BIN_DIR_MODERN=%CD%\bin\release\%1-win10-static
set ARCH_MODERN=amd64 x86 arm64 arm
set BIN_DIR_LEGACY=%CD%\bin\release\%1-win7-static
set ARCH_LEGACY=amd64 x86

if "x%2" == "xsetup" (
	set SIGN_PATHS=%CD%\installer\*.exe
) else (
	set SIGN_PATHS=

	for %%a in (%ARCH_MODERN%) do ( 
		set SIGN_PATHS=!SIGN_PATHS! "%BIN_DIR_MODERN%\%%a\*.exe"
		set SIGN_PATHS=!SIGN_PATHS! "%BIN_DIR_MODERN%\%%a\*.dll"
	)

	for %%a in (%ARCH_LEGACY%) do ( 
		set SIGN_PATHS=!SIGN_PATHS! "%BIN_DIR_LEGACY%\%%a\*.exe"
		set SIGN_PATHS=!SIGN_PATHS! "%BIN_DIR_LEGACY%\%%a\*.dll"
	)
)

call "build\auto\setup_%1.cmd"

signtool sign /fd sha256 /a /tr http://time.certum.pl /td sha256 %SIGN_PATHS% || goto error

goto noerror

:error
cd "%MY_DIR%"
echo "Signing failed."
pause
exit 1

:setupargumentserror
echo "Usage: sign_openmpt_executables.cmd vs2019 [setup]"
exit 1

:noerror
cd "%MY_DIR%"
echo "Signing successful."
exit 0
