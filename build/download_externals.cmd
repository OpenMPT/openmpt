@echo off

if not "x%1" == "xauto" (
	echo "WARNING: This script will unconditionally remove all files from the destination directories."
	pause
)

if "x%2" == "xnodownload" (
 set MPT_DOWNLOAD=no
)
if not "x%2" == "xnodownload" (
 set MPT_DOWNLOAD=yes
)

set MY_DIR=%CD%
set BATCH_DIR=%~dp0
cd %BATCH_DIR% || goto error
cd .. || goto error
goto main

:killdir
 set MPT_KILLDIR_DIR="%~1"
 if exist %MPT_KILLDIR_DIR% rmdir /S /Q %MPT_KILLDIR_DIR%
exit /B 0
goto error

:main
if not exist "build\externals" mkdir "build\externals"
if not exist "build\tools"     mkdir "build\tools"


call build\scriptlib\download.cmd %MPT_DOWNLOAD% x%1 "https://www.7-zip.org/a/7za920.zip"                                                              "build\externals\7za920.zip"                   84e830c91a0e8ae499cc4814080da6569d8a6acbddc585c8b62abc86c809793aeb669b0a741063a379fd281ade85f120bc27efeb67d63bf961be893eec8bc3b3  384846 || goto error

call build\scriptlib\download.cmd %MPT_DOWNLOAD% x%1 "https://www.7-zip.org/a/7z1900-extra.7z"                                                         "build\externals\7z1900-extra.7z"              17713037504215ae277037ea7e61d78afe2863a1ff3b8b9ff0a0f6af80969c054df35f4f2bf5ba79a35601a29b8600a8b71a16ced23d17e55eeb1cc131d55f65  929117 || goto error

call build\scriptlib\download.cmd %MPT_DOWNLOAD% x%1 "https://www.7-zip.org/a/7z1900.exe"                                                              "build\externals\7z1900.exe"                   2924fd60f5dd636f643b68d402b65c2bfab5536122aa688ebba5ae142c7d04ce8b1c8e078f54db8adadce9d5c6fa74c0794604ecc16a4c5489f9ca70a6d9e1c4 1185968 || goto error


call build\scriptlib\download.cmd %MPT_DOWNLOAD% x%1 "https://web.archive.org/web/20131217072017id_/http://download.nullsoft.com/winamp/plugin-dev/WA5.55_SDK.exe" "build\externals\WA5.55_SDK.exe"               394375db8a16bf155b5de9376f6290488ab339e503dbdfdc4e2f5bede967799e625c559cca363bc988324f1a8e86e5fd28a9f697422abd7bb3dcde4a766607b5  336166 || goto error

call build\scriptlib\download.cmd %MPT_DOWNLOAD% x%1 "https://www.un4seen.com/files/xmp-sdk.zip"                                                       "build\externals\xmp-sdk.zip"                  67b96c6e6aa794e9de4f446d23f969e3591457196fd100c5475f5df52308de861a0c411db54fcb2bf46a12e9136ddda9d2974a5167432a979a701ef2c4679ef9  322903 || goto error


call build\scriptlib\download.cmd %MPT_DOWNLOAD% x%1 "https://web.archive.org/web/20200918004813id_/http://download.microsoft.com/download/0/A/9/0A939EF6-E31C-430F-A3DF-DFAE7960D564/htmlhelp.exe" "build\externals\htmlhelp.exe"                 d91371244ea98c691b4674ee266c4a2496a296800c176adae069d21f5c52c0763b21cc7859cfffa865b89e50171a2c99a6d14620c32f7d72c0ef04045348f856 3509072 || goto error


call build\scriptlib\download.cmd %MPT_DOWNLOAD% x%1 "https://www.python.org/ftp/python/3.10.1/python-3.10.1-embed-amd64.zip"                          "build\externals\python-3.10.1-embed-amd64.zip" cb67b15c312cf6e157b5023106b52129998f0cc214fc79e53ffa8b7ef9f34200690d9ab6798401c47be4e6c22bfbfcb3c0fd17e71d9253c53d4572b1d4931029 8484449 || goto error


call build\scriptlib\download.cmd %MPT_DOWNLOAD% x%1 "https://netcologne.dl.sourceforge.net/project/innounp/innounp/innounp%%%%200.50/innounp050.rar"  "build\externals\innounp050.rar"               dbbc809308267a866db9d6b751fdeda6d179e1a65d8ddb14bb51984431ae91493f9a76105e1789b245732043a2c696c869ed10964b48cf59f81e55bd52f85330  141621 || goto error

call build\scriptlib\download.cmd %MPT_DOWNLOAD% x%1 "http://files.jrsoftware.org/is/6/innosetup-6.1.2.exe"                                            "build\externals\innosetup-6.1.2.exe"  ea6e276479011c0c90f58a68e95e79cf3273e0e5d6794749942493e5eea5b99b4a3f2ec881ef14cf7231dbc902408d3a29d8225cf3228c9b0de266cd7e02582a 4516136 || goto error


call :killdir "build\tools\7zipold" || goto error
call :killdir "build\tools\7zipa" || goto error
call :killdir "build\tools\7zip" || goto error
rem Get old 7zip distributed as zip and unpack with built-in zip depacker
rem Get current 7zip commandline version which can unpack 7zip and the 7zip installer but not other archive formats
rem Get 7zip installer and unpack it with current commandline 7zip
rem This is a mess for automatic. Oh well.
cscript build\scriptlib\unpack-zip.vbs "build\externals\7za920.zip" "build\tools\7zipold" || goto error
build\tools\7zipold\7za.exe x -y -obuild\tools\7zipa "build\externals\7z1900-extra.7z" || goto error
build\tools\7zipa\7za.exe x -y -obuild\tools\7zip "build\externals\7z1900.exe" || goto error

call build\scriptlib\unpack.cmd "build\tools\htmlhelp" "build\externals\htmlhelp.exe" "." || goto error

call build\scriptlib\unpack.cmd "include\winamp"   "build\externals\WA5.55_SDK.exe" "."          || goto error
call build\scriptlib\unpack.cmd "include\xmplay"   "build\externals\xmp-sdk.zip"    "."          || goto error

call build\scriptlib\unpack.cmd "build\tools\python3" "build\externals\python-3.10.1-embed-amd64.zip" "." || goto error

call :killdir "build\tools\innounp"   || goto error
call :killdir "build\tools\innosetup" || goto error
call build\scriptlib\unpack.cmd "build\tools\innounp" "build\externals\innounp050.rar" "." || goto error
build\tools\innounp\innounp.exe -x -dbuild\tools\innosetup "build\externals\innosetup-6.1.2.exe" || goto error

goto ok

:ok
echo "All OK."
if "x%1" == "xauto" (
	exit 0
)
goto end
:error
echo "Error!"
if "x%1" == "xauto" (
	exit 1
)
goto end
:end
cd %MY_DIR%
pause
