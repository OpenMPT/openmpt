@echo off

set SVNVERSION=unknown

set SVNVERSION_VALID=false
del /f svnversion.txt
if "x%SVNVERSION%" == "xunknown" (
	set SVNVERSION_VALID=true
	svnversion > svnversion.txt
	if errorlevel 1 (
		set SVNVERSION_VALID=false
	)
)
if "x%SVNVERSION_VALID%" == "xtrue" (
	set /p %RAWSVNVERSION=<svnversion.txt
)
if "x%SVNVERSION_VALID%" == "xtrue" (
	if "%RAWSVNVERSION%" == "Unversioned directory" (
		set SVNVERSION_VALID=false
	)
)
if "x%SVNVERSION_VALID%" == "xtrue" (
	set SVNVERSION=r%RAWSVNVERSION::=-%
)
del /f svnversion.txt

set GITVERSION_VALID=false
del /f gitversion.txt
if "x%SVNVERSION%" == "xunknown" (
	set GITVERSION_VALID=true
	git log --format=format:%%ct -n 1 > gitversion.txt
	if errorlevel 1 (
		set GITVERSION_VALID=false
	)
)
if "x%GITVERSION_VALID%" == "xtrue" (
	set /p %RAWGITVERSION=<gitversion.txt
)
if "x%GITVERSION_VALID%" == "xtrue" (
	set SVNVERSION=t%RAWGITVERSION%
)
del /f gitversion.txt

set RAWSVNVERSION=
set SVNVERSION_VALID=
set GITVERSION_VALID=

echo %SVNVERSION%
