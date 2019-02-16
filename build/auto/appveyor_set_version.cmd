@echo off

set BATCH_DIR=%~dp0
cd %BATCH_DIR%
cd ..\..

set MY_DIR=%CD%

call build\auto\setup_vs_any.cmd

call build\auto\helper_get_svnversion.cmd
call build\auto\helper_get_openmpt_version.cmd

set MPT_REVISION=%OPENMPT_VERSION%-%SVNVERSION%

appveyor UpdateBuild -Version "%OPENMPT_VERSION%-%SVNVERSION%-appveyor%APPVEYOR_BUILD_NUMBER%"

cd "%MY_DIR%"
