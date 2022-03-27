@echo off

set BATCH_DIR=%~dp0
cd %BATCH_DIR%
cd ..\..

set MY_DIR=%CD%

call build\auto\setup_vs_any.cmd

call build\auto\helper_get_svnversion.cmd
call build\auto\helper_get_openmpt_version.cmd

set MPT_REVISION=%OPENMPT_VERSION%-%SVNVERSION%
if "x%OPENMPT_VERSION_MINORMINOR%" == "x00" (
	set MPT_REVISION=%OPENMPT_VERSION%
)



cd bin || goto error
rmdir /s /q openmpt
del /f /q openmpt-pkg.win-retro.tar
mkdir openmpt
mkdir openmpt\pkg.win-retro
mkdir openmpt\pkg.win-retro\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%
mkdir openmpt\dbg.win-retro
mkdir openmpt\dbg.win-retro\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-RETRO-Setup.exe                                  openmpt\pkg.win-retro\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-RETRO-Setup.exe
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-RETRO-Setup.exe.digests                          openmpt\pkg.win-retro\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-RETRO-Setup.exe.digests
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-RETRO-Setup.update.json                          openmpt\pkg.win-retro\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-RETRO-Setup.update.json
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-RETRO-portable-x86.zip                           openmpt\pkg.win-retro\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-RETRO-portable-x86.zip
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-RETRO-portable-x86.zip.digests                   openmpt\pkg.win-retro\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-RETRO-portable-x86.zip.digests
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-RETRO-portable-x86.update.json                   openmpt\pkg.win-retro\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-RETRO-portable-x86.update.json
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-RETRO-portable-amd64.zip                         openmpt\pkg.win-retro\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-RETRO-portable-amd64.zip
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-RETRO-portable-amd64.zip.digests                 openmpt\pkg.win-retro\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-RETRO-portable-amd64.zip.digests
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-RETRO-portable-amd64.update.json                 openmpt\pkg.win-retro\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-RETRO-portable-amd64.update.json
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-RETRO-symbols.7z                                 openmpt\dbg.win-retro\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-RETRO-symbols.7z
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-RETRO-symbols.7z.digests                         openmpt\dbg.win-retro\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-RETRO-symbols.7z.digests
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-RETRO-update.json                                openmpt\pkg.win-retro\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-RETRO-update.json
..\build\tools\7zip\7z.exe a -ttar openmpt-pkg.win-retro.tar openmpt || goto error
rmdir /s /q openmpt
cd .. || goto error



goto noerror

:error
cd "%MY_DIR%"
exit 1

:noerror
cd "%MY_DIR%"
exit 0
