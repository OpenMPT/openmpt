@echo off

svnversion > svnversion.txt
set /p %RAWSVNVERSION=<svnversion.txt
del /f svnversion.txt
set SVNVERSION=%RAWSVNVERSION::=-%

rem echo %SVNVERSION%
