@echo off

set GOT_REVISION=%1%
set MY_DIR=%CD%



cd libopenmpt\bin || goto error
del /f /q openmpt-plugins.tar
del /f /q openmpt-plugins-r%GOT_REVISION%.7z
"C:\Program Files\7-Zip\7z.exe" a -t7z -mx=9 openmpt-plugins-r%GOT_REVISION%.7z in_openmpt.dll xmp-openmpt.dll foo_openmpt.dll libopenmpt_settings.dll || goto error
"C:\Program Files\7-Zip\7z.exe" a -ttar openmpt-plugins.tar openmpt-plugins-r%GOT_REVISION%.7z || goto error
del /f /q openmpt-plugins-r%GOT_REVISION%.7z
cd ..\.. || goto error



goto noerror

:error
cd "%MY_DIR%"
exit 1

:noerror
cd "%MY_DIR%"
exit 0
