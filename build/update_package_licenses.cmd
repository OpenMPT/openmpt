@echo off

set BATCH_DIR=%~dp0
cd %BATCH_DIR%
cd ..

set MY_DIR=%CD%



cd packageTemplate || goto error
copy /y ..\LICENSE .\License.txt || goto error
rmdir /s /q Licenses
mkdir Licenses
copy /y ..\include\bladeenc\BladeDLL.html                 .\Licenses\License.BladeDLL.html || goto error
copy /y ..\include\flac\COPYING.Xiph                      .\Licenses\License.FLAC.txt || goto error
rem copy /y ..\include\foobar2000sdk\sdk-license.txt          .\Licenses\License.Foobar2000SDK.txt || goto error
rem copy /y ..\include\foobar2000sdk\pfc\pfc-license.txt      .\Licenses\License.Foobar2000SDK-pfc.txt || goto error
copy /y ..\include\lhasa\COPYING                          .\Licenses\License.lhasa.txt || goto error
rem copy /y ..\include\miniz\miniz.c                          .\Licenses\License.miniz.txt || goto error
rem copy /y ..\include\msinttypes\stdint\stdint.h             .\Licenses\License.msinttypes.txt || goto error
copy /y ..\include\msinttypes\inttypes\inttypes.h         .\Licenses\License.msinttypes.txt || goto error
copy /y ..\include\ogg\COPYING                            .\Licenses\License.ogg.txt || goto error
copy /y ..\include\opus\COPYING                           .\Licenses\License.Opus.txt || goto error
copy /y ..\include\opusfile\COPYING                       .\Licenses\License.Opusfile.txt || goto error
copy /y ..\include\portaudio\LICENSE.txt                  .\Licenses\License.PortAudio.txt || goto error
rem copy /y ..\include\portaudio\bindings\cpp\COPYING         .\Licenses\License.portaudiocpp.txt || goto error
rem copy /y ..\include\pugixml\readme.txt                     .\Licenses\License.PugiXML.txt || goto error
copy /y ..\include\r8brain\other\License.txt              .\Licenses\License.R8Brain.txt || goto error
copy /y ..\include\rtmidi\License.txt                   .\Licenses\License.RtMidi.txt || goto error
copy /y ..\include\smbPitchShift\smbPitchShift.cpp        .\Licenses\License.smbPitchShift.txt || goto error
copy /y ..\include\soundtouch\COPYING.TXT                 .\Licenses\License.SoundTouch.txt || goto error
rem copy /y ..\include\stb_vorbis\stb_vorbis.c                .\Licenses\License.stb_vorbis.txt || goto error
copy /y ..\include\unrar\license.txt                      .\Licenses\License.UnRAR.txt || goto error
copy /y ..\include\vorbis\COPYING                         .\Licenses\License.Vorbis.txt || goto error
copy /y ..\include\zlib\README                            .\Licenses\License.zlib.txt || goto error
copy /y ..\include\zlib\contrib\minizip\MiniZip64_info.txt .\Licenses\License.minizip.txt || goto error
cd .. || goto error



goto noerror

:error
cd "%MY_DIR%"
exit 1

:noerror
cd "%MY_DIR%"
exit 0
