set MY_DIR=%CD%
set BATCH_DIR=%~dp0
cd %BATCH_DIR% || goto error

powershell -Command "(New-Object Net.WebClient).DownloadFile('https://download.microsoft.com/download/0/A/9/0A939EF6-E31C-430F-A3DF-DFAE7960D564/htmlhelp.exe', 'htmlhelp.exe')"
"C:\Program Files\7-Zip\7z.exe" x -y htmlhelp.exe -ohtmlhelp
rm htmlhelp.exe

cd MY_DIR