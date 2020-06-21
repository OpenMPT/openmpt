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
del /f /q openmpt-pkg.win-multi.tar
mkdir openmpt
mkdir openmpt\pkg.win
mkdir openmpt\pkg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%
mkdir openmpt\dbg.win
mkdir openmpt\dbg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-Setup.exe                         openmpt\pkg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-Setup.exe
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-Setup.exe.digests                 openmpt\pkg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-Setup.exe.digests
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-Setup.update.json                 openmpt\pkg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-Setup.update.json
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-portable-x86.zip                  openmpt\pkg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-portable-x86.zip
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-portable-x86.zip.digests          openmpt\pkg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-portable-x86.zip.digests
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-portable-x86.update.json          openmpt\pkg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-portable-x86.update.json
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-portable-x86-legacy.zip           openmpt\pkg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-portable-x86-legacy.zip
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-portable-x86-legacy.zip.digests   openmpt\pkg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-portable-x86-legacy.zip.digests
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-portable-x86-legacy.update.json   openmpt\pkg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-portable-x86-legacy.update.json
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-portable-amd64.zip                openmpt\pkg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-portable-amd64.zip
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-portable-amd64.zip.digests        openmpt\pkg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-portable-amd64.zip.digests
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-portable-amd64.update.json        openmpt\pkg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-portable-amd64.update.json
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-portable-amd64-legacy.zip         openmpt\pkg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-portable-amd64-legacy.zip
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-portable-amd64-legacy.zip.digests openmpt\pkg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-portable-amd64-legacy.zip.digests
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-portable-amd64-legacy.update.json openmpt\pkg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-portable-amd64-legacy.update.json
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-portable-arm.zip                  openmpt\pkg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-portable-arm.zip
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-portable-arm.zip.digests          openmpt\pkg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-portable-arm.zip.digests
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-portable-arm.update.json          openmpt\pkg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-portable-arm.update.json
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-portable-arm64.zip                openmpt\pkg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-portable-arm64.zip
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-portable-arm64.zip.digests        openmpt\pkg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-portable-arm64.zip.digests
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-portable-arm64.update.json        openmpt\pkg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-portable-arm64.update.json
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-symbols.7z                        openmpt\dbg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-symbols.7z
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-symbols.7z.digests                openmpt\dbg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-symbols.7z.digests
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-update.json                       openmpt\pkg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%MPT_REVISION%-update.json
..\build\tools\7zip\7z.exe a -ttar openmpt-pkg.win-multi.tar openmpt || goto error
rmdir /s /q openmpt
cd .. || goto error



goto noerror

:error
cd "%MY_DIR%"
exit 1

:noerror
cd "%MY_DIR%"
exit 0
