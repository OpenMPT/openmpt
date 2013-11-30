@echo off

set GOT_REVISION=%1%
set MY_DIR=%CD%



cd bin\Win32 || goto error
del /f /q libopenmpt-win32.tar
del /f /q libopenmpt-win32-r%GOT_REVISION%.7z
"C:\Program Files\7-Zip\7z.exe" a -t7z -mx=9 libopenmpt-win32-r%GOT_REVISION%.7z libopenmpt.dll libmodplug.dll in_openmpt.dll xmp-openmpt.dll foo_openmpt.dll libopenmpt_settings.dll openmpt123.exe || goto error
"C:\Program Files\7-Zip\7z.exe" a -ttar libopenmpt-win32.tar libopenmpt-win32-r%GOT_REVISION%.7z || goto error
del /f /q libopenmpt-win32-r%GOT_REVISION%.7z
cd ..\.. || goto error



goto noerror

:error
cd "%MY_DIR%"
exit 1

:noerror
cd "%MY_DIR%"
exit 0
