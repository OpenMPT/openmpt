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



:main



call build\scriptlib\unpack.cmd "include\genie" "build\externals\GENie-78817a9707c1a02e845fb38b3adcc5353b02d377.zip" "GENie-78817a9707c1a02e845fb38b3adcc5353b02d377" || goto error

xcopy /E /I /Y build\genie\genie\build\vs2015 include\genie\build\vs2015 || goto error
xcopy /E /I /Y build\genie\genie\build\vs2017 include\genie\build\vs2017 || goto error

if exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" (
 call build\auto\setup_vs2017.cmd || goto error
 cd include\genie\build\vs2017 || goto error
 devenv genie.sln /Upgrade || goto error
 msbuild genie.sln /target:Build /property:Configuration=Release;Platform=Win32 /maxcpucount /verbosity:minimal || goto error
 cd ..\..\..\.. || goto error
 goto geniedone
)
if exist "C:\Program Files\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" (
 call build\auto\setup_vs2017.cmd || goto error
 cd include\genie\build\vs2017 || goto error
 devenv genie.sln /Upgrade || goto error
 msbuild genie.sln /target:Build /property:Configuration=Release;Platform=Win32 /maxcpucount /verbosity:minimal || goto error
 cd ..\..\..\.. || goto error
 goto geniedone
)

if exist "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" (
 call build\auto\setup_vs2015.cmd || goto error
 cd include\genie\build\vs2015 || goto error
 msbuild genie.sln /target:Build /property:Configuration=Release;Platform=Win32 /maxcpucount /verbosity:minimal || goto error
 cd ..\..\..\.. || goto error
 goto geniedone
)
if exist "C:\Program Files\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" (
 call build\auto\setup_vs2015.cmd || goto error
 cd include\genie\build\vs2015 || goto error
 msbuild genie.sln /target:Build /property:Configuration=Release;Platform=Win32 /maxcpucount /verbosity:minimal || goto error
 cd ..\..\..\.. || goto error
 goto geniedone
)

:geniedone

echo "78817a9707c1a02e845fb38b3adcc5353b02d377" > include\genie\OpenMPT-version.txt



call build\scriptlib\unpack.cmd "include\premake" "build\externals\premake-core-2e7ca5fb18acdbcd5755fb741710622b20f2e0f6.zip" "premake-core-2e7ca5fb18acdbcd5755fb741710622b20f2e0f6" || goto error

if exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" (
 call build\auto\setup_vs2017.cmd || goto error
 cd include\premake || goto error
  nmake -f Bootstrap.mak windows MSDEV=vs2017 || goto error
  bin\release\premake5 embed --bytecode || goto error
  bin\release\premake5 --to=build/vs2017 vs2017 --no-curl --no-zlib --no-luasocket || goto error
 cd ..\.. || goto error
 cd include\premake\build\vs2017 || goto error
  msbuild Premake5.sln /target:Clean /property:Configuration=Release;Platform=Win32 /maxcpucount /verbosity:minimal || goto error
  msbuild Premake5.sln /target:Build /property:Configuration=Release;Platform=Win32 /maxcpucount /verbosity:minimal || goto error
 cd ..\..\..\.. || goto error
 goto premakedone
)
if exist "C:\Program Files\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" (
 call build\auto\setup_vs2017.cmd || goto error
 cd include\premake || goto error
  nmake -f Bootstrap.mak windows MSDEV=vs2017 || goto error
  bin\release\premake5 embed --bytecode || goto error
  bin\release\premake5 --to=build/vs2017 vs2017 --no-curl --no-zlib --no-luasocket || goto error
 cd ..\.. || goto error
 cd include\premake\build\vs2017 || goto error
  msbuild Premake5.sln /target:Clean /property:Configuration=Release;Platform=Win32 /maxcpucount /verbosity:minimal || goto error
  msbuild Premake5.sln /target:Build /property:Configuration=Release;Platform=Win32 /maxcpucount /verbosity:minimal || goto error
 cd ..\..\..\.. || goto error
 goto premakedone
)

if exist "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" (
 call build\auto\setup_vs2015.cmd || goto error
 cd include\premake || goto error
  nmake -f Bootstrap.mak windows MSDEV=vs2015 || goto error
  bin\release\premake5 embed --bytecode || goto error
  bin\release\premake5 --to=build/vs2015 vs2015 --no-curl --no-zlib --no-luasocket || goto error
 cd ..\.. || goto error
 cd include\premake\build\vs2015 || goto error
  msbuild Premake5.sln /target:Clean /property:Configuration=Release;Platform=Win32 /maxcpucount /verbosity:minimal || goto error
  msbuild Premake5.sln /target:Build /property:Configuration=Release;Platform=Win32 /maxcpucount /verbosity:minimal || goto error
 cd ..\..\..\.. || goto error
 goto premakedone
)
if exist "C:\Program Files\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" (
 call build\auto\setup_vs2015.cmd || goto error
 cd include\premake || goto error
  nmake -f Bootstrap.mak windows MSDEV=vs2015 || goto error
  bin\release\premake5 embed --bytecode || goto error
  bin\release\premake5 --to=build/vs2015 vs2015 --no-curl --no-zlib --no-luasocket || goto error
 cd ..\.. || goto error
 cd include\premake\build\vs2015 || goto error
  msbuild Premake5.sln /target:Clean /property:Configuration=Release;Platform=Win32 /maxcpucount /verbosity:minimal || goto error
  msbuild Premake5.sln /target:Build /property:Configuration=Release;Platform=Win32 /maxcpucount /verbosity:minimal || goto error
 cd ..\..\..\.. || goto error
 goto premakedone
)

goto error

:premakedone

echo "2e7ca5fb18acdbcd5755fb741710622b20f2e0f6" > include\premake\OpenMPT-version.txt

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
