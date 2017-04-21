@echo off


if exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" (
 call %~dp0\setup_vs2017.cmd
 goto vsdone
)
if exist "C:\Program Files\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" (
 call %~dp0\setup_vs2017.cmd
 goto vsdone
)

if exist "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" (
 call %~dp0\setup_vs2015.cmd
 goto vsdone
)
if exist "C:\Program Files\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" (
 call %~dp0\setup_vs2015.cmd
 goto vsdone
)

:vsdone
