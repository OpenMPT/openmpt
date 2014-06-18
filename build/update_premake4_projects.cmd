@echo off

set MY_DIR=%CD%

cd build
cd .. || goto err

echo dofile "build/premake4.lua" > premake4.lua || goto err

build\premake4.exe vs2008 || goto err
build\premake4.exe vs2010 || goto err

del premake4.lua || goto err

cd %MY_DIR% || goto err

goto end

:err
echo ERROR!
goto end

:end
cd %MY_DIR%
pause
