@echo off

set BATCH_DIR=%~dp0
cd %BATCH_DIR%
cd ..\..

set MY_DIR=%CD%

call build\auto\helper_get_svnversion.cmd

set MPT_REVISION=%SVNVERSION%



cd bin || goto error
rmdir /s /q libopenmpt-win
del /f /q libopenmpt-win.tar
del /f /q libopenmpt-win-r%MPT_REVISION%.7z
mkdir libopenmpt-win
cd libopenmpt-win || goto error
mkdir openmpt123
mkdir openmpt123\x86
mkdir openmpt123\x86_64
mkdir XMPlay
mkdir Winamp
mkdir foobar2000
rmdir /s /q Licenses
mkdir Licenses
rem copy /y ..\..\include\bladeenc\BladeDLL.html                 .\Licenses\License.BladeDLL.html || goto error
copy /y ..\..\include\flac\COPYING.Xiph                      .\Licenses\License.FLAC.txt || goto error
copy /y ..\..\include\foobar2000sdk\sdk-license.txt          .\Licenses\License.Foobar2000SDK.txt || goto error
copy /y ..\..\include\foobar2000sdk\pfc\pfc-license.txt      .\Licenses\License.Foobar2000SDK-pfc.txt || goto error
rem copy /y ..\..\include\lhasa\COPYING                          .\Licenses\License.lhasa.txt || goto error
rem copy /y ..\..\include\miniz\miniz.c                          .\Licenses\License.miniz.txt || goto error
copy /y ..\..\include\msinttypes\stdint\stdint.h             .\Licenses\License.msinttypes.txt || goto error
copy /y ..\..\include\ogg\COPYING                            .\Licenses\License.ogg.txt || goto error
rem copy /y ..\..\include\opus\COPYING                           .\Licenses\License.Opus.txt || goto error
copy /y ..\..\include\portaudio\LICENSE.txt                  .\Licenses\License.PortAudio.txt || goto error
copy /y ..\..\include\portaudio\bindings\cpp\COPYING         .\Licenses\License.portaudiocpp.txt || goto error
rem copy /y ..\..\include\portmidi\license.txt                   .\Licenses\License.PortMIDI.txt || goto error
copy /y ..\..\include\pugixml\readme.txt                     .\Licenses\License.PugiXML.txt || goto error
rem copy /y ..\..\include\r8brain\other\License.txt              .\Licenses\License.R8Brain.txt || goto error
rem copy /y ..\..\include\smbPitchShift\smbPitchShift.cpp        .\Licenses\License.smbPitchShift.txt || goto error
rem copy /y ..\..\include\soundtouch\COPYING.TXT                 .\Licenses\License.SoundTouch.txt || goto error
rem copy /y ..\..\include\stb_vorbis\stb_vorbis.c                .\Licenses\License.stb_vorbis.txt || goto error
rem copy /y ..\..\include\unrar\license.txt                      .\Licenses\License.UnRAR.txt || goto error
copy /y ..\..\include\vorbis\COPYING                         .\Licenses\License.Vorbis.txt || goto error
copy /y ..\..\include\zlib\README                            .\Licenses\License.zlib.txt || goto error
rem copy /y ..\..\include\zlib\contrib\minizip\MiniZip64_info.txt .\Licenses\License.minizip.txt || goto error
copy /y ..\..\LICENSE .\LICENSE.txt || goto error
copy /y ..\..\libopenmpt\dox\changelog.md .\ || goto error
copy /y ..\..\libopenmpt\doc\xmp-openmpt.txt .\XMPlay\ || goto error
copy /y ..\..\libopenmpt\doc\in_openmpt.txt .\Winamp\ || goto error
copy /y ..\..\libopenmpt\doc\foo_openmpt.txt .\foobar2000\ || goto error
copy /y ..\..\bin\Win32\openmpt123.exe .\openmpt123\x86\ || goto error
copy /y ..\..\bin\x64\openmpt123.exe .\openmpt123\x86_64\ || goto error
copy /y ..\..\bin\Win32\xmp-openmpt.dll .\XMPlay\ || goto error
copy /y ..\..\bin\Win32\in_openmpt.dll .\Winamp\ || goto error
copy /y ..\..\bin\Win32\foo_openmpt.dll .\foobar2000\ || goto error
"C:\Program Files\7-Zip\7z.exe" a -t7z -mx=9 ..\libopenmpt-win-r%MPT_REVISION%.7z ^
 LICENSE.txt ^
 Licenses ^
 changelog.md ^
 openmpt123\x86\openmpt123.exe ^
 openmpt123\x86_64\openmpt123.exe ^
 XMPlay\xmp-openmpt.txt ^
 XMPlay\xmp-openmpt.dll ^
 Winamp\in_openmpt.txt ^
 Winamp\in_openmpt.dll ^
 foobar2000\foo_openmpt.txt ^
 foobar2000\foo_openmpt.dll ^
 || goto error
