@echo off

set BATCH_DIR=%~dp0
cd %BATCH_DIR%
cd ..\..

set MY_DIR=%CD%

call build\auto\helper_get_svnversion.cmd

set MPT_REVISION=%SVNVERSION%



cd bin || goto error
rmdir /s /q libopenmpt-winold
del /f /q libopenmpt-winold.tar
del /f /q libopenmpt-winold-r%MPT_REVISION%.7z
mkdir libopenmpt-winold
cd libopenmpt-winold || goto error
mkdir openmpt123
mkdir XMPlay
mkdir Winamp
rmdir /s /q Licenses
mkdir Licenses
rem copy /y ..\..\include\bladeenc\BladeDLL.html                 .\Licenses\License.BladeDLL.html || goto error
copy /y ..\..\include\flac\COPYING.Xiph                      .\Licenses\License.FLAC.txt || goto error
rem copy /y ..\..\include\foobar2000sdk\sdk-license.txt          .\Licenses\License.Foobar2000SDK.txt || goto error
rem copy /y ..\..\include\foobar2000sdk\pfc\pfc-license.txt      .\Licenses\License.Foobar2000SDK-pfc.txt || goto error
rem copy /y ..\..\include\lhasa\COPYING                          .\Licenses\License.lhasa.txt || goto error
copy /y ..\..\include\miniz\miniz.c                          .\Licenses\License.miniz.txt || goto error
copy /y ..\..\include\msinttypes\stdint\stdint.h             .\Licenses\License.msinttypes.txt || goto error
rem copy /y ..\..\include\ogg\COPYING                            .\Licenses\License.ogg.txt || goto error
rem copy /y ..\..\include\opus\COPYING                           .\Licenses\License.Opus.txt || goto error
copy /y ..\..\include\portaudio\LICENSE.txt                  .\Licenses\License.PortAudio.txt || goto error
copy /y ..\..\include\portaudio\bindings\cpp\COPYING         .\Licenses\License.portaudiocpp.txt || goto error
rem copy /y ..\..\include\portmidi\license.txt                   .\Licenses\License.PortMIDI.txt || goto error
copy /y ..\..\include\pugixml\readme.txt                     .\Licenses\License.PugiXML.txt || goto error
rem copy /y ..\..\include\r8brain\other\License.txt              .\Licenses\License.R8Brain.txt || goto error
rem copy /y ..\..\include\smbPitchShift\smbPitchShift.cpp        .\Licenses\License.smbPitchShift.txt || goto error
rem copy /y ..\..\include\soundtouch\COPYING.TXT                 .\Licenses\License.SoundTouch.txt || goto error
copy /y ..\..\include\stb_vorbis\stb_vorbis.c                .\Licenses\License.stb_vorbis.txt || goto error
rem copy /y ..\..\include\unrar\license.txt                      .\Licenses\License.UnRAR.txt || goto error
rem copy /y ..\..\include\vorbis\COPYING                         .\Licenses\License.Vorbis.txt || goto error
rem copy /y ..\..\include\zlib\README                            .\Licenses\License.zlib.txt || goto error
rem copy /y ..\..\include\zlib\contrib\minizip\MiniZip64_info.txt .\Licenses\License.minizip.txt || goto error
copy /y ..\..\LICENSE .\LICENSE.txt || goto error
copy /y ..\..\libopenmpt\dox\changelog.md .\ || goto error
copy /y ..\..\libopenmpt\doc\xmp-openmpt.txt .\XMPlay\ || goto error
copy /y ..\..\libopenmpt\doc\in_openmpt.txt .\Winamp\ || goto error
copy /y ..\..\bin\Win32\openmpt123.exe .\openmpt123\ || goto error
copy /y ..\..\bin\Win32\xmp-openmpt.dll .\XMPlay\ || goto error
copy /y ..\..\bin\Win32\in_openmpt.dll .\Winamp\ || goto error
"C:\Program Files\7-Zip\7z.exe" a -t7z -mx=9 ..\libopenmpt-winold-r%MPT_REVISION%.7z ^
 LICENSE.txt ^
 Licenses ^
 changelog.md ^
 openmpt123\openmpt123.exe ^
 XMPlay\xmp-openmpt.txt ^
 XMPlay\xmp-openmpt.dll ^
 Winamp\in_openmpt.txt ^
 Winamp\in_openmpt.dll ^
 || goto error
cd .. || goto error
"C:\Program Files\7-Zip\7z.exe" a -ttar libopenmpt-winold.tar libopenmpt-winold-r%MPT_REVISION%.7z || goto error
del /f /q libopenmpt-winold-r%MPT_REVISION%.7z
cd .. || goto error



goto noerror

:error
cd "%MY_DIR%"
exit 1

:noerror
cd "%MY_DIR%"
exit 0
