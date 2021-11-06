@echo off

if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" goto win64
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" goto win32

goto setupdone

:win32
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
goto setupdone

:win64
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
goto setupdone

:setupdone
