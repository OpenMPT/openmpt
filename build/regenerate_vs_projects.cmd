@echo off

set MY_DIR=%CD%

cd build
cd .. || goto err


echo dofile "build/premake4-win/premake.lua" > premake5.lua || goto err

include\premake\premake5.exe --group=libopenmpt_test vs2008 || goto err
include\premake\premake5.exe --group=libopenmpt vs2008 || goto err
include\premake\premake5.exe --group=openmpt123 vs2008 || goto err
include\premake\premake5.exe --group=PluginBridge vs2008 || goto err
include\premake\premake5.exe --group=OpenMPT vs2008 || goto err
include\premake\premake5.exe --group=all-externals vs2008 || goto err

include\premake\premake5.exe --group=libopenmpt_test vs2010 || goto err
include\premake\premake5.exe --group=foo_openmpt vs2010 || goto err
include\premake\premake5.exe --group=in_openmpt vs2010 || goto err
include\premake\premake5.exe --group=xmp-openmpt vs2010 || goto err
include\premake\premake5.exe --group=libopenmpt vs2010 || goto err
include\premake\premake5.exe --group=openmpt123 vs2010 || goto err
include\premake\premake5.exe --group=PluginBridge vs2010 || goto err
include\premake\premake5.exe --group=OpenMPT vs2010 || goto err
include\premake\premake5.exe --group=all-externals vs2010 || goto err

include\premake\premake5.exe --group=libopenmpt_test vs2012 || goto err
include\premake\premake5.exe --group=in_openmpt vs2012 || goto err
include\premake\premake5.exe --group=xmp-openmpt vs2012 || goto err
include\premake\premake5.exe --group=libopenmpt vs2012 || goto err
include\premake\premake5.exe --group=openmpt123 vs2012 || goto err
include\premake\premake5.exe --group=PluginBridge vs2012 || goto err
include\premake\premake5.exe --group=OpenMPT vs2012 || goto err
include\premake\premake5.exe --group=all-externals vs2012 || goto err

include\premake\premake5.exe --group=libopenmpt_test vs2013 || goto err
include\premake\premake5.exe --group=in_openmpt vs2013 || goto err
include\premake\premake5.exe --group=xmp-openmpt vs2013 || goto err
include\premake\premake5.exe --group=libopenmpt vs2013 || goto err
include\premake\premake5.exe --group=openmpt123 vs2013 || goto err
include\premake\premake5.exe --group=PluginBridge vs2013 || goto err
include\premake\premake5.exe --group=OpenMPT vs2013 || goto err
include\premake\premake5.exe --group=all-externals vs2013 || goto err

include\premake\premake5.exe --group=libopenmpt_test vs2015 || goto err
include\premake\premake5.exe --group=in_openmpt vs2015 || goto err
include\premake\premake5.exe --group=xmp-openmpt vs2015 || goto err
include\premake\premake5.exe --group=libopenmpt vs2015 || goto err
include\premake\premake5.exe --group=openmpt123 vs2015 || goto err
include\premake\premake5.exe --group=PluginBridge vs2015 || goto err
include\premake\premake5.exe --group=OpenMPT vs2015 || goto err
include\premake\premake5.exe --group=all-externals vs2015 || goto err

include\premake\premake5.exe postprocess || goto err

del premake5.lua || goto err


cd %MY_DIR% || goto err

goto end

:err
echo ERROR!
goto end

:end
cd %MY_DIR%
pause