cd .. || goto error
"C:\Program Files\7-Zip\7z.exe" a -ttar libopenmpt-win.tar libopenmpt-win-r%MPT_REVISION%.7z || goto error
del /f /q libopenmpt-win-r%MPT_REVISION%.7z
cd .. || goto error

cd bin || goto error
rmdir /s /q libopenmpt-dev-vs2010
del /f /q libopenmpt-dev-vs2010.tar
del /f /q libopenmpt-dev-vs2010-r%MPT_REVISION%.7z
mkdir libopenmpt-dev-vs2010
cd libopenmpt-dev-vs2010 || goto error
mkdir inc
mkdir inc\libopenmpt
mkdir lib
mkdir lib\x86
mkdir lib\x86_64
mkdir bin
mkdir bin\x86
mkdir bin\x86_64
copy /y ..\..\LICENSE .\LICENSE.txt || goto error
rmdir /s /q Licenses
mkdir Licenses
rem copy /y ..\..\include\bladeenc\BladeDLL.html                 .\Licenses\License.BladeDLL.html || goto error
rem copy /y ..\..\include\flac\COPYING.Xiph                      .\Licenses\License.FLAC.txt || goto error
rem copy /y ..\..\include\foobar2000sdk\sdk-license.txt          .\Licenses\License.Foobar2000SDK.txt || goto error
rem copy /y ..\..\include\foobar2000sdk\pfc\pfc-license.txt      .\Licenses\License.Foobar2000SDK-pfc.txt || goto error
rem copy /y ..\..\include\lhasa\COPYING                          .\Licenses\License.lhasa.txt || goto error
rem copy /y ..\..\include\miniz\miniz.c                          .\Licenses\License.miniz.txt || goto error
copy /y ..\..\include\msinttypes\stdint\stdint.h             .\Licenses\License.msinttypes.txt || goto error
copy /y ..\..\include\ogg\COPYING                            .\Licenses\License.ogg.txt || goto error
rem copy /y ..\..\include\opus\COPYING                           .\Licenses\License.Opus.txt || goto error
rem copy /y ..\..\include\portaudio\LICENSE.txt                  .\Licenses\License.PortAudio.txt || goto error
rem copy /y ..\..\include\portaudio\bindings\cpp\COPYING         .\Licenses\License.portaudiocpp.txt || goto error
rem copy /y ..\..\include\portmidi\license.txt                   .\Licenses\License.PortMIDI.txt || goto error
rem copy /y ..\..\include\pugixml\readme.txt                     .\Licenses\License.PugiXML.txt || goto error
rem copy /y ..\..\include\r8brain\other\License.txt              .\Licenses\License.R8Brain.txt || goto error
rem copy /y ..\..\include\smbPitchShift\smbPitchShift.cpp        .\Licenses\License.smbPitchShift.txt || goto error
rem copy /y ..\..\include\soundtouch\COPYING.TXT                 .\Licenses\License.SoundTouch.txt || goto error
rem copy /y ..\..\include\stb_vorbis\stb_vorbis.c                .\Licenses\License.stb_vorbis.txt || goto error
rem copy /y ..\..\include\unrar\license.txt                      .\Licenses\License.UnRAR.txt || goto error
copy /y ..\..\include\vorbis\COPYING                         .\Licenses\License.Vorbis.txt || goto error
copy /y ..\..\include\zlib\README                            .\Licenses\License.zlib.txt || goto error
rem copy /y ..\..\include\zlib\contrib\minizip\MiniZip64_info.txt .\Licenses\License.minizip.txt || goto error
copy /y ..\..\libopenmpt\dox\changelog.md .\changelog.md || goto error
copy /y ..\..\libopenmpt\libopenmpt.h inc\libopenmpt\ || goto error
copy /y ..\..\libopenmpt\libopenmpt.hpp inc\libopenmpt\ || goto error
copy /y ..\..\libopenmpt\libopenmpt_config.h inc\libopenmpt\ || goto error
copy /y ..\..\libopenmpt\libopenmpt_version.h inc\libopenmpt\ || goto error
copy /y ..\..\libopenmpt\libopenmpt_ext.hpp inc\libopenmpt\ || goto error
copy /y ..\..\bin\Win32-Shared\libopenmpt.lib lib\x86\ || goto error
copy /y ..\..\bin\Win32-Shared\libopenmpt.dll bin\x86\ || goto error
copy /y ..\..\bin\Win32-Shared\openmpt-ogg.dll bin\x86\ || goto error
copy /y ..\..\bin\Win32-Shared\openmpt-vorbis.dll bin\x86\ || goto error
copy /y ..\..\bin\Win32-Shared\openmpt-zlib.dll bin\x86\ || goto error
copy /y ..\..\bin\x64-Shared\libopenmpt.lib lib\x86_64\ || goto error
copy /y ..\..\bin\x64-Shared\libopenmpt.dll bin\x86_64\ || goto error
copy /y ..\..\bin\x64-Shared\openmpt-ogg.dll bin\x86_64\ || goto error
copy /y ..\..\bin\x64-Shared\openmpt-vorbis.dll bin\x86_64\ || goto error
copy /y ..\..\bin\x64-Shared\openmpt-zlib.dll bin\x86_64\ || goto error
"C:\Program Files\7-Zip\7z.exe" a -t7z -mx=9 ..\libopenmpt-dev-vs2010-r%MPT_REVISION%.7z ^
 LICENSE.txt ^
 Licenses ^
 changelog.md ^
 inc\libopenmpt\libopenmpt.h ^
 inc\libopenmpt\libopenmpt.hpp ^
 inc\libopenmpt\libopenmpt_config.h ^
 inc\libopenmpt\libopenmpt_version.h ^
 inc\libopenmpt\libopenmpt_ext.hpp ^
 lib\x86\libopenmpt.lib ^
 lib\x86_64\libopenmpt.lib ^
 bin\x86\libopenmpt.dll ^
 bin\x86\openmpt-ogg.dll ^
 bin\x86\openmpt-vorbis.dll ^
 bin\x86\openmpt-zlib.dll ^
 bin\x86_64\libopenmpt.dll ^
 bin\x86_64\openmpt-ogg.dll ^
 bin\x86_64\openmpt-vorbis.dll ^
 bin\x86_64\openmpt-zlib.dll ^
 || goto error
cd .. || goto error
"C:\Program Files\7-Zip\7z.exe" a -ttar libopenmpt-dev-vs2010.tar libopenmpt-dev-vs2010-r%MPT_REVISION%.7z || goto error
del /f /q libopenmpt-dev-vs2010-r%MPT_REVISION%.7z
cd .. || goto error



goto noerror

:error
cd "%MY_DIR%"
exit 1

:noerror
cd "%MY_DIR%"
exit 0
