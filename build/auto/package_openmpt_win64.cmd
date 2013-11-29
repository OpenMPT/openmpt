@echo off

set GOT_REVISION=%1%
set MY_DIR=%CD%



cd bin\x64 || goto error
del /f /q openmpt-win64.tar
del /f /q openmpt-win64-r%GOT_REVISION%.7z
"C:\Program Files\7-Zip\7z.exe" a -t7z -mx=9 openmpt-win64-r%GOT_REVISION%.7z mptrack.exe OpenMPT_SoundTouch_i16.dll "MIDI Input Output.dll" || goto error
"C:\Program Files\7-Zip\7z.exe" a -ttar openmpt-win64.tar openmpt-win64-r%GOT_REVISION%.7z || goto error
del /f /q openmpt-win64-r%GOT_REVISION%.7z
cd ..\.. || goto error



goto noerror

:error
cd "%MY_DIR%"
exit 1

:noerror
cd "%MY_DIR%"
exit 0
