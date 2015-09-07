@echo off

set BATCH_DIR=%~dp0
cd %BATCH_DIR%
cd ..\..

set MY_DIR=%CD%

call build\auto\helper_get_svnversion.cmd

set MPT_REVISION=%SVNVERSION%



cd bin || goto error
rmdir /s /q libopenmpt-winold
del /f /q libopenmpt-winold.tar
del /f /q libopenmpt-winold-r%MPT_REVISION%.7z
mkdir libopenmpt-winold
cd libopenmpt-winold || goto error
mkdir openmpt123
mkdir XMPlay
mkdir Winamp
copy /y ..\..\LICENSE .\LICENSE.txt || goto error
copy /y ..\..\libopenmpt\dox\changelog.md .\ || goto error
copy /y ..\..\libopenmpt\doc\xmp-openmpt.txt .\XMPlay\ || goto error
copy /y ..\..\libopenmpt\doc\in_openmpt.txt .\Winamp\ || goto error
copy /y ..\..\bin\Win32\openmpt123.exe .\openmpt123\ || goto error
copy /y ..\..\bin\Win32\xmp-openmpt.dll .\XMPlay\ || goto error
copy /y ..\..\bin\Win32\in_openmpt.dll .\Winamp\ || goto error
"C:\Program Files\7-Zip\7z.exe" a -t7z -mx=9 ..\libopenmpt-winold-r%MPT_REVISION%.7z ^
 LICENSE.txt ^
 changelog.md ^
 openmpt123\openmpt123.exe ^
 XMPlay\xmp-openmpt.txt ^
 XMPlay\xmp-openmpt.dll ^
 Winamp\in_openmpt.txt ^
 Winamp\in_openmpt.dll ^
 || goto error
cd .. || goto error
"C:\Program Files\7-Zip\7z.exe" a -ttar libopenmpt-winold.tar libopenmpt-winold-r%MPT_REVISION%.7z || goto error
del /f /q libopenmpt-winold-r%MPT_REVISION%.7z
cd .. || goto error



goto noerror

:error
cd "%MY_DIR%"
exit 1

:noerror
cd "%MY_DIR%"
exit 0
