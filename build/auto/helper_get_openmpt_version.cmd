@echo off

cd build\auto
cl /I..\.. /EHsc helper_get_openmpt_version.cpp
cd ..\..

build\auto\helper_get_openmpt_version.exe > openmpt_version.txt
set /p OPENMPT_VERSION=<openmpt_version.txt
del /f openmpt_version.txt

cd build\auto
del /f helper_get_openmpt_version.obj
del /f helper_get_openmpt_version.exe
cd ..\..

rem echo %OPENMPT_VERSION%
