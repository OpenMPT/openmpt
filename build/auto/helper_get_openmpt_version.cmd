@echo off

cd build\auto
cl /I..\.. /EHsc helper_get_openmpt_version.cpp
cd ..\..

build\auto\helper_get_openmpt_version.exe openmpt > openmpt_version.txt
set /p OPENMPT_VERSION=<openmpt_version.txt
del /f openmpt_version.txt

build\auto\helper_get_openmpt_version.exe libopenmpt-version-major > openmpt_version.txt
set /p LIBOPENMPT_VERSION_MAJOR=<openmpt_version.txt
del /f openmpt_version.txt

build\auto\helper_get_openmpt_version.exe libopenmpt-version-minor > openmpt_version.txt
set /p LIBOPENMPT_VERSION_MINOR=<openmpt_version.txt
del /f openmpt_version.txt

build\auto\helper_get_openmpt_version.exe libopenmpt-version-string > openmpt_version.txt
set /p LIBOPENMPT_VERSION_STRING=<openmpt_version.txt
del /f openmpt_version.txt

cd build\auto
del /f helper_get_openmpt_version.obj
del /f helper_get_openmpt_version.exe
cd ..\..

rem echo %OPENMPT_VERSION%
