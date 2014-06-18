@echo off

if exist "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" goto win64
if exist "C:\Program Files\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" goto win32

goto setupdone

:win32
call "C:\Program Files\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" x86
goto setupdone

:win64
call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" x86
goto setupdone

:setupdone
