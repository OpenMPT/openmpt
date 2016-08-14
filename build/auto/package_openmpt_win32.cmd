@echo off

set BATCH_DIR=%~dp0
cd %BATCH_DIR%
cd ..\..

set MY_DIR=%CD%

set MPT_VSVER=%1
set MPT_ARCH=%2
set MPT_BIN_TARGET=%3

call build\auto\setup_vs_any.cmd

call build\auto\helper_get_svnversion.cmd
call build\auto\helper_get_openmpt_version.cmd

set MPT_REVISION=%OPENMPT_VERSION%-r%SVNVERSION%


cd bin || goto error
rmdir /s /q openmpt-win32
del /f /q openmpt-win32.tar
del /f /q openmpt-win32-%MPT_REVISION%.7z
del /f /q openmpt-win32-%MPT_REVISION%-symbols.7z
mkdir openmpt-win32
cd openmpt-win32 || goto error
copy /y ..\..\LICENSE .\LICENSE.txt || goto error
rmdir /s /q Licenses
mkdir Licenses
copy /y ..\..\packageTemplate\Licenses\*.* .\Licenses\ || goto error
copy /y ..\..\bin\release\vs2010-static\x86-32-win7\mptrack.exe .\ || goto error
copy /y ..\..\bin\release\vs2010-static\x86-32-win7\mptrack.pdb .\ || goto error
copy /y ..\..\bin\release\vs2010-static\x86-32-win7\OpenMPT_SoundTouch_f32.dll .\ || goto error
copy /y ..\..\bin\release\vs2010-static\x86-32-win7\PluginBridge32.exe .\ || goto error
copy /y ..\..\bin\release\vs2010-static\x86-32-win7\PluginBridge64.exe .\ || goto error
"C:\Program Files\7-Zip\7z.exe" a -t7z -mx=9 ..\openmpt-win32-%MPT_REVISION%.7z ^
 LICENSE.txt ^
 Licenses ^
 mptrack.exe ^
 OpenMPT_SoundTouch_f32.dll ^
 PluginBridge32.exe ^
 PluginBridge64.exe ^
 || goto error
"C:\Program Files\7-Zip\7z.exe" a -t7z -mx=9 ..\openmpt-win32-%MPT_REVISION%-symbols.7z mptrack.pdb || goto error
cd .. || goto error
"C:\Program Files\7-Zip\7z.exe" a -ttar openmpt-win32.tar openmpt-win32-%MPT_REVISION%.7z openmpt-win32-%MPT_REVISION%-symbols.7z || goto error
del /f /q openmpt-win32-%MPT_REVISION%.7z
del /f /q openmpt-win32-%MPT_REVISION%-symbols.7z
rmdir /s /q openmpt-win32
cd .. || goto error


goto noerror

:error
cd "%MY_DIR%"
exit 1

:noerror
cd "%MY_DIR%"
exit 0
