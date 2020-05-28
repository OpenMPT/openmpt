@echo off

set BATCH_DIR=%~dp0
cd %BATCH_DIR%

set MY_DIR=%CD%

if "x%1" == "xauto" (
	..\..\build\tools\python3\python.exe ..\..\build\auto\build_openmpt_release_packages_multiarch.py --localtools --singlethreaded --noexamplesongs --noninteractive || goto error
) else (
	..\..\build\tools\python3\python.exe ..\..\build\auto\build_openmpt_release_packages_multiarch.py --localtools --singlethreaded --noexamplesongs || goto error
)

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

