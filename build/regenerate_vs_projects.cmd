@echo off

set MY_DIR=%CD%

set BATCH_DIR=%~dp0
cd %BATCH_DIR% || goto err
cd .. || goto err


set PREMAKE=
if exist "include\premake\premake5.exe" set PREMAKE=include\premake\premake5.exe
if exist "include\premake\bin\release\premake5.exe" set PREMAKE=include\premake\bin\release\premake5.exe



echo dofile "build/premake/premake.lua" > premake5.lua || goto err

%PREMAKE% --group=libopenmpt_test vs2010 || goto err
%PREMAKE% --group=foo_openmpt vs2010 || goto err
%PREMAKE% --group=in_openmpt vs2010 || goto err
%PREMAKE% --group=xmp-openmpt vs2010 || goto err
%PREMAKE% --group=libopenmpt-small vs2010 || goto err
%PREMAKE% --group=libopenmpt vs2010 || goto err
%PREMAKE% --group=openmpt123 vs2010 || goto err
rem %PREMAKE% --group=PluginBridge vs2010 || goto err
rem %PREMAKE% --group=OpenMPT-VSTi vs2010 || goto err
rem %PREMAKE% --group=OpenMPT vs2010 || goto err
%PREMAKE% --group=all-externals vs2010 || goto err

%PREMAKE% --group=libopenmpt_test vs2012 || goto err
%PREMAKE% --group=foo_openmpt vs2012 || goto err
%PREMAKE% --group=in_openmpt vs2012 || goto err
%PREMAKE% --group=xmp-openmpt vs2012 || goto err
%PREMAKE% --group=libopenmpt-small vs2012 || goto err
%PREMAKE% --group=libopenmpt vs2012 || goto err
%PREMAKE% --group=openmpt123 vs2012 || goto err
rem %PREMAKE% --group=PluginBridge vs2012 || goto err
rem %PREMAKE% --group=OpenMPT-VSTi vs2012 || goto err
rem %PREMAKE% --group=OpenMPT vs2012 || goto err
%PREMAKE% --group=all-externals vs2012 || goto err

%PREMAKE% --group=libopenmpt_test vs2013 || goto err
%PREMAKE% --group=foo_openmpt vs2013 || goto err
%PREMAKE% --group=in_openmpt vs2013 || goto err
%PREMAKE% --group=xmp-openmpt vs2013 || goto err
%PREMAKE% --group=libopenmpt-small vs2013 || goto err
%PREMAKE% --group=libopenmpt vs2013 || goto err
%PREMAKE% --group=openmpt123 vs2013 || goto err
rem %PREMAKE% --group=PluginBridge vs2013 || goto err
rem %PREMAKE% --group=OpenMPT-VSTi vs2013 || goto err
rem %PREMAKE% --group=OpenMPT vs2013 || goto err
%PREMAKE% --group=all-externals vs2013 || goto err

%PREMAKE% --group=libopenmpt_test vs2015 || goto err
%PREMAKE% --group=foo_openmpt vs2015 || goto err
%PREMAKE% --group=in_openmpt vs2015 || goto err
%PREMAKE% --group=xmp-openmpt vs2015 || goto err
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
%PREMAKE% --group=libopenmpt-small vs2017 || goto err
%PREMAKE% --group=libopenmpt vs2017 || goto err
%PREMAKE% --group=openmpt123 vs2017 || goto err
%PREMAKE% --group=PluginBridge vs2017 || goto err
%PREMAKE% --group=OpenMPT-VSTi vs2017 || goto err
%PREMAKE% --group=OpenMPT vs2017 || goto err
%PREMAKE% --group=all-externals vs2017 || goto err

%PREMAKE% --group=libopenmpt_test vs2010 --xp || goto err
%PREMAKE% --group=foo_openmpt vs2010 --xp || goto err
%PREMAKE% --group=in_openmpt vs2010 --xp || goto err
%PREMAKE% --group=xmp-openmpt vs2010 --xp || goto err
%PREMAKE% --group=libopenmpt-small vs2010 --xp || goto err
%PREMAKE% --group=libopenmpt vs2010 --xp || goto err
%PREMAKE% --group=openmpt123 vs2010 --xp || goto err
rem %PREMAKE% --group=PluginBridge vs2010 --xp || goto err
rem %PREMAKE% --group=OpenMPT-VSTi vs2010 --xp || goto err
rem %PREMAKE% --group=OpenMPT vs2010 --xp || goto err
%PREMAKE% --group=all-externals vs2010 --xp || goto err

%PREMAKE% --group=libopenmpt_test vs2012 --xp || goto err
%PREMAKE% --group=foo_openmpt vs2012 --xp || goto err
%PREMAKE% --group=in_openmpt vs2012 --xp || goto err
%PREMAKE% --group=xmp-openmpt vs2012 --xp || goto err
%PREMAKE% --group=libopenmpt-small vs2012 --xp || goto err
%PREMAKE% --group=libopenmpt vs2012 --xp || goto err
%PREMAKE% --group=openmpt123 vs2012 --xp || goto err
rem %PREMAKE% --group=PluginBridge vs2012 --xp || goto err
rem %PREMAKE% --group=OpenMPT-VSTi vs2012 --xp || goto err
rem %PREMAKE% --group=OpenMPT vs2012 --xp || goto err
%PREMAKE% --group=all-externals vs2012 --xp || goto err

%PREMAKE% --group=libopenmpt_test vs2013 --xp || goto err
%PREMAKE% --group=foo_openmpt vs2013 --xp || goto err
%PREMAKE% --group=in_openmpt vs2013 --xp || goto err
%PREMAKE% --group=xmp-openmpt vs2013 --xp || goto err
%PREMAKE% --group=libopenmpt-small vs2013 --xp || goto err
%PREMAKE% --group=libopenmpt vs2013 --xp || goto err
%PREMAKE% --group=openmpt123 vs2013 --xp || goto err
rem %PREMAKE% --group=PluginBridge vs2013 --xp || goto err
rem %PREMAKE% --group=OpenMPT-VSTi vs2013 --xp || goto err
rem %PREMAKE% --group=OpenMPT vs2013 --xp || goto err
%PREMAKE% --group=all-externals vs2013 --xp || goto err

%PREMAKE% --group=libopenmpt_test vs2015 --xp || goto err
%PREMAKE% --group=foo_openmpt vs2015 --xp || goto err
%PREMAKE% --group=in_openmpt vs2015 --xp || goto err
%PREMAKE% --group=xmp-openmpt vs2015 --xp || goto err
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
