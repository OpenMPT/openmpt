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



cd bin || goto error
rmdir /s /q openmpt
del /f /q openmpt-pkg.win-multi.tar
mkdir openmpt
mkdir openmpt\pkg.win
mkdir openmpt\pkg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-Setup.exe                  openmpt\pkg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%OPENMPT_VERSION%-%SVNVERSION%-Setup.exe
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-Setup.exe.digests          openmpt\pkg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%OPENMPT_VERSION%-%SVNVERSION%-Setup.exe.digests
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-portable-x86.zip           openmpt\pkg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%OPENMPT_VERSION%-%SVNVERSION%-portable-x86.zip
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-portable-x86.zip.digests   openmpt\pkg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%OPENMPT_VERSION%-%SVNVERSION%-portable-x86.zip.digests
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-portable-amd64.zip         openmpt\pkg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%OPENMPT_VERSION%-%SVNVERSION%-portable-amd64.zip
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-portable-amd64.zip.digests openmpt\pkg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%OPENMPT_VERSION%-%SVNVERSION%-portable-amd64.zip.digests
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-portable-arm.zip           openmpt\pkg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%OPENMPT_VERSION%-%SVNVERSION%-portable-arm.zip
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-portable-arm.zip.digests   openmpt\pkg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%OPENMPT_VERSION%-%SVNVERSION%-portable-arm.zip.digests
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-portable-arm64.zip         openmpt\pkg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%OPENMPT_VERSION%-%SVNVERSION%-portable-arm64.zip
copy /y ..\installer\OpenMPT-%OPENMPT_VERSION%-portable-arm64.zip.digests openmpt\pkg.win\%OPENMPT_VERSION_MAJORMAJOR%.%OPENMPT_VERSION_MAJOR%\OpenMPT-%OPENMPT_VERSION%-%SVNVERSION%-portable-arm64.zip.digests
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
