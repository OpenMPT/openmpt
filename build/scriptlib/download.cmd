rem bDOWNLOAD bAUTO URL FILE HASH SIZE 
echo URL %3
if "%1" == "yes" (
 if "%2" == "xauto" (
  echo Check %3 ...
  if exist "%~4" (
   powershell -ExecutionPolicy Unrestricted .\build\scriptlib\Verify-File.ps1 -hashvalue %5 -filesize %6 -filename %4 || del /Q "%4"
  )
 )
 if not exist "%~4" (
  echo Download %3 ...
  powershell -Command "(New-Object Net.WebClient).DownloadFile('%~3', '%~4')" || exit /B 1
 )
)
echo Verify %4 ...
powershell -ExecutionPolicy Unrestricted .\build\scriptlib\Verify-File.ps1 -hashvalue %5 -filesize %6 -filename %4 || exit /B 1
exit /B 0
