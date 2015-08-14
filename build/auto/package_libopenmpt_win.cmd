@echo off

set BATCH_DIR=%~dp0
cd %BATCH_DIR%
cd ..\..

set MY_DIR=%CD%

call build\auto\helper_get_svnversion.cmd

set MPT_REVISION=%SVNVERSION%



cd bin || goto error
rmdir /s /q libopenmpt-win
del /f /q libopenmpt-win.tar
del /f /q libopenmpt-win-r%MPT_REVISION%.7z
mkdir libopenmpt-win
cd libopenmpt-win || goto error
mkdir openmpt123
mkdir openmpt123\x86
mkdir openmpt123\x86_64
mkdir XMPlay
mkdir Winamp
mkdir foobar2000
copy /y ..\..\LICENSE .\LICENSE.txt || goto error
copy /y ..\..\libopenmpt\dox\changelog.md .\ || goto error
copy /y ..\..\libopenmpt\doc\xmp-openmpt.txt .\XMPlay\ || goto error
copy /y ..\..\libopenmpt\doc\in_openmpt.txt .\Winamp\ || goto error
copy /y ..\..\libopenmpt\doc\foo_openmpt.txt .\foobar2000\ || goto error
copy /y ..\..\bin\Win32\openmpt123.exe .\openmpt123\x86\ || goto error
copy /y ..\..\bin\x64\openmpt123.exe .\openmpt123\x86_64\ || goto error
copy /y ..\..\bin\Win32\xmp-openmpt.dll .\XMPlay\ || goto error
copy /y ..\..\bin\Win32\in_openmpt.dll .\Winamp\ || goto error
copy /y ..\..\bin\Win32\foo_openmpt.dll .\foobar2000\ || goto error
"C:\Program Files\7-Zip\7z.exe" a -t7z -mx=9 ..\libopenmpt-win-r%MPT_REVISION%.7z ^
 LICENSE.txt ^
 changelog.md ^
 openmpt123\x86\openmpt123.exe ^
 openmpt123\x86_64\openmpt123.exe ^
 XMPlay\xmp-openmpt.txt ^
 XMPlay\xmp-openmpt.dll ^
 Winamp\in_openmpt.txt ^
 Winamp\in_openmpt.dll ^
 foobar2000\foo_openmpt.txt ^
 foobar2000\foo_openmpt.dll ^
 || goto error
cd .. || goto error
"C:\Program Files\7-Zip\7z.exe" a -ttar libopenmpt-win.tar libopenmpt-win-r%MPT_REVISION%.7z || goto error
del /f /q libopenmpt-win-r%MPT_REVISION%.7z
cd .. || goto error

cd bin || goto error
rmdir /s /q libopenmpt-dev-vs2010
del /f /q libopenmpt-dev-vs2010.tar
del /f /q libopenmpt-dev-vs2010-r%MPT_REVISION%.7z
mkdir libopenmpt-dev-vs2010
cd libopenmpt-dev-vs2010 || goto error
mkdir inc
mkdir inc\libopenmpt
mkdir lib
mkdir lib\x86
mkdir lib\x86_64
mkdir bin
mkdir bin\x86
mkdir bin\x86_64
copy /y ..\..\LICENSE .\LICENSE.txt || goto error
copy /y ..\..\libopenmpt\dox\changelog.md .\changelog.md || goto error
copy /y ..\..\libopenmpt\libopenmpt.h inc\libopenmpt\ || goto error
copy /y ..\..\libopenmpt\libopenmpt.hpp inc\libopenmpt\ || goto error
copy /y ..\..\libopenmpt\libopenmpt_config.h inc\libopenmpt\ || goto error
copy /y ..\..\libopenmpt\libopenmpt_version.h inc\libopenmpt\ || goto error
copy /y ..\..\libopenmpt\libopenmpt_ext.hpp inc\libopenmpt\ || goto error
copy /y ..\..\bin\Win32\libopenmpt.lib lib\x86\ || goto error
copy /y ..\..\bin\Win32\libopenmpt.dll bin\x86\ || goto error
copy /y ..\..\bin\x64\libopenmpt.lib lib\x86_64\ || goto error
copy /y ..\..\bin\x64\libopenmpt.dll bin\x86_64\ || goto error
"C:\Program Files\7-Zip\7z.exe" a -t7z -mx=9 ..\libopenmpt-dev-vs2010-r%MPT_REVISION%.7z ^
 LICENSE.txt ^
 changelog.md ^
 inc\libopenmpt\libopenmpt.h ^
 inc\libopenmpt\libopenmpt.hpp ^
 inc\libopenmpt\libopenmpt_config.h ^
 inc\libopenmpt\libopenmpt_version.h ^
 inc\libopenmpt\libopenmpt_ext.hpp ^
 lib\x86\libopenmpt.lib ^
 lib\x86_64\libopenmpt.lib ^
 bin\x86\libopenmpt.dll ^
 bin\x86_64\libopenmpt.dll ^
 || goto error
cd .. || goto error
"C:\Program Files\7-Zip\7z.exe" a -ttar libopenmpt-dev-vs2010.tar libopenmpt-dev-vs2010-r%MPT_REVISION%.7z || goto error
del /f /q libopenmpt-dev-vs2010-r%MPT_REVISION%.7z
cd .. || goto error



goto noerror

:error
cd "%MY_DIR%"
exit 1

:noerror
cd "%MY_DIR%"
exit 0
