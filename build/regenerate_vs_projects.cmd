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



echo dofile "build/genie/genie.lua" > genie.lua || goto err

%GENIE% --target="winstore82"   vs2017 || goto err

%GENIE% --target="winstore10"   vs2019 || goto err

del genie.lua || goto err



echo dofile "build/xcode-genie/genie.lua" > genie.lua || goto err

%GENIE% --target="macosx"   --os=macosx xcode9 || goto err
%GENIE% --target="iphoneos" --os=macosx xcode9 || goto err

del genie.lua || goto err



%PREMAKE% --file=build\vcpkg\premake5.lua vs2017 || goto err



echo dofile "build/premake/premake.lua" > premake5.lua || goto err

%PREMAKE% --group=libopenmpt_test vs2017 --win7 || goto err
%PREMAKE% --group=in_openmpt vs2017 --win7 || goto err
%PREMAKE% --group=xmp-openmpt vs2017 --win7 || goto err
%PREMAKE% --group=libopenmpt-small vs2017 --win7 || goto err
%PREMAKE% --group=libopenmpt vs2017 --win7 || goto err
%PREMAKE% --group=openmpt123 vs2017 --win7 || goto err
%PREMAKE% --group=PluginBridge vs2017 --win7 || goto err
%PREMAKE% --group=OpenMPT vs2017 --win7 || goto err
%PREMAKE% --group=all-externals vs2017 --win7 || goto err

%PREMAKE% --group=libopenmpt_test vs2017 --win10 || goto err
%PREMAKE% --group=in_openmpt vs2017 --win10 || goto err
%PREMAKE% --group=xmp-openmpt vs2017 --win10 || goto err
%PREMAKE% --group=libopenmpt-small vs2017 --win10 || goto err
%PREMAKE% --group=libopenmpt vs2017 --win10 || goto err
%PREMAKE% --group=openmpt123 vs2017 --win10 || goto err
%PREMAKE% --group=PluginBridge vs2017 --win10 || goto err
%PREMAKE% --group=OpenMPT vs2017 --win10 || goto err
%PREMAKE% --group=all-externals vs2017 --win10 || goto err

%PREMAKE% --group=libopenmpt_test vs2019 --win7 || goto err
%PREMAKE% --group=in_openmpt vs2019 --win7 || goto err
%PREMAKE% --group=xmp-openmpt vs2019 --win7 || goto err
%PREMAKE% --group=libopenmpt-small vs2019 --win7 || goto err
%PREMAKE% --group=libopenmpt vs2019 --win7 || goto err
%PREMAKE% --group=openmpt123 vs2019 --win7 || goto err
%PREMAKE% --group=PluginBridge vs2019 --win7 || goto err
%PREMAKE% --group=OpenMPT vs2019 --win7 || goto err
%PREMAKE% --group=all-externals vs2019 --win7 || goto err

%PREMAKE% --group=libopenmpt_test vs2019 --win81 || goto err
%PREMAKE% --group=in_openmpt vs2019 --win81 || goto err
%PREMAKE% --group=xmp-openmpt vs2019 --win81 || goto err
%PREMAKE% --group=libopenmpt-small vs2019 --win81 || goto err
%PREMAKE% --group=libopenmpt vs2019 --win81 || goto err
%PREMAKE% --group=openmpt123 vs2019 --win81 || goto err
%PREMAKE% --group=PluginBridge vs2019 --win81 || goto err
%PREMAKE% --group=OpenMPT vs2019 --win81 || goto err
%PREMAKE% --group=all-externals vs2019 --win81 || goto err

%PREMAKE% --group=libopenmpt_test vs2019 --win10 || goto err
%PREMAKE% --group=in_openmpt vs2019 --win10 || goto err
%PREMAKE% --group=xmp-openmpt vs2019 --win10 || goto err
%PREMAKE% --group=libopenmpt-small vs2019 --win10 || goto err
%PREMAKE% --group=libopenmpt vs2019 --win10 || goto err
%PREMAKE% --group=openmpt123 vs2019 --win10 || goto err
%PREMAKE% --group=PluginBridge vs2019 --win10 || goto err
%PREMAKE% --group=OpenMPT vs2019 --win10 || goto err
%PREMAKE% --group=all-externals vs2019 --win10 || goto err

%PREMAKE% --group=libopenmpt_test vs2019 --clang --win10 || goto err
%PREMAKE% --group=in_openmpt vs2019 --clang --win10 || goto err
%PREMAKE% --group=xmp-openmpt vs2019 --clang --win10 || goto err
%PREMAKE% --group=libopenmpt-small vs2019 --clang --win10 || goto err
%PREMAKE% --group=libopenmpt vs2019 --clang --win10 || goto err
%PREMAKE% --group=openmpt123 vs2019 --clang --win10 || goto err
%PREMAKE% --group=PluginBridge vs2019 --clang --win10 || goto err
%PREMAKE% --group=OpenMPT vs2019 --clang --win10 || goto err
%PREMAKE% --group=all-externals vs2019 --clang --win10 || goto err

del premake5.lua || goto err



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
