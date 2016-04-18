@echo off

set BATCH_DIR=%~dp0
cd %BATCH_DIR%
cd ..\..

set MY_DIR=%CD%

call build\auto\setup_vs_any.cmd

call build\auto\helper_get_svnversion.cmd
call build\auto\helper_get_openmpt_version.cmd

set MPT_REVISION=%OPENMPT_VERSION%-r%SVNVERSION%


cd bin\Win32 || goto error
del /f /q openmpt-win32old.tar
del /f /q openmpt-win32old-%MPT_REVISION%.7z
copy /y ..\..\LICENSE .\LICENSE.txt || goto error
rmdir /s /q Licenses
mkdir Licenses
copy /y ..\..\packageTemplate\Licenses\*.* .\Licenses\ || goto error
"C:\Program Files\7-Zip\7z.exe" a -t7z -mx=9 openmpt-win32old-%MPT_REVISION%.7z ^
 LICENSE.txt ^
 Licenses ^
 mptrack.exe ^
 OpenMPT_SoundTouch_f32.dll ^
 "MIDI Input Output.dll" ^
 PluginBridge32.exe ^
 PluginBridge64.exe ^
 || goto error
"C:\Program Files\7-Zip\7z.exe" a -t7z -mx=9 openmpt-win32old-%MPT_REVISION%-symbols.7z mptrack.pdb || goto error
"C:\Program Files\7-Zip\7z.exe" a -ttar openmpt-win32old.tar openmpt-win32old-%MPT_REVISION%.7z openmpt-win32old-%MPT_REVISION%-symbols.7z || goto error
del /f /q openmpt-win32old-%MPT_REVISION%.7z
del /f /q openmpt-win32old-%MPT_REVISION%-symbols.7z
cd ..\.. || goto error



goto noerror

:error
cd "%MY_DIR%"
exit 1

:noerror
cd "%MY_DIR%"
exit 0
