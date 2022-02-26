@echo off

set MY_DIR=%CD%

set BATCH_DIR=%~dp0
cd %BATCH_DIR% || goto err
cd .. || goto err


set PREMAKE=
if exist "include\premake\premake5.exe" set PREMAKE=include\premake\premake5.exe
if exist "include\premake\bin\release\premake5.exe" set PREMAKE=include\premake\bin\release\premake5.exe



copy /y include\premake\OpenMPT.txt include\premake\OpenMPT-expected.txt
fc include\premake\OpenMPT-expected.txt include\premake\OpenMPT-version.txt
if errorlevel 1 goto errversion



start cmd /c ^( ^
%PREMAKE% --file=build/premake/premake.lua --group=libopenmpt_test  vs2017 --windows-version=winxp ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=in_openmpt       vs2017 --windows-version=winxp ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=xmp-openmpt      vs2017 --windows-version=winxp ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=libopenmpt-small vs2017 --windows-version=winxp ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=libopenmpt       vs2017 --windows-version=winxp ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=openmpt123       vs2017 --windows-version=winxp ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=PluginBridge     vs2017 --windows-version=winxp ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=OpenMPT          vs2017 --windows-version=winxp ^&^& ^
echo Done ^) ^|^| pause

start cmd /c ^( ^
%PREMAKE% --file=build/premake/premake.lua --group=libopenmpt_test  vs2019 --windows-version=win7 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=in_openmpt       vs2019 --windows-version=win7 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=xmp-openmpt      vs2019 --windows-version=win7 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=libopenmpt-small vs2019 --windows-version=win7 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=libopenmpt       vs2019 --windows-version=win7 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=openmpt123       vs2019 --windows-version=win7 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=PluginBridge     vs2019 --windows-version=win7 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=OpenMPT          vs2019 --windows-version=win7 ^&^& ^
echo Done ^) ^|^| pause"

start cmd /c ^( ^
%PREMAKE% --file=build/premake/premake.lua --group=libopenmpt_test  vs2019 --windows-version=win81 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=in_openmpt       vs2019 --windows-version=win81 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=xmp-openmpt      vs2019 --windows-version=win81 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=libopenmpt-small vs2019 --windows-version=win81 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=libopenmpt       vs2019 --windows-version=win81 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=openmpt123       vs2019 --windows-version=win81 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=PluginBridge     vs2019 --windows-version=win81 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=OpenMPT          vs2019 --windows-version=win81 ^&^& ^
echo Done ^) ^|^| pause"

start cmd /c ^( ^
%PREMAKE% --file=build/premake/premake.lua --group=libopenmpt_test  vs2019 --windows-version=win10 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=in_openmpt       vs2019 --windows-version=win10 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=xmp-openmpt      vs2019 --windows-version=win10 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=libopenmpt-small vs2019 --windows-version=win10 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=libopenmpt       vs2019 --windows-version=win10 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=openmpt123       vs2019 --windows-version=win10 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=PluginBridge     vs2019 --windows-version=win10 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=OpenMPT          vs2019 --windows-version=win10 ^&^& ^
echo Done ^) ^|^| pause"

start cmd /c ^( ^
%PREMAKE% --file=build/premake/premake.lua --group=libopenmpt_test  vs2022 --windows-version=win7 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=in_openmpt       vs2022 --windows-version=win7 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=xmp-openmpt      vs2022 --windows-version=win7 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=libopenmpt-small vs2022 --windows-version=win7 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=libopenmpt       vs2022 --windows-version=win7 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=openmpt123       vs2022 --windows-version=win7 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=PluginBridge     vs2022 --windows-version=win7 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=OpenMPT          vs2022 --windows-version=win7 ^&^& ^
echo Done ^) ^|^| pause"

start cmd /c ^( ^
%PREMAKE% --file=build/premake/premake.lua --group=libopenmpt_test  vs2022 --windows-version=win81 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=in_openmpt       vs2022 --windows-version=win81 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=xmp-openmpt      vs2022 --windows-version=win81 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=libopenmpt-small vs2022 --windows-version=win81 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=libopenmpt       vs2022 --windows-version=win81 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=openmpt123       vs2022 --windows-version=win81 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=PluginBridge     vs2022 --windows-version=win81 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=OpenMPT          vs2022 --windows-version=win81 ^&^& ^
echo Done ^) ^|^| pause"

start cmd /c ^( ^
%PREMAKE% --file=build/premake/premake.lua --group=libopenmpt_test  vs2022 --windows-version=win10 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=in_openmpt       vs2022 --windows-version=win10 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=xmp-openmpt      vs2022 --windows-version=win10 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=libopenmpt-small vs2022 --windows-version=win10 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=libopenmpt       vs2022 --windows-version=win10 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=openmpt123       vs2022 --windows-version=win10 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=PluginBridge     vs2022 --windows-version=win10 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=OpenMPT          vs2022 --windows-version=win10 ^&^& ^
echo Done ^) ^|^| pause"

start cmd /c ^( ^
%PREMAKE% --file=build/premake/premake.lua --group=libopenmpt_test  vs2022 --clang --windows-version=win10 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=in_openmpt       vs2022 --clang --windows-version=win10 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=xmp-openmpt      vs2022 --clang --windows-version=win10 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=libopenmpt-small vs2022 --clang --windows-version=win10 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=libopenmpt       vs2022 --clang --windows-version=win10 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=openmpt123       vs2022 --clang --windows-version=win10 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=PluginBridge     vs2022 --clang --windows-version=win10 ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=OpenMPT          vs2022 --clang --windows-version=win10 ^&^& ^
echo Done ^) ^|^| pause"

start cmd /c ^( ^
%PREMAKE% --file=build/premake/premake.lua --group=libopenmpt-small vs2019 --windows-version=win10 --windows-family=uwp ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=libopenmpt       vs2019 --windows-version=win10 --windows-family=uwp ^&^& ^
echo Done ^) ^|^| pause"

start cmd /c ^( ^
%PREMAKE% --file=build/premake/premake.lua --group=libopenmpt-small vs2022 --windows-version=win10 --windows-family=uwp ^&^& ^
%PREMAKE% --file=build/premake/premake.lua --group=libopenmpt       vs2022 --windows-version=win10 --windows-family=uwp ^&^& ^
echo Done ^) ^|^| pause"

start cmd /c ^( ^
%PREMAKE% --file=build/premake-xcode/premake.lua --target=macosx xcode4 ^&^& ^
%PREMAKE% --file=build/premake-xcode/premake.lua --target=ios    xcode4 ^&^& ^
echo Done ^) ^|^| pause"



cd %MY_DIR% || goto err

goto end

:errversion
echo Premake version mismatch
goto err

:err
echo ERROR!
goto end

:end
cd %MY_DIR%
pause
