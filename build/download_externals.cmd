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

if "%MPT_DOWNLOAD%" == "yes" (

 call build\scriptlib\download.cmd "https://www.7-zip.org/a/7za920.zip"                                                              "build\externals\7za920.zip"      || goto error
 call build\scriptlib\download.cmd "https://www.7-zip.org/a/7z1805-extra.7z"                                                         "build\externals\7z1805-extra.7z" || goto error
 call build\scriptlib\download.cmd "https://www.7-zip.org/a/7z1805.exe"                                                              "build\externals\7z1805.exe"      || goto error

 call build\scriptlib\download.cmd "https://github.com/bkaradzic/GENie/archive/78817a9707c1a02e845fb38b3adcc5353b02d377.zip"         "build\externals\GENie-78817a9707c1a02e845fb38b3adcc5353b02d377.zip"        || goto error
 call build\scriptlib\download.cmd "https://github.com/premake/premake-core/archive/2e7ca5fb18acdbcd5755fb741710622b20f2e0f6.zip"    "build\externals\premake-core-2e7ca5fb18acdbcd5755fb741710622b20f2e0f6.zip" || goto error

 call build\scriptlib\download.cmd "http://download.nullsoft.com/winamp/plugin-dev/WA5.55_SDK.exe"                                   "build\externals\WA5.55_SDK.exe"  || goto error
 call build\scriptlib\download.cmd "https://www.un4seen.com/files/xmp-sdk.zip"                                                       "build\externals\xmp-sdk.zip"     || goto error
 call build\scriptlib\download.cmd "https://www.steinberg.net/sdk_downloads/asiosdk2.3.zip"                                          "build\externals\asiosdk2.3.zip"  || goto error

 call build\scriptlib\download.cmd "https://download.microsoft.com/download/0/A/9/0A939EF6-E31C-430F-A3DF-DFAE7960D564/htmlhelp.exe" "build\externals\htmlhelp.exe"    || goto error

)

powershell -ExecutionPolicy Unrestricted .\build\scriptlib\Verify-File.ps1 -hashvalue e81805f2039c18d4311fb1e04297656f6efbbe600a7eda91c835375dad03dbe805dc6f704355cfd085600a0f755392e96df7a1d80d4b6f416e177314cc20d666 -filesize 1181017 -filename build/externals/7z1805.exe || goto error
powershell -ExecutionPolicy Unrestricted .\build\scriptlib\Verify-File.ps1 -hashvalue 09bca8018272c3a4e7dd68f62a832acfe0581d3713f29473463725fa5c1708bc34b30a126b069e741e0e4939b645f6818e1f100cf1b2b021e851d132a6abcca5 -filesize  923870 -filename build/externals/7z1805-extra.7z || goto error
powershell -ExecutionPolicy Unrestricted .\build\scriptlib\Verify-File.ps1 -hashvalue 84e830c91a0e8ae499cc4814080da6569d8a6acbddc585c8b62abc86c809793aeb669b0a741063a379fd281ade85f120bc27efeb67d63bf961be893eec8bc3b3 -filesize  384846 -filename build/externals/7za920.zip || goto error
powershell -ExecutionPolicy Unrestricted .\build\scriptlib\Verify-File.ps1 -hashvalue aba21883cd026a789395757f7dcc127d7d6372965693ddc3794c8adfc3a9675c255cedf2a87177729fa0b094e1bdb4de9d2e47555c61ddd6976c24d71cbd5e38 -filesize  422934 -filename build/externals/asiosdk2.3.zip || goto error
powershell -ExecutionPolicy Unrestricted .\build\scriptlib\Verify-File.ps1 -hashvalue d93650059cf10faff343e28a5eda60a9e78c123574f1ff479ecea85a179912f3241a36734e9f54ca3041ebcf80b732a9d21a3433b0da6c48b95bed16677af0cb -filesize  685026 -filename build/externals/GENie-78817a9707c1a02e845fb38b3adcc5353b02d377.zip || goto error
powershell -ExecutionPolicy Unrestricted .\build\scriptlib\Verify-File.ps1 -hashvalue d91371244ea98c691b4674ee266c4a2496a296800c176adae069d21f5c52c0763b21cc7859cfffa865b89e50171a2c99a6d14620c32f7d72c0ef04045348f856 -filesize 3509072 -filename build/externals/htmlhelp.exe || goto error
powershell -ExecutionPolicy Unrestricted .\build\scriptlib\Verify-File.ps1 -hashvalue ec3e56e3fe452accafbb89096112119dc737913f4b533d2f19347ebe1d42099d80f0b6cd1a5b0b3bdb5e7c5cd68348613211f3e3d93493893c6143d15935b14f -filesize 5106004 -filename build/externals/premake-core-2e7ca5fb18acdbcd5755fb741710622b20f2e0f6.zip || goto error
powershell -ExecutionPolicy Unrestricted .\build\scriptlib\Verify-File.ps1 -hashvalue 394375db8a16bf155b5de9376f6290488ab339e503dbdfdc4e2f5bede967799e625c559cca363bc988324f1a8e86e5fd28a9f697422abd7bb3dcde4a766607b5 -filesize  336166 -filename build/externals/WA5.55_SDK.exe || goto error
powershell -ExecutionPolicy Unrestricted .\build\scriptlib\Verify-File.ps1 -hashvalue ddaea1ba4b3ead2b3c870652c6a2ce0a9bdab2f0d2f01030214959dd4ab9ca0f9d34c88c817a1a2847b3c39597215e0e3b749a0489a0e6ce806a4d7e0d44ef9b -filesize   28971 -filename build/externals/xmp-sdk.zip || goto error

call :killdir "build\tools\7zipold" || goto error
call :killdir "build\tools\7zipa" || goto error
call :killdir "build\tools\7zip" || goto error
rem Get old 7zip distributed as zip and unpack with built-in zip depacker
rem Get current 7zip commandline version which can unpack 7zip and the 7zip installer but not other archive formats
rem Get 7zip installer and unpack it with current commandline 7zip
rem This is a mess for automatic. Oh well.
cscript build\scriptlib\unpack-zip.vbs "build\externals\7za920.zip" "build\tools\7zipold" || goto error
build\tools\7zipold\7za.exe x -y -obuild\tools\7zipa "build\externals\7z1805-extra.7z" || goto error
build\tools\7zipa\7za.exe x -y -obuild\tools\7zip "build\externals\7z1805.exe" || goto error

call build\scriptlib\unpack.cmd "build\tools\htmlhelp" "build\externals\htmlhelp.exe" "." || goto error

call build\scriptlib\unpack.cmd "include\winamp"   "build\externals\WA5.55_SDK.exe" "."          || goto error
call build\scriptlib\unpack.cmd "include\xmplay"   "build\externals\xmp-sdk.zip"    "."          || goto error
call build\scriptlib\unpack.cmd "include\ASIOSDK2" "build\externals\asiosdk2.3.zip" "ASIOSDK2.3" || goto error

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
