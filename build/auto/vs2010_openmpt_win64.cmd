@echo off

set MY_DIR=%CD%

call build\auto\setup_vs2010.cmd

call build\auto\prepare_win.cmd



cd mptrack || goto error
devenv MPTRACK_10.sln /clean "Release|x64" || goto error
devenv MPTRACK_10.sln /build "Release|x64" || goto error
cd .. || goto error



goto noerror

:error
cd "%MY_DIR%"
exit 1

:noerror
cd "%MY_DIR%"
exit 0
