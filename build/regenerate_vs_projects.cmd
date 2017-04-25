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



echo dofile "build/genie/genie.lua" > genie.lua || goto err

%GENIE% --target="windesktop81" vs2015 || goto err
%GENIE% --target="winstore82"   vs2015 || goto err

del genie.lua || goto err



echo dofile "build/premake/premake.lua" > premake5.lua || goto err

%PREMAKE% --group=libopenmpt_test vs2015 || goto err
%PREMAKE% --group=foo_openmpt vs2015 || goto err
%PREMAKE% --group=in_openmpt vs2015 || goto err
%PREMAKE% --group=xmp-openmpt vs2015 || goto err
%PREMAKE% --group=libopenmpt-full vs2015 || goto err
%PREMAKE% --group=libopenmpt-small vs2015 || goto err
%PREMAKE% --group=libopenmpt vs2015 || goto err
%PREMAKE% --group=openmpt123 vs2015 || goto err
%PREMAKE% --group=PluginBridge vs2015 || goto err
%PREMAKE% --group=OpenMPT-VSTi vs2015 || goto err
%PREMAKE% --group=OpenMPT vs2015 || goto err
%PREMAKE% --group=all-externals vs2015 || goto err

%PREMAKE% --group=libopenmpt_test vs2017 || goto err
%PREMAKE% --group=foo_openmpt vs2017 || goto err
%PREMAKE% --group=in_openmpt vs2017 || goto err
%PREMAKE% --group=xmp-openmpt vs2017 || goto err
%PREMAKE% --group=libopenmpt-full vs2017 || goto err
%PREMAKE% --group=libopenmpt-small vs2017 || goto err
%PREMAKE% --group=libopenmpt vs2017 || goto err
%PREMAKE% --group=openmpt123 vs2017 || goto err
%PREMAKE% --group=PluginBridge vs2017 || goto err
%PREMAKE% --group=OpenMPT-VSTi vs2017 || goto err
%PREMAKE% --group=OpenMPT vs2017 || goto err
%PREMAKE% --group=all-externals vs2017 || goto err

%PREMAKE% --group=libopenmpt_test vs2015 --xp || goto err
%PREMAKE% --group=foo_openmpt vs2015 --xp || goto err
%PREMAKE% --group=in_openmpt vs2015 --xp || goto err
%PREMAKE% --group=xmp-openmpt vs2015 --xp || goto err
%PREMAKE% --group=libopenmpt-full vs2015 --xp || goto err
%PREMAKE% --group=libopenmpt-small vs2015 --xp || goto err
%PREMAKE% --group=libopenmpt vs2015 --xp || goto err
%PREMAKE% --group=openmpt123 vs2015 --xp || goto err
%PREMAKE% --group=PluginBridge vs2015 --xp || goto err
%PREMAKE% --group=OpenMPT-VSTi vs2015 --xp || goto err
%PREMAKE% --group=OpenMPT vs2015 --xp || goto err
%PREMAKE% --group=all-externals vs2015 --xp || goto err

%PREMAKE% --group=libopenmpt_test vs2017 --xp || goto err
%PREMAKE% --group=foo_openmpt vs2017 --xp || goto err
%PREMAKE% --group=in_openmpt vs2017 --xp || goto err
%PREMAKE% --group=xmp-openmpt vs2017 --xp || goto err
%PREMAKE% --group=libopenmpt-full vs2017 --xp || goto err
%PREMAKE% --group=libopenmpt-small vs2017 --xp || goto err
%PREMAKE% --group=libopenmpt vs2017 --xp || goto err
%PREMAKE% --group=openmpt123 vs2017 --xp || goto err
%PREMAKE% --group=PluginBridge vs2017 --xp || goto err
%PREMAKE% --group=OpenMPT-VSTi vs2017 --xp || goto err
%PREMAKE% --group=OpenMPT vs2017 --xp || goto err
%PREMAKE% --group=all-externals vs2017 --xp || goto err

%PREMAKE% postprocess || goto err

del premake5.lua || goto err



cd %MY_DIR% || goto err

goto end

:err
echo ERROR!
goto end

:end
cd %MY_DIR%
pause
