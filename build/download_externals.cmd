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

call build\scriptlib\download.cmd %MPT_DOWNLOAD% x%1 "https://www.7-zip.org/a/7z1805-extra.7z"                                                         "build\externals\7z1805-extra.7z"              09bca8018272c3a4e7dd68f62a832acfe0581d3713f29473463725fa5c1708bc34b30a126b069e741e0e4939b645f6818e1f100cf1b2b021e851d132a6abcca5  923870 || goto error

call build\scriptlib\download.cmd %MPT_DOWNLOAD% x%1 "https://www.7-zip.org/a/7z1805.exe"                                                              "build\externals\7z1805.exe"                   e81805f2039c18d4311fb1e04297656f6efbbe600a7eda91c835375dad03dbe805dc6f704355cfd085600a0f755392e96df7a1d80d4b6f416e177314cc20d666 1181017 || goto error


call build\scriptlib\download.cmd %MPT_DOWNLOAD% x%1 "http://download.nullsoft.com/winamp/plugin-dev/WA5.55_SDK.exe"                                   "build\externals\WA5.55_SDK.exe"               394375db8a16bf155b5de9376f6290488ab339e503dbdfdc4e2f5bede967799e625c559cca363bc988324f1a8e86e5fd28a9f697422abd7bb3dcde4a766607b5  336166 || goto error

call build\scriptlib\download.cmd %MPT_DOWNLOAD% x%1 "https://www.un4seen.com/files/xmp-sdk.zip"                                                       "build\externals\xmp-sdk.zip"                  62c442d656d4bb380360368a0f5f01da11b4ed54333d7f54f875a9a5ec390b08921e00bd08e62cd7a0a5fe642e3377023f20a950cc2a42898ff4cda9ab88fc91  322744 || goto error

call build\scriptlib\download.cmd %MPT_DOWNLOAD% x%1 "https://www.steinberg.net/sdk_downloads/asiosdk2.3.zip"                                          "build\externals\asiosdk2.3.zip"               aba21883cd026a789395757f7dcc127d7d6372965693ddc3794c8adfc3a9675c255cedf2a87177729fa0b094e1bdb4de9d2e47555c61ddd6976c24d71cbd5e38  422934 || goto error


call build\scriptlib\download.cmd %MPT_DOWNLOAD% x%1 "https://download.microsoft.com/download/0/A/9/0A939EF6-E31C-430F-A3DF-DFAE7960D564/htmlhelp.exe" "build\externals\htmlhelp.exe"                 d91371244ea98c691b4674ee266c4a2496a296800c176adae069d21f5c52c0763b21cc7859cfffa865b89e50171a2c99a6d14620c32f7d72c0ef04045348f856 3509072 || goto error


call build\scriptlib\download.cmd %MPT_DOWNLOAD% x%1 "https://www.python.org/ftp/python/3.7.3/python-3.7.3-embed-win32.zip"                            "build\externals\python-3.7.3-embed-win32.zip" 2c1b1f0a29d40a91771ae21a5f733eedc10984cd182cb10c2793bbd24191a89f20612a3f23c34047f37fb06369016bfd4a52915ed1b4a56f8bd2b4ca6994eb31 6526486 || goto error


call build\scriptlib\download.cmd %MPT_DOWNLOAD% x%1 "https://netcologne.dl.sourceforge.net/project/innounp/innounp/innounp%%%%200.49/innounp049.rar"  "build\externals\innounp049.rar"               a00dbb671de1fb0bc3c94ce97a569d2bbfcc00f6fcbe16578ea9940ffdc0558dcde7c9fa0b4a5d7c17d4b73706f6eb21f479c7e6d6b8de3605f3974d25841da3  140885 || goto error

call build\scriptlib\download.cmd %MPT_DOWNLOAD% x%1 "http://files.jrsoftware.org/is/6/innosetup-6.0.4.exe"                                            "build\externals\innosetup-6.0.4.exe"  396cd154e8210c4524748301105037e4cc6e9def6800c266c7de5ab1252df403d435125bbdee048dd35fd85c0df938afeb6df8dc8ff369b86c9648c521079d27 4217832 || goto error


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

call build\scriptlib\unpack.cmd "build\tools\python3" "build\externals\python-3.7.3-embed-win32.zip" "." || goto error

call :killdir "build\tools\innounp"   || goto error
call :killdir "build\tools\innosetup" || goto error
call build\scriptlib\unpack.cmd "build\tools\innounp" "build\externals\innounp049.rar" "." || goto error
build\tools\innounp\innounp.exe -x -dbuild\tools\innosetup "build\externals\innosetup-6.0.3.exe" || goto error

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
