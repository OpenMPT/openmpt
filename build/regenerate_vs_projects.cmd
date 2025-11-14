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
%PREMAKE% --file=build/premake/premake.lua --os=windows vs2017 --windows-version=winxp --windows-family=desktop --windows-charset=MBCS ^&^& ^
echo Done ^) ^|^| pause

start cmd /c ^( ^
%PREMAKE% --file=build/premake/premake.lua --os=windows vs2017 --windows-version=winxp --windows-family=desktop --windows-charset=Unicode ^&^& ^
echo Done ^) ^|^| pause

start cmd /c ^( ^
%PREMAKE% --file=build/premake/premake.lua --os=windows vs2017 --windows-version=winxpx64 --windows-family=desktop --windows-charset=Unicode ^&^& ^
echo Done ^) ^|^| pause

start cmd /c ^( ^
%PREMAKE% --file=build/premake/premake.lua --os=windows vs2019 --windows-version=win7 --windows-family=desktop --windows-charset=Unicode ^&^& ^
echo Done ^) ^|^| pause

start cmd /c ^( ^
%PREMAKE% --file=build/premake/premake.lua --os=windows vs2022 --windows-version=win7 --windows-family=desktop --windows-charset=Unicode ^&^& ^
echo Done ^) ^|^| pause

start cmd /c ^( ^
%PREMAKE% --file=build/premake/premake.lua --os=windows vs2022 --windows-version=win8 --windows-family=desktop --windows-charset=Unicode ^&^& ^
echo Done ^) ^|^| pause

start cmd /c ^( ^
%PREMAKE% --file=build/premake/premake.lua --os=windows vs2022 --windows-version=win81 --windows-family=desktop --windows-charset=Unicode ^&^& ^
echo Done ^) ^|^| pause

start cmd /c ^( ^
%PREMAKE% --file=build/premake/premake.lua --os=windows vs2022 --windows-version=win10 --windows-family=desktop --windows-charset=Unicode ^&^& ^
echo Done ^) ^|^| pause

start cmd /c ^( ^
%PREMAKE% --file=build/premake/premake.lua --os=windows vs2022 --windows-version=win11 --windows-family=desktop --windows-charset=Unicode ^&^& ^
echo Done ^) ^|^| pause

start cmd /c ^( ^
%PREMAKE% --file=build/premake/premake.lua --os=windows vs2026 --windows-version=win11 --windows-family=desktop --windows-charset=Unicode ^&^& ^
echo Done ^) ^|^| pause

start cmd /c ^( ^
%PREMAKE% --file=build/premake/premake.lua --os=windows vs2022 --clang --windows-version=win11 --windows-family=desktop --windows-charset=Unicode ^&^& ^
echo Done ^) ^|^| pause

start cmd /c ^( ^
%PREMAKE% --file=build/premake/premake.lua --os=windows vs2026 --clang --windows-version=win11 --windows-family=desktop --windows-charset=Unicode ^&^& ^
echo Done ^) ^|^| pause

start cmd /c ^( ^
%PREMAKE% --file=build/premake/premake.lua --os=windows vs2022 --windows-version=win10 --windows-family=uwp --windows-charset=Unicode ^&^& ^
echo Done ^) ^|^| pause

start cmd /c ^( ^
%PREMAKE% --file=build/premake/premake.lua --os=windows vs2022 --windows-version=win11 --windows-family=uwp --windows-charset=Unicode ^&^& ^
echo Done ^) ^|^| pause

start cmd /c ^( ^
%PREMAKE% --file=build/premake/premake.lua --os=windows vs2026 --windows-version=win11 --windows-family=uwp --windows-charset=Unicode ^&^& ^
echo Done ^) ^|^| pause

start cmd /c ^( ^
%PREMAKE% --file=build/premake-xcode/premake.lua --target=macosx xcode4 ^&^& ^
%PREMAKE% --file=build/premake-xcode/premake.lua --target=ios    xcode4 ^&^& ^
echo Done ^) ^|^| pause



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
