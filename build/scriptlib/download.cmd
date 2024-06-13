rem FILE SIZE HASH [URLS...]
set MPT_DOWNLOAD_FILENAME=%~1
set MPT_DOWNLOAD_FILESIZE=%2
set MPT_DOWNLOAD_FILEHASH=%3
shift
shift
shift

echo "Checking '%MPT_DOWNLOAD_FILENAME%' ..."
if exist "%MPT_DOWNLOAD_FILENAME%" (
 powershell -ExecutionPolicy Unrestricted .\build\scriptlib\Verify-File.ps1 -filename %MPT_DOWNLOAD_FILENAME% -filesize %MPT_DOWNLOAD_FILESIZE% -hashvalue %MPT_DOWNLOAD_FILEHASH% || powershell -ExecutionPolicy Unrestricted .\build\scriptlib\Delete-File.ps1 -filename %MPT_DOWNLOAD_FILENAME%
)

if "x%MPT_DOWNLOAD_DO%" == "xyes" (
 :loop
 if not "%~1" == "" (
  if not exist "%MPT_DOWNLOAD_FILENAME%" (
   echo "Downloading '%MPT_DOWNLOAD_FILENAME%' from '%~1' ..."
   powershell -Command "(New-Object Net.WebClient).DownloadFile('%~1', '%MPT_DOWNLOAD_FILENAME%')"
   echo "Verifying '%~1' ..."
   if exist "%MPT_DOWNLOAD_FILENAME%" (
    powershell -ExecutionPolicy Unrestricted .\build\scriptlib\Verify-File.ps1 -filename %MPT_DOWNLOAD_FILENAME% -filesize %MPT_DOWNLOAD_FILESIZE% -hashvalue %MPT_DOWNLOAD_FILEHASH% || powershell -ExecutionPolicy Unrestricted .\build\scriptlib\Delete-File.ps1 -filename %MPT_DOWNLOAD_FILENAME%
   )
  )
  shift
  goto loop
 )
)

if not exist "%MPT_DOWNLOAD_FILENAME%" (
 echo "Failed to download '%MPT_DOWNLOAD_FILENAME%'."
 exit /B 1
)
exit /b 0
