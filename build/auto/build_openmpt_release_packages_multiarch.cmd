@echo off

set BATCH_DIR=%~dp0
cd %BATCH_DIR%

set MY_DIR=%CD%

cd ..\.. || goto error

call build\auto\setup_vs_any.cmd

call build\auto\helper_get_svnversion.cmd
call build\auto\helper_get_openmpt_version.cmd

cd build\auto || goto error

set BUILD_PARAMS=--localtools --singlethreaded
if "x%1" == "xauto" (
	set BUILD_PARAMS=%BUILD_PARAMS% --noninteractive
	if "x%2" == "xsign" (
		set BUILD_PARAMS=%BUILD_PARAMS% --sign-binaries --sign-installer
	)
) else (
	if "x%1" == "xsign" (
		set BUILD_PARAMS=%BUILD_PARAMS% --sign-binaries --sign-installer
	)
)

..\..\build\tools\python3\python.exe ..\..\build\auto\build_openmpt_release_packages_multiarch.py %BUILD_PARAMS% || goto error

set BUILD_PARAMS=

cd "%MY_DIR%"

goto ok

:ok
echo "All OK."
if "x%1" == "xauto" (
	exit 0
)
goto end
:error
echo "Error!"
if "x%1" == "xauto" (
	exit 1
)
goto end
:end
cd %MY_DIR%
pause
