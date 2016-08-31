@echo off


if exist "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" (
 call %~dp0\setup_vs2015.cmd
 goto vsdone
)
if exist "C:\Program Files\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" (
 call %~dp0\setup_vs2015.cmd
 goto vsdone
)

if exist "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" (
 call %~dp0\setup_vs2013.cmd
 goto vsdone
)
if exist "C:\Program Files\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" (
 call %~dp0\setup_vs2013.cmd
 goto vsdone
)

if exist "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\vcvarsall.bat" (
 call %~dp0\setup_vs2012.cmd
 goto vsdone
)
if exist "C:\Program Files\Microsoft Visual Studio 11.0\VC\vcvarsall.bat" (
 call %~dp0\setup_vs2012.cmd
 goto vsdone
)

if exist "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" (
 call %~dp0\setup_vs2010.cmd
 goto vsdone
)
if exist "C:\Program Files\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" (
 call %~dp0\setup_vs2010.cmd
 goto vsdone
)

:vsdone
