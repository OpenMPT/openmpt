@echo off

set SVNVERSION=unknown

if "x%SVNVERSION%" == "xunknown" (
	svnversion > svnversion.txt
	if errorlevel 1 (
		del /f svnversion.txt
		set SVNVERSION=unknown
	) else (
		set /p %RAWSVNVERSION=<svnversion.txt
		del /f svnversion.txt
		if "%RAWSVNVERSION%" == "Unversioned directory" (
			set SVNVERSION=unknown
		) else (
			set SVNVERSION=r%RAWSVNVERSION::=-%
		)
	)
)

if "x%SVNVERSION%" == "xunknown" (
	git log --format=format:"%ct" -n 1 > gitversion
	if errorlevel 1 (
		set SVNVERSION=unknown
	) else (
		set /p %RAWGITVERSION=<gitversion.txt
		del /f gitversion.txt
		set SVNVERSION=t%RAWGITVERSION%
	)
)

echo %SVNVERSION%
