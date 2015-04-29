@echo off

set BATCH_DIR=%~dp0
cd %BATCH_DIR%
cd ..\..

set MY_DIR=%CD%

call build\auto\setup_vs2010.cmd





cd mptrack || goto error
devenv MPTRACK_10.sln /clean "Release|Win32" || goto error
cd .. || goto error
cd pluginBridge || goto error
devenv PluginBridge.sln /clean "Release|x64" || goto error
cd .. || goto error

cd mptrack || goto error
devenv MPTRACK_10.sln /build "Release|Win32" || goto error
cd .. || goto error
cd pluginBridge || goto error
devenv PluginBridge.sln /build "Release|x64" || goto error
cd .. || goto error



goto noerror

:error
cd "%MY_DIR%"
exit 1

:noerror
cd "%MY_DIR%"
exit 0
