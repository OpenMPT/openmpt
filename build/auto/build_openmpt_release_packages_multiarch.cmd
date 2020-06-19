@echo off

set BATCH_DIR=%~dp0
cd %BATCH_DIR%

set MY_DIR=%CD%

cd ..\.. || goto error

call build\auto\setup_vs_any.cmd

call build\auto\helper_get_svnversion.cmd
call build\auto\helper_get_openmpt_version.cmd

cd build\auto || goto error

if "x%1" == "xauto" (
	..\..\build\tools\python3\python.exe ..\..\build\auto\build_openmpt_release_packages_multiarch.py --localtools --singlethreaded --noninteractive || goto error
) else (
	..\..\build\tools\python3\python.exe ..\..\build\auto\build_openmpt_release_packages_multiarch.py --localtools --singlethreaded || goto error
)
..\..\build\tools\python3\python.exe ..\..\installer\generate_update_json.py || goto error

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
