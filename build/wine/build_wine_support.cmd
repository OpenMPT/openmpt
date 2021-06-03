rem @echo off

set CUR_DIR=%CD%

set BATCH_DIR=%~dp0

set INTDIR=%1

set OUTDIR=%2

cd %BATCH_DIR%
cd ..\..

del /f /q "%OUTDIR%\openmpt-wine-support.zip"
"C:\Program Files\7-Zip\7z.exe" a -tzip -mm=Deflate -mx=9 openmpt-wine-support.zip ^
 LICENSE ^
 include\nlohmann-json\LICENSE.MIT ^
 include\nlohmann-json\include\nlohmann\*.hpp ^
 include\nlohmann-json\include\nlohmann\detail\*.hpp ^
 include\nlohmann-json\include\nlohmann\detail\conversions\*.hpp ^
 include\nlohmann-json\include\nlohmann\detail\input\*.hpp ^
 include\nlohmann-json\include\nlohmann\detail\iterators\*.hpp ^
 include\nlohmann-json\include\nlohmann\detail\meta\*.hpp ^
 include\nlohmann-json\include\nlohmann\detail\output\*.hpp ^
 include\nlohmann-json\include\nlohmann\thirdparty\hedley\*.hpp ^
 include\rtkit\rtkit.c ^
 include\rtkit\rtkit.h ^
 src\mpt\audio\*.hpp ^
 src\mpt\base\*.hpp ^
 src\mpt\base\tests\*.hpp ^
 src\mpt\binary\*.hpp ^
 src\mpt\binary\tests\*.hpp ^
 src\mpt\check\*.hpp ^
 src\mpt\crc\*.hpp ^
 src\mpt\crc\tests\*.hpp ^
 src\mpt\crypto\*.hpp ^
 src\mpt\crypto\tests\*.hpp ^
 src\mpt\detect\*.hpp ^
 src\mpt\endian\*.hpp ^
 src\mpt\endian\tests\*.hpp ^
 src\mpt\environment\*.hpp ^
 src\mpt\exception_text\*.hpp ^
 src\mpt\format\*.hpp ^
 src\mpt\format\test\*.hpp ^
 src\mpt\io\*.hpp ^
 src\mpt\io_write\*.hpp ^
 src\mpt\json\*.hpp ^
 src\mpt\library\*.hpp ^
 src\mpt\mutex\*.hpp ^
 src\mpt\osinfo\*.hpp ^
 src\mpt\parse\*.hpp ^
 src\mpt\parse\tests\*.hpp ^
 src\mpt\path\*.hpp ^
 src\mpt\out_of_memory\*.hpp ^
 src\mpt\random\*.hpp ^
 src\mpt\random\tests\*.hpp ^
 src\mpt\string\*.hpp ^
 src\mpt\string\tests\*.hpp ^
 src\mpt\string_convert\*.hpp ^
 src\mpt\string_convert\tests\*.hpp ^
 src\mpt\system_error\*.hpp ^
 src\mpt\test\*.hpp ^
 src\mpt\uuid\*.hpp ^
 src\mpt\uuid\test\*.hpp ^
 src\mpt\uuid_namespace\*.hpp ^
 src\mpt\uuid_namespace\tests\*.hpp ^
 src\openmpt\all\*.cpp ^
 src\openmpt\all\*.hpp ^
 src\openmpt\base\*.cpp ^
 src\openmpt\base\*.hpp ^
 src\openmpt\logging\*.cpp ^
 src\openmpt\logging\*.hpp ^
 src\openmpt\random\*.cpp ^
 src\openmpt\random\*.hpp ^
 src\openmpt\soundbase\*.cpp ^
 src\openmpt\soundbase\*.hpp ^
 src\openmpt\sounddevice\*.cpp ^
 src\openmpt\sounddevice\*.hpp ^
 common\*.h ^
 common\*.cpp ^
 misc\*.h ^
 misc\*.cpp ^
 mptrack\wine\*.h ^
 mptrack\wine\*.cpp ^
 mptrack\wine\*.c ^
 build\wine\dialog.sh ^
 build\wine\native_support.mk ^
 build\wine\wine_wrapper.mk ^
 build\wine\wine_wrapper.spec ^
 build\wine\deps\*.txt ^
 || goto error

cd %BATCH_DIR%
move ..\..\openmpt-wine-support.zip "%OUTDIR%\" || goto error

goto noerror

:error
exit 1

:noerror
exit 0
