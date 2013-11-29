@echo off

set GOT_REVISION=%1%
set MY_DIR=%CD%



cd openmpt123\bin || goto error
del /f /q openmpt123.tar
del /f /q openmpt123-r%GOT_REVISION%.7z
"C:\Program Files\7-Zip\7z.exe" a -t7z -mx=9 openmpt123-r%GOT_REVISION%.7z openmpt123.exe || goto error
"C:\Program Files\7-Zip\7z.exe" a -ttar openmpt123.tar openmpt123-r%GOT_REVISION%.7z || goto error
del /f /q openmpt123-r%GOT_REVISION%.7z
cd ..\.. || goto error



goto noerror

:error
cd "%MY_DIR%"
exit 1

:noerror
cd "%MY_DIR%"
exit 0
