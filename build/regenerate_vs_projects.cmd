@echo off

set MY_DIR=%CD%

set BATCH_DIR=%~dp0
cd %BATCH_DIR% || goto err
cd .. || goto err


set PREMAKE=
if exist "include\premake\premake5.exe" set PREMAKE=include\premake\premake5.exe
if exist "include\premake\bin\release\premake5.exe" set PREMAKE=include\premake\bin\release\premake5.exe

set GENIE=
set GENIE=include\genie\bin\windows\genie.exe



copy /y include\genie\OpenMPT.txt include\genie\OpenMPT-expected.txt
fc include\genie\OpenMPT-expected.txt include\genie\OpenMPT-version.txt
if errorlevel 1 goto errversion
copy /y include\premake\OpenMPT.txt include\premake\OpenMPT-expected.txt
fc include\premake\OpenMPT-expected.txt include\premake\OpenMPT-version.txt
if errorlevel 1 goto errversion



echo dofile "build/premake/premake.lua" > premake5.lua || goto err

start cmd /c ^( ^
%PREMAKE% --group=libopenmpt_test vs2017 --winxp ^&^& ^
%PREMAKE% --group=in_openmpt vs2017 --winxp ^&^& ^
%PREMAKE% --group=xmp-openmpt vs2017 --winxp ^&^& ^
%PREMAKE% --group=libopenmpt-small vs2017 --winxp ^&^& ^
%PREMAKE% --group=libopenmpt vs2017 --winxp ^&^& ^
%PREMAKE% --group=openmpt123 vs2017 --winxp ^&^& ^
%PREMAKE% --group=PluginBridge vs2017 --winxp ^&^& ^
%PREMAKE% --group=OpenMPT vs2017 --winxp ^&^& ^
%PREMAKE% --group=all-externals vs2017 --winxp ^&^& ^
echo Done ^) ^|^| pause

start cmd /c ^( ^
%PREMAKE% --group=libopenmpt_test vs2019 --win7 ^&^& ^
%PREMAKE% --group=in_openmpt vs2019 --win7 ^&^& ^
%PREMAKE% --group=xmp-openmpt vs2019 --win7 ^&^& ^
%PREMAKE% --group=libopenmpt-small vs2019 --win7 ^&^& ^
%PREMAKE% --group=libopenmpt vs2019 --win7 ^&^& ^
%PREMAKE% --group=openmpt123 vs2019 --win7 ^&^& ^
%PREMAKE% --group=PluginBridge vs2019 --win7 ^&^& ^
%PREMAKE% --group=OpenMPT vs2019 --win7 ^&^& ^
%PREMAKE% --group=all-externals vs2019 --win7 ^&^& ^
echo Done ^) ^|^| pause"

start cmd /c ^( ^
%PREMAKE% --group=libopenmpt_test vs2019 --win81 ^&^& ^
%PREMAKE% --group=in_openmpt vs2019 --win81 ^&^& ^
%PREMAKE% --group=xmp-openmpt vs2019 --win81 ^&^& ^
%PREMAKE% --group=libopenmpt-small vs2019 --win81 ^&^& ^
%PREMAKE% --group=libopenmpt vs2019 --win81 ^&^& ^
%PREMAKE% --group=openmpt123 vs2019 --win81 ^&^& ^
%PREMAKE% --group=PluginBridge vs2019 --win81 ^&^& ^
%PREMAKE% --group=OpenMPT vs2019 --win81 ^&^& ^
%PREMAKE% --group=all-externals vs2019 --win81 ^&^& ^
echo Done ^) ^|^| pause"

start cmd /c ^( ^
%PREMAKE% --group=libopenmpt_test vs2019 --win10 ^&^& ^
%PREMAKE% --group=in_openmpt vs2019 --win10 ^&^& ^
%PREMAKE% --group=xmp-openmpt vs2019 --win10 ^&^& ^
%PREMAKE% --group=libopenmpt-small vs2019 --win10 ^&^& ^
%PREMAKE% --group=libopenmpt vs2019 --win10 ^&^& ^
%PREMAKE% --group=openmpt123 vs2019 --win10 ^&^& ^
%PREMAKE% --group=PluginBridge vs2019 --win10 ^&^& ^
%PREMAKE% --group=OpenMPT vs2019 --win10 ^&^& ^
%PREMAKE% --group=all-externals vs2019 --win10 ^&^& ^
echo Done ^) ^|^| pause"

