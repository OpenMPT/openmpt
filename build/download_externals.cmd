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

call build\scriptlib\download.cmd %MPT_DOWNLOAD% x%1 "https://www.7-zip.org/a/7z2201-extra.7z"                                                         "build\externals\7z2201-extra.7z"              845b3fd5dda4187e47fa0650a5d8465484e6c407a2a1745bb12bc50aa266cc4b573393184642c1875388c262f16039c1b93f102908799147dbfc824a52d8d89d 1018450 || goto error

call build\scriptlib\download.cmd %MPT_DOWNLOAD% x%1 "https://www.7-zip.org/a/7z2201-x64.exe"                                                              "build\externals\7z2201-x64.exe"                   965d43f06d104bf6707513c459f18aaf8b049f4a043643d720b184ed9f1bb6c929309c51c3991d5aaff7b9d87031a7248ee3274896521abe955d0e49f901ac94 1575742 || goto error


call build\scriptlib\download.cmd %MPT_DOWNLOAD% x%1 "https://web.archive.org/web/20131217072017if_/http://download.nullsoft.com/winamp/plugin-dev/WA5.55_SDK.exe" "build\externals\WA5.55_SDK.exe"               394375db8a16bf155b5de9376f6290488ab339e503dbdfdc4e2f5bede967799e625c559cca363bc988324f1a8e86e5fd28a9f697422abd7bb3dcde4a766607b5  336166 || goto error

call build\scriptlib\download.cmd %MPT_DOWNLOAD% x%1 "https://www.un4seen.com/files/xmp-sdk.zip"                                                       "build\externals\xmp-sdk.zip"                  62c442d656d4bb380360368a0f5f01da11b4ed54333d7f54f875a9a5ec390b08921e00bd08e62cd7a0a5fe642e3377023f20a950cc2a42898ff4cda9ab88fc91  322744 || goto error


call build\scriptlib\download.cmd %MPT_DOWNLOAD% x%1 "https://web.archive.org/web/20200918004813if_/http://download.microsoft.com/download/0/A/9/0A939EF6-E31C-430F-A3DF-DFAE7960D564/htmlhelp.exe" "build\externals\htmlhelp.exe"                 d91371244ea98c691b4674ee266c4a2496a296800c176adae069d21f5c52c0763b21cc7859cfffa865b89e50171a2c99a6d14620c32f7d72c0ef04045348f856 3509072 || goto error


call build\scriptlib\download.cmd %MPT_DOWNLOAD% x%1 "https://www.python.org/ftp/python/3.11.1/python-3.11.1-embed-amd64.zip"                          "build\externals\python-3.11.1-embed-amd64.zip" 762d16b7c5cb8e55ed9ab015b8e12015d494dd1ffcae44534f25f61dcfac9d7e6e7a1d5e21d9284271e6114d869165d0dcd2ab4219f40a834172ee816ecd4604 10538985 || goto error


call build\scriptlib\download.cmd %MPT_DOWNLOAD% x%1 "https://netcologne.dl.sourceforge.net/project/innounp/innounp/innounp%%%%200.50/innounp050.rar"  "build\externals\innounp050.rar"               dbbc809308267a866db9d6b751fdeda6d179e1a65d8ddb14bb51984431ae91493f9a76105e1789b245732043a2c696c869ed10964b48cf59f81e55bd52f85330  141621 || goto error

call build\scriptlib\download.cmd %MPT_DOWNLOAD% x%1 "https://files.jrsoftware.org/is/6/innosetup-6.2.1.exe"                                            "build\externals\innosetup-6.2.1.exe"  be4a517ea178b988931548bf2cc7cdda2fc5a66da5ff82e4ed60bacd1854a79ea26bed41138a65d6392fca1e22517f78d5053bd09bb98a6b4c6bad1e3b6c28f9 4710104 || goto error

call build\scriptlib\download.cmd %MPT_DOWNLOAD% x%1 "https://files.jrsoftware.org/is/5/isetup-5.5.8-unicode.exe" "build\externals\isetup-5.5.8-unicode.exe"  da7e27d85caec85b4194c7b1412c5a64c0ae12f22d903b94f2f4ee9ea0cb99c91b2d1dbb49262eefae8129e6b91f5c46f26f353011076e77e75f9c955fc5e1cb 2342456 || goto error


call build\scriptlib\download.cmd %MPT_DOWNLOAD% x%1 "https://download.openmpt.org/resources/modules/example_songs_ompt_1_30.7z"                       "build\externals\example_songs_ompt_1_30.7z"  bfecf7f97fd71bd52bcfb38307ccb98c751e6a0fa0c1f31208b22b9392f03ea3da8f9271327df2de4fc2e463e0c13c6a24107fbe18caf8f446b7e7cf93073fa5 4881392 || goto error


call :killdir "build\tools\7zipold" || goto error
call :killdir "build\tools\7zipa" || goto error
call :killdir "build\tools\7zip" || goto error
rem Get old 7zip distributed as zip and unpack with built-in zip depacker
rem Get current 7zip commandline version which can unpack 7zip and the 7zip installer but not other archive formats
rem Get 7zip installer and unpack it with current commandline 7zip
rem This is a mess for automatic. Oh well.
cscript build\scriptlib\unpack-zip.vbs "build\externals\7za920.zip" "build\tools\7zipold" || goto error
build\tools\7zipold\7za.exe x -y -obuild\tools\7zipa "build\externals\7z2201-extra.7z" || goto error
build\tools\7zipa\7za.exe x -y -obuild\tools\7zip "build\externals\7z2201-x64.exe" || goto error

call build\scriptlib\unpack.cmd "build\tools\htmlhelp" "build\externals\htmlhelp.exe" "." || goto error

call build\scriptlib\unpack.cmd "include\winamp"   "build\externals\WA5.55_SDK.exe" "."          || goto error
call build\scriptlib\unpack.cmd "include\xmplay"   "build\externals\xmp-sdk.zip"    "."          || goto error

call build\scriptlib\unpack.cmd "build\tools\python3" "build\externals\python-3.11.1-embed-amd64.zip" "." || goto error

call :killdir "build\tools\innounp"   || goto error
call :killdir "build\tools\innosetup" || goto error
call :killdir "build\tools\innosetup5" || goto error
call build\scriptlib\unpack.cmd "build\tools\innounp" "build\externals\innounp050.rar" "." || goto error
build\tools\innounp\innounp.exe -x -dbuild\tools\innosetup "build\externals\innosetup-6.2.1.exe" || goto error
build\tools\innounp\innounp.exe -x -dbuild\tools\innosetup5 "build\externals\isetup-5.5.8-unicode.exe" || goto error

call build\scriptlib\unpack.cmd "packageTemplate\ExampleSongs" "build\externals\example_songs_ompt_1_30.7z" "." || goto error


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
