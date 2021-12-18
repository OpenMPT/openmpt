@echo off

set BATCH_DIR=%~dp0
cd %BATCH_DIR%
cd ..\..

set MY_DIR=%CD%



cmd /c build\download_externals.cmd auto nodownload                                                    || goto error
cmd /c build\auto\update_package_template.cmd                                                          || goto error
cmd /c build\auto\build_openmpt_args.cmd vs2019 win10 Win32 Release 7z default                         || goto error
cmd /c build\auto\build_openmpt_args.cmd vs2019 win10 x64   Release 7z default                         || goto error
cmd /c build\auto\build_openmpt_args.cmd vs2019 win10 ARM   Release 7z default                         || goto error
cmd /c build\auto\build_openmpt_args.cmd vs2019 win10 ARM64 Release 7z default                         || goto error
cmd /c build\auto\build_openmpt_args.cmd vs2019 win7  Win32 Release 7z default                         || goto error
cmd /c build\auto\build_openmpt_args.cmd vs2019 win7  x64   Release 7z default                         || goto error
cmd /c build\auto\build_openmpt_release_packages_multiarch.cmd auto sign                               || goto error
cmd /c build\auto\build_openmpt_update_information.cmd auto                                            || goto error
cmd /c build\auto\package_openmpt_installer_multiarch_args.cmd vs2019 win10 Win32 Release 7z default   || goto error



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
