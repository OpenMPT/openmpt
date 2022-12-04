@echo off

set BATCH_DIR=%~dp0
cd %BATCH_DIR%
cd ..\..

set MY_DIR=%CD%



cmd /c build\download_externals.cmd auto nodownload                                                    || goto error
cmd /c build\auto\update_package_template_retro.cmd                                                    || goto error
rem cmd /c build\auto\build_openmpt_args.cmd vs2017 winxp Win32 Release 7z default                         || goto error
rem cmd /c build\auto\build_openmpt_args.cmd vs2017 winxp x64   Release 7z default                         || goto error
cmd /c build\auto\build_openmpt_release_packages_retro.cmd auto                                        || goto error
cmd /c build\auto\build_openmpt_update_information_retro.cmd auto                                      || goto error
cmd /c build\auto\package_openmpt_installer_retro_args.cmd vs2017 winxp Win32 Release 7z default       || goto error



goto noerror

:error
cd "%MY_DIR%"
echo "FAILED."
pause
exit 1

:noerror
cd "%MY_DIR%"
echo "ALL OK."
pause
exit 0
