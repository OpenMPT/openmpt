set MPT_GET_DESTDIR="%~1"
set MPT_GET_FILE="%~2"
set MPT_GET_SUBDIR="%~3"
echo %MPT_GET_DESTDIR%
if exist %MPT_GET_DESTDIR% rmdir /S /Q %MPT_GET_DESTDIR%
if "%~3" == "." (
 build\tools\7zip\7z.exe x -y -o%MPT_GET_DESTDIR% "%~2" || exit /B 1
)
if not "%~3" == "." (
 if exist "%~1.tmp" rmdir /S /Q "%~1.tmp"
 build\tools\7zip\7z.exe x -y -o"%~1.tmp" -ir!"%~3" "%~2" || exit /B 1
 choice /C y /N /T 1 /D y
 move /Y "%~1.tmp\%~3" %MPT_GET_DESTDIR% || exit /B 1
 rmdir /S /Q "%~1.tmp"
)
exit /B 0
