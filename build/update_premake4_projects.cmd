@echo off

set MY_DIR=%CD%

cd build
cd .. || goto err


echo dofile "build/premake4.lua" > premake4.lua || goto err

build\premake4.exe --group=libopenmpt_test vs2008 || goto err
build\premake4.exe --group=libopenmpt vs2008 || goto err
build\premake4.exe --group=openmpt123 vs2008 || goto err
build\premake4.exe --group=all-externals vs2008 || goto err

build\premake4.exe --group=libopenmpt_test vs2010 || goto err
build\premake4.exe --group=in_openmpt vs2010 || goto err
build\premake4.exe --group=xmp-openmpt vs2010 || goto err
build\premake4.exe --group=libopenmpt vs2010 || goto err
build\premake4.exe --group=openmpt123 vs2010 || goto err
build\premake4.exe --group=all-externals vs2010 || goto err

build\premake4.exe postprocess || goto err

del premake4.lua || goto err


cd %MY_DIR% || goto err

goto end

:err
echo ERROR!
goto end

:end
cd %MY_DIR%
pause
