@echo off

set BATCH_DIR=%~dp0
cd %BATCH_DIR%
cd ..\..

set MY_DIR=%CD%

call "build\auto\setup_arguments.cmd" %1 %2 %3 %4

call build\auto\setup_vs_any.cmd

call build\auto\helper_get_svnversion.cmd
call build\auto\helper_get_openmpt_version.cmd

set MPT_REVISION=%OPENMPT_VERSION%-r%SVNVERSION%



cd bin || goto error
rmdir /s /q openmpt-%MPT_DIST_VARIANT%
del /f /q openmpt-%MPT_DIST_VARIANT%.tar
del /f /q openmpt-%MPT_DIST_VARIANT%-%MPT_REVISION%.7z
del /f /q openmpt-%MPT_DIST_VARIANT%-%MPT_REVISION%-symbols.7z
mkdir openmpt-%MPT_DIST_VARIANT%
cd openmpt-%MPT_DIST_VARIANT% || goto error
copy /y ..\..\LICENSE .\LICENSE.txt || goto error
rmdir /s /q Licenses
mkdir Licenses
copy /y ..\..\packageTemplate\Licenses\*.* .\Licenses\ || goto error
copy /y ..\..\bin\%MPT_BIN_CONF%\%MPT_VS_VER%-%MPT_BIN_RUNTIME%\%MPT_BIN_ARCH%-%MPT_BIN_TARGET%\mptrack.exe .\ || goto error
copy /y ..\..\bin\%MPT_BIN_CONF%\%MPT_VS_VER%-%MPT_BIN_RUNTIME%\%MPT_BIN_ARCH%-%MPT_BIN_TARGET%\mptrack.pdb .\ || goto error
copy /y ..\..\bin\%MPT_BIN_CONF%\%MPT_VS_VER%-%MPT_BIN_RUNTIME%\%MPT_BIN_ARCH%-%MPT_BIN_TARGET%\OpenMPT_SoundTouch_f32.dll .\ || goto error
copy /y ..\..\bin\%MPT_BIN_CONF%\%MPT_VS_VER%-%MPT_BIN_RUNTIME%\%MPT_BIN_ARCH%-%MPT_BIN_TARGET%\PluginBridge32.exe .\ || goto error
copy /y ..\..\bin\%MPT_BIN_CONF%\%MPT_VS_VER%-%MPT_BIN_RUNTIME%\%MPT_BIN_ARCH%-%MPT_BIN_TARGET%\PluginBridge64.exe .\ || goto error
"C:\Program Files\7-Zip\7z.exe" a -t7z -mx=9 ..\openmpt-%MPT_DIST_VARIANT%-%MPT_REVISION%.7z ^
 LICENSE.txt ^
 Licenses ^
 mptrack.exe ^
 OpenMPT_SoundTouch_f32.dll ^
 PluginBridge32.exe ^
 PluginBridge64.exe ^
 || goto error
"C:\Program Files\7-Zip\7z.exe" a -t7z -mx=9 ..\openmpt-%MPT_DIST_VARIANT%-%MPT_REVISION%-symbols.7z mptrack.pdb || goto error
cd .. || goto error
"C:\Program Files\7-Zip\7z.exe" a -ttar openmpt-%MPT_DIST_VARIANT%.tar openmpt-%MPT_DIST_VARIANT%-%MPT_REVISION%.7z openmpt-%MPT_DIST_VARIANT%-%MPT_REVISION%-symbols.7z || goto error
del /f /q openmpt-%MPT_DIST_VARIANT%-%MPT_REVISION%.7z
del /f /q openmpt-%MPT_DIST_VARIANT%-%MPT_REVISION%-symbols.7z
rmdir /s /q openmpt-%MPT_DIST_VARIANT%
cd .. || goto error



goto noerror

:error
cd "%MY_DIR%"
exit 1

:noerror
cd "%MY_DIR%"
exit 0
