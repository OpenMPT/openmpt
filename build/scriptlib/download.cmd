echo %1
if not exist "%~2" (
 powershell -Command "(New-Object Net.WebClient).DownloadFile('%~1', '%~2')" || exit /B 1
)
exit /B 0
