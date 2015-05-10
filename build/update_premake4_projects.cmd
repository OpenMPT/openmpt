@echo off

set MY_DIR=%CD%

cd build
cd .. || goto err


goto premake4


:premake4

echo dofile "build/premake4-win/premake.lua" > premake4.lua || goto err

include\premake\premake4.exe --group=libopenmpt_test vs2008 || goto err
include\premake\premake4.exe --group=libopenmpt vs2008 || goto err
include\premake\premake4.exe --group=openmpt123 vs2008 || goto err
include\premake\premake4.exe --group=PluginBridge vs2008 || goto err
include\premake\premake4.exe --group=OpenMPT vs2008 || goto err
include\premake\premake4.exe --group=all-externals vs2008 || goto err

include\premake\premake4.exe --group=libopenmpt_test vs2010 || goto err
include\premake\premake4.exe --group=in_openmpt vs2010 || goto err
include\premake\premake4.exe --group=xmp-openmpt vs2010 || goto err
include\premake\premake4.exe --group=libopenmpt vs2010 || goto err
include\premake\premake4.exe --group=openmpt123 vs2010 || goto err
include\premake\premake4.exe --group=PluginBridge vs2010 || goto err
include\premake\premake4.exe --group=OpenMPT vs2010 || goto err
include\premake\premake4.exe --group=all-externals vs2010 || goto err

include\premake\premake4.exe postprocess || goto err

del premake4.lua || goto err

goto premakedone


:premake5

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

include\premake\premake5.exe postprocess || goto err

del premake5.lua || goto err

goto premakedone


:premakedone

cd %MY_DIR% || goto err

goto end

:err
echo ERROR!
goto end

:end
cd %MY_DIR%
pause
