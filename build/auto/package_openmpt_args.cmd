@echo off

set BATCH_DIR=%~dp0
cd %BATCH_DIR%
cd ..\..

set MY_DIR=%CD%

call "build\auto\setup_arguments.cmd" %1 %2 %3 %4 %5 %6

call build\auto\setup_vs_any.cmd

call build\auto\helper_get_svnversion.cmd
call build\auto\helper_get_openmpt_version.cmd

set MPT_REVISION=%OPENMPT_VERSION%-%SVNVERSION%



cmd /c build\auto\update_package_template.cmd || goto error
cd bin || goto error
rmdir /s /q openmpt
mkdir openmpt || goto error
mkdir openmpt\bin.%MPT_DIST_VARIANT_TRK%
mkdir openmpt\bin.%MPT_DIST_VARIANT_TRK%\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%
mkdir openmpt\dbg.%MPT_DIST_VARIANT_TRK%
mkdir openmpt\dbg.%MPT_DIST_VARIANT_TRK%\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%
rmdir /s /q openmpt-%MPT_DIST_VARIANT_TRK%
del /f /q openmpt-%MPT_DIST_VARIANT_TRK%.tar
del /f /q openmpt-%MPT_DIST_VARIANT_TRK%-%MPT_REVISION%.%MPT_PKG_FORMAT%
del /f /q openmpt-%MPT_DIST_VARIANT_TRK%-%MPT_REVISION%-symbols.%MPT_PKG_FORMAT_SYMBOLS%
mkdir openmpt-%MPT_DIST_VARIANT_TRK%
cd openmpt-%MPT_DIST_VARIANT_TRK% || goto error
copy /y ..\..\LICENSE .\License.txt || goto error
rmdir /s /q Licenses
mkdir Licenses
copy /y ..\..\packageTemplate\Licenses\*.* .\Licenses\ || goto error
rmdir /s /q extraKeymaps
mkdir extraKeymaps
copy /y ..\..\packageTemplate\extraKeymaps\*.* .\extraKeymaps\ || goto error
copy /y ..\..\bin\%MPT_BIN_CONF%\%MPT_VS_VER%-%MPT_BIN_TARGET%-%MPT_BIN_RUNTIME%\%MPT_BIN_ARCH_TRK%\OpenMPT%MPT_VS_FLAVOUR%.exe .\ || goto error
copy /y ..\..\bin\%MPT_BIN_CONF%\%MPT_VS_VER%-%MPT_BIN_TARGET%-%MPT_BIN_RUNTIME%\%MPT_BIN_ARCH_TRK%\OpenMPT%MPT_VS_FLAVOUR%.pdb .\ || goto error
copy /y ..\..\bin\%MPT_BIN_CONF%\%MPT_VS_VER%-%MPT_BIN_TARGET%-%MPT_BIN_RUNTIME%\%MPT_BIN_ARCH_TRK%\openmpt-lame.dll .\ || goto error
copy /y ..\..\bin\%MPT_BIN_CONF%\%MPT_VS_VER%-%MPT_BIN_TARGET%-%MPT_BIN_RUNTIME%\%MPT_BIN_ARCH_TRK%\openmpt-lame.pdb .\ || goto error
copy /y ..\..\bin\%MPT_BIN_CONF%\%MPT_VS_VER%-%MPT_BIN_TARGET%-%MPT_BIN_RUNTIME%\%MPT_BIN_ARCH_TRK%\openmpt-mpg123.dll .\ || goto error
copy /y ..\..\bin\%MPT_BIN_CONF%\%MPT_VS_VER%-%MPT_BIN_TARGET%-%MPT_BIN_RUNTIME%\%MPT_BIN_ARCH_TRK%\openmpt-mpg123.pdb .\ || goto error
copy /y ..\..\bin\%MPT_BIN_CONF%\%MPT_VS_VER%-%MPT_BIN_TARGET%-%MPT_BIN_RUNTIME%\%MPT_BIN_ARCH_TRK%\openmpt-soundtouch.dll .\ || goto error
copy /y ..\..\bin\%MPT_BIN_CONF%\%MPT_VS_VER%-%MPT_BIN_TARGET%-%MPT_BIN_RUNTIME%\%MPT_BIN_ARCH_TRK%\openmpt-soundtouch.pdb .\ || goto error
copy /y ..\..\bin\%MPT_BIN_CONF%\%MPT_VS_VER%-%MPT_BIN_TARGET%-%MPT_BIN_RUNTIME%\x86\PluginBridge-x86.exe .\ || goto error
copy /y ..\..\bin\%MPT_BIN_CONF%\%MPT_VS_VER%-%MPT_BIN_TARGET%-%MPT_BIN_RUNTIME%\x86\PluginBridge-x86.pdb .\ || goto error
copy /y ..\..\bin\%MPT_BIN_CONF%\%MPT_VS_VER%-%MPT_BIN_TARGET%-%MPT_BIN_RUNTIME%\amd64\PluginBridge-amd64.exe .\ || goto error
copy /y ..\..\bin\%MPT_BIN_CONF%\%MPT_VS_VER%-%MPT_BIN_TARGET%-%MPT_BIN_RUNTIME%\amd64\PluginBridge-amd64.pdb .\ || goto error
copy /y ..\..\bin\%MPT_BIN_CONF%\%MPT_VS_VER%-%MPT_BIN_TARGET%-%MPT_BIN_RUNTIME%\%MPT_BIN_ARCH_TRK%\openmpt-wine-support.zip .\ || goto error
..\..\build\tools\7zip\7z.exe a -t%MPT_PKG_FORMAT% -mx=9 ..\openmpt\bin.%MPT_DIST_VARIANT_TRK%\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\openmpt-%MPT_DIST_VARIANT_TRK%-%MPT_REVISION%.%MPT_PKG_FORMAT% ^
 License.txt ^
 Licenses ^
 OpenMPT%MPT_VS_FLAVOUR%.exe ^
 openmpt-lame.dll ^
 openmpt-mpg123.dll ^
 openmpt-soundtouch.dll ^
 PluginBridge-x86.exe ^
 PluginBridge-amd64.exe ^
 openmpt-wine-support.zip ^
 extraKeymaps ^
 || goto error
