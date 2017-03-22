@echo off

svnversion > svnversion.txt
set /p %RAWSVNVERSION=<svnversion.txt
del /f svnversion.txt
if "%RAWSVNVERSION%" == "Unversioned directory" set RAWSVNVERSION=unknown
set SVNVERSION=%RAWSVNVERSION::=-%

echo %SVNVERSION%
