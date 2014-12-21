@echo off

set BATCH_DIR=%~dp0
cd %BATCH_DIR%
cd ..\..

set GOT_REVISION=%1%
set MY_DIR=%CD%



cd bin\Win32 || goto error
del /f /q libopenmpt-win32.tar
del /f /q libopenmpt-win32-r%GOT_REVISION%.7z
mkdir inc lib bin
mkdir inc\libopenmpt lib\Win32 bin\Win32
copy /y ..\..\libopenmpt\libopenmpt.h inc\libopenmpt\ || goto error
copy /y ..\..\libopenmpt\libopenmpt.hpp inc\libopenmpt\ || goto error
copy /y ..\..\libopenmpt\libopenmpt_config.h inc\libopenmpt\ || goto error
copy /y ..\..\libopenmpt\libopenmpt_version.h inc\libopenmpt\ || goto error
copy /y ..\..\libopenmpt\libopenmpt_ext.hpp inc\libopenmpt\ || goto error
copy /y libopenmpt.lib lib\Win32\ || goto error
copy /y libopenmpt.dll bin\Win32\ || goto error
copy /y ..\..\libopenmpt\doc\in_openmpt.txt .\ || goto error
copy /y ..\..\libopenmpt\doc\xmp-openmpt.txt .\ || goto error
copy /y ..\..\libopenmpt\doc\foo_openmpt.txt .\ || goto error
copy /y ..\..\LICENSE .\ || goto error
copy /y ..\..\libopenmpt\dox\changelog.md .\ || goto error
"C:\Program Files\7-Zip\7z.exe" a -t7z -mx=9 libopenmpt-win32-r%GOT_REVISION%.7z libopenmpt.dll libmodplug.dll in_openmpt.dll xmp-openmpt.dll foo_openmpt.dll openmpt123.exe inc\libopenmpt\libopenmpt.h inc\libopenmpt\libopenmpt.hpp inc\libopenmpt\libopenmpt_ext.hpp inc\libopenmpt\libopenmpt_config.h inc\libopenmpt\libopenmpt_version.h lib\Win32\libopenmpt.lib bin\Win32\libopenmpt.dll in_openmpt.txt xmp-openmpt.txt foo_openmpt.txt LICENSE changelog.md || goto error
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
