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



echo "78817a9707c1a02e845fb38b3adcc5353b02d377" > include\genie\OpenMPT-expected.txt
fc include\genie\OpenMPT-expected.txt include\genie\OpenMPT-version.txt
if errorlevel 1 goto errversion
echo "5.0.0-alpha13" > include\premake\OpenMPT-expected.txt
fc include\premake\OpenMPT-expected.txt include\premake\OpenMPT-version.txt
if errorlevel 1 goto errversion



echo dofile "build/genie/genie.lua" > genie.lua || goto err

%GENIE% --target="windesktop81" vs2015 || goto err
%GENIE% --target="winstore82"   vs2015 || goto err

del genie.lua || goto err



echo dofile "build/xcode-genie/genie.lua" > genie.lua || goto err

%GENIE% --target="macosx"   --os=macosx xcode9 || goto err
%GENIE% --target="iphoneos" --os=macosx xcode9 || goto err

del genie.lua || goto err



%PREMAKE% --file=build\vcpkg\premake5.lua vs2017 || goto err



echo dofile "build/premake/premake.lua" > premake5.lua || goto err

%PREMAKE% --group=libopenmpt_test vs2015 || goto err
%PREMAKE% --group=foo_openmpt vs2015 || goto err
%PREMAKE% --group=in_openmpt vs2015 || goto err
%PREMAKE% --group=xmp-openmpt vs2015 || goto err
%PREMAKE% --group=libopenmpt-small vs2015 || goto err
%PREMAKE% --group=libopenmpt_modplug vs2015 || goto err
%PREMAKE% --group=libopenmpt vs2015 || goto err
%PREMAKE% --group=openmpt123 vs2015 || goto err
%PREMAKE% --group=PluginBridge vs2015 || goto err
%PREMAKE% --group=OpenMPT vs2015 || goto err
%PREMAKE% --group=all-externals vs2015 || goto err

%PREMAKE% --group=libopenmpt_test vs2017 || goto err
%PREMAKE% --group=foo_openmpt vs2017 || goto err
%PREMAKE% --group=in_openmpt vs2017 || goto err
%PREMAKE% --group=xmp-openmpt vs2017 || goto err
%PREMAKE% --group=libopenmpt-small vs2017 || goto err
%PREMAKE% --group=libopenmpt_modplug vs2017 || goto err
%PREMAKE% --group=libopenmpt vs2017 || goto err
%PREMAKE% --group=openmpt123 vs2017 || goto err
%PREMAKE% --group=PluginBridge vs2017 || goto err
%PREMAKE% --group=OpenMPT vs2017 || goto err
%PREMAKE% --group=all-externals vs2017 || goto err

%PREMAKE% --group=libopenmpt_test vs2015 --xp || goto err
%PREMAKE% --group=foo_openmpt vs2015 --xp || goto err
%PREMAKE% --group=in_openmpt vs2015 --xp || goto err
%PREMAKE% --group=xmp-openmpt vs2015 --xp || goto err
%PREMAKE% --group=libopenmpt-small vs2015 --xp || goto err
%PREMAKE% --group=libopenmpt_modplug vs2015 --xp || goto err
%PREMAKE% --group=libopenmpt vs2015 --xp || goto err
%PREMAKE% --group=openmpt123 vs2015 --xp || goto err
%PREMAKE% --group=PluginBridge vs2015 --xp || goto err
%PREMAKE% --group=OpenMPT vs2015 --xp || goto err
%PREMAKE% --group=all-externals vs2015 --xp || goto err

%PREMAKE% --group=libopenmpt_test vs2017 --xp || goto err
%PREMAKE% --group=foo_openmpt vs2017 --xp || goto err
%PREMAKE% --group=in_openmpt vs2017 --xp || goto err
%PREMAKE% --group=xmp-openmpt vs2017 --xp || goto err
%PREMAKE% --group=libopenmpt-small vs2017 --xp || goto err
%PREMAKE% --group=libopenmpt_modplug vs2017 --xp || goto err
%PREMAKE% --group=libopenmpt vs2017 --xp || goto err
%PREMAKE% --group=openmpt123 vs2017 --xp || goto err
%PREMAKE% --group=PluginBridge vs2017 --xp || goto err
%PREMAKE% --group=OpenMPT vs2017 --xp || goto err
%PREMAKE% --group=all-externals vs2017 --xp || goto err

%PREMAKE% --group=libopenmpt_test vs2017 --win10 || goto err
%PREMAKE% --group=foo_openmpt vs2017 --win10 || goto err
%PREMAKE% --group=in_openmpt vs2017 --win10 || goto err
%PREMAKE% --group=xmp-openmpt vs2017 --win10 || goto err
%PREMAKE% --group=libopenmpt-small vs2017 --win10 || goto err
%PREMAKE% --group=libopenmpt_modplug vs2017 --win10 || goto err
%PREMAKE% --group=libopenmpt vs2017 --win10 || goto err
%PREMAKE% --group=openmpt123 vs2017 --win10 || goto err
%PREMAKE% --group=PluginBridge vs2017 --win10 || goto err
%PREMAKE% --group=OpenMPT vs2017 --win10 || goto err
%PREMAKE% --group=all-externals vs2017 --win10 || goto err

%PREMAKE% postprocess || goto err

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