start cmd /c ^( ^
%PREMAKE% --group=libopenmpt_test vs2022 --win7 ^&^& ^
%PREMAKE% --group=in_openmpt vs2022 --win7 ^&^& ^
%PREMAKE% --group=xmp-openmpt vs2022 --win7 ^&^& ^
%PREMAKE% --group=libopenmpt-small vs2022 --win7 ^&^& ^
%PREMAKE% --group=libopenmpt vs2022 --win7 ^&^& ^
%PREMAKE% --group=openmpt123 vs2022 --win7 ^&^& ^
%PREMAKE% --group=PluginBridge vs2022 --win7 ^&^& ^
%PREMAKE% --group=OpenMPT vs2022 --win7 ^&^& ^
%PREMAKE% --group=all-externals vs2022 --win7 ^&^& ^
echo Done ^) ^|^| pause"

start cmd /c ^( ^
%PREMAKE% --group=libopenmpt_test vs2022 --win81 ^&^& ^
%PREMAKE% --group=in_openmpt vs2022 --win81 ^&^& ^
%PREMAKE% --group=xmp-openmpt vs2022 --win81 ^&^& ^
%PREMAKE% --group=libopenmpt-small vs2022 --win81 ^&^& ^
%PREMAKE% --group=libopenmpt vs2022 --win81 ^&^& ^
%PREMAKE% --group=openmpt123 vs2022 --win81 ^&^& ^
%PREMAKE% --group=PluginBridge vs2022 --win81 ^&^& ^
%PREMAKE% --group=OpenMPT vs2022 --win81 ^&^& ^
%PREMAKE% --group=all-externals vs2022 --win81 ^&^& ^
echo Done ^) ^|^| pause"

start cmd /c ^( ^
%PREMAKE% --group=libopenmpt_test vs2022 --win10 ^&^& ^
%PREMAKE% --group=in_openmpt vs2022 --win10 ^&^& ^
%PREMAKE% --group=xmp-openmpt vs2022 --win10 ^&^& ^
%PREMAKE% --group=libopenmpt-small vs2022 --win10 ^&^& ^
%PREMAKE% --group=libopenmpt vs2022 --win10 ^&^& ^
%PREMAKE% --group=openmpt123 vs2022 --win10 ^&^& ^
%PREMAKE% --group=PluginBridge vs2022 --win10 ^&^& ^
%PREMAKE% --group=OpenMPT vs2022 --win10 ^&^& ^
%PREMAKE% --group=all-externals vs2022 --win10 ^&^& ^
echo Done ^) ^|^| pause"

start cmd /c ^( ^
%PREMAKE% --group=libopenmpt_test vs2022 --clang --win10 ^&^& ^
%PREMAKE% --group=in_openmpt vs2022 --clang --win10 ^&^& ^
%PREMAKE% --group=xmp-openmpt vs2022 --clang --win10 ^&^& ^
%PREMAKE% --group=libopenmpt-small vs2022 --clang --win10 ^&^& ^
%PREMAKE% --group=libopenmpt vs2022 --clang --win10 ^&^& ^
%PREMAKE% --group=openmpt123 vs2022 --clang --win10 ^&^& ^
%PREMAKE% --group=PluginBridge vs2022 --clang --win10 ^&^& ^
%PREMAKE% --group=OpenMPT vs2022 --clang --win10 ^&^& ^
%PREMAKE% --group=all-externals vs2022 --clang --win10 ^&^& ^
echo Done ^) ^|^| pause"

start cmd /c ^( ^
%PREMAKE% --group=libopenmpt-small vs2019 --win10 --uwp ^&^& ^
%PREMAKE% --group=libopenmpt vs2019 --win10 --uwp ^&^& ^
%PREMAKE% --group=all-externals vs2019 --win10 --uwp ^&^& ^
echo Done ^) ^|^| pause"

start cmd /c ^( ^
%PREMAKE% --group=libopenmpt-small vs2022 --win10 --uwp ^&^& ^
%PREMAKE% --group=libopenmpt vs2022 --win10 --uwp ^&^& ^
%PREMAKE% --group=all-externals vs2022 --win10 --uwp ^&^& ^
echo Done ^) ^|^| pause"



echo dofile "build/xcode-genie/genie.lua" > genie.lua || goto err

%GENIE% --target="macosx"   --os=macosx xcode9 || goto err
%GENIE% --target="iphoneos" --os=macosx xcode9 || goto err



cd %MY_DIR% || goto err

goto end

:errversion
echo Genie or Premake version mismatch
goto err

:err
echo ERROR!
goto end

:end
cd %MY_DIR%
pause