mkdir ..\openmpt\dbg.%MPT_DIST_VARIANT_TRK%\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\openmpt-%MPT_DIST_VARIANT_TRK%-%MPT_REVISION%-symbols || goto error
..\..\build\tools\7zip\7z.exe a -t%MPT_PKG_FORMAT_SYMBOLS% -mx=9 ..\openmpt\dbg.%MPT_DIST_VARIANT_TRK%\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\openmpt-%MPT_DIST_VARIANT_TRK%-%MPT_REVISION%-symbols\OpenMPT%MPT_VS_FLAVOUR%.pdb.%MPT_PKG_FORMAT_SYMBOLS% OpenMPT%MPT_VS_FLAVOUR%.pdb || goto error
..\..\build\tools\7zip\7z.exe a -t%MPT_PKG_FORMAT_SYMBOLS% -mx=9 ..\openmpt\dbg.%MPT_DIST_VARIANT_TRK%\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\openmpt-%MPT_DIST_VARIANT_TRK%-%MPT_REVISION%-symbols\openmpt-lame.pdb.%MPT_PKG_FORMAT_SYMBOLS% openmpt-lame.pdb || goto error
..\..\build\tools\7zip\7z.exe a -t%MPT_PKG_FORMAT_SYMBOLS% -mx=9 ..\openmpt\dbg.%MPT_DIST_VARIANT_TRK%\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\openmpt-%MPT_DIST_VARIANT_TRK%-%MPT_REVISION%-symbols\openmpt-mpg123.pdb.%MPT_PKG_FORMAT_SYMBOLS% openmpt-mpg123.pdb || goto error
..\..\build\tools\7zip\7z.exe a -t%MPT_PKG_FORMAT_SYMBOLS% -mx=9 ..\openmpt\dbg.%MPT_DIST_VARIANT_TRK%\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\openmpt-%MPT_DIST_VARIANT_TRK%-%MPT_REVISION%-symbols\openmpt-soundtouch.pdb.%MPT_PKG_FORMAT_SYMBOLS% openmpt-soundtouch.pdb || goto error
..\..\build\tools\7zip\7z.exe a -t%MPT_PKG_FORMAT_SYMBOLS% -mx=9 ..\openmpt\dbg.%MPT_DIST_VARIANT_TRK%\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\openmpt-%MPT_DIST_VARIANT_TRK%-%MPT_REVISION%-symbols\PluginBridge-x86.pdb.%MPT_PKG_FORMAT_SYMBOLS% PluginBridge-x86.pdb || goto error
..\..\build\tools\7zip\7z.exe a -t%MPT_PKG_FORMAT_SYMBOLS% -mx=9 ..\openmpt\dbg.%MPT_DIST_VARIANT_TRK%\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\openmpt-%MPT_DIST_VARIANT_TRK%-%MPT_REVISION%-symbols\PluginBridge-amd64.pdb.%MPT_PKG_FORMAT_SYMBOLS% PluginBridge-amd64.pdb || goto error
cd .. || goto error
..\build\tools\7zip\7z.exe a -ttar openmpt-%MPT_DIST_VARIANT_TRK%.tar openmpt || goto error
del /f /q openmpt-%MPT_DIST_VARIANT_TRK%-%MPT_REVISION%.%MPT_PKG_FORMAT%
del /f /q openmpt-%MPT_DIST_VARIANT_TRK%-%MPT_REVISION%-symbols.%MPT_PKG_FORMAT_SYMBOLS%
rmdir /s /q openmpt-%MPT_DIST_VARIANT_TRK%
cd .. || goto error



goto noerror

:error
cd "%MY_DIR%"
exit 1

:noerror
cd "%MY_DIR%"
exit 0
