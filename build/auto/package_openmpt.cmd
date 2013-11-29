@echo off

set GOT_REVISION=%1%
set MY_DIR=%CD%



cd bin\Win32 || goto error
del /f /q openmpt.tar
del /f /q openmpt-r%GOT_REVISION%.7z
"C:\Program Files\7-Zip\7z.exe" a -t7z -mx=9 openmpt-r%GOT_REVISION%.7z mptrack.exe OpenMPT_SoundTouch_i16.dll || goto error
"C:\Program Files\7-Zip\7z.exe" a -ttar openmpt.tar openmpt-r%GOT_REVISION%.7z || goto error
del /f /q openmpt-r%GOT_REVISION%.7z
cd ..\.. || goto error



goto noerror

:error
cd "%MY_DIR%"
exit 1

:noerror
cd "%MY_DIR%"
exit 0
