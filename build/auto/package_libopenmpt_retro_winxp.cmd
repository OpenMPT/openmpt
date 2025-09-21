@echo off

set BATCH_DIR=%~dp0
cd %BATCH_DIR%
cd ..\..

set MY_DIR=%CD%

call "build\auto\setup_arguments.cmd" %1 %2 %3 %4 %5 %6

call build\auto\setup_vs_any.cmd

call build\auto\helper_get_svnversion.cmd
call build\auto\helper_get_openmpt_version.cmd

set MPT_REVISION=%LIBOPENMPT_VERSION_STRING%+%SVNVERSION%
if "x%LIBOPENMPT_VERSION_PREREL%" == "x" (
	set MPT_REVISION=%LIBOPENMPT_VERSION_STRING%+release
)


set MPT_PKG_TAG=%MPT_DIST_VARIANT_OS%



cd bin || goto error
rmdir /s /q libopenmpt
mkdir libopenmpt || goto error
mkdir libopenmpt\bin.retro.winxp
mkdir libopenmpt\bin.retro.winxp\%LIBOPENMPT_VERSION_STRING%
rmdir /s /q libopenmpt-retro-winxp
del /f /q libopenmpt-retro-winxp.tar
del /f /q libopenmpt-%MPT_REVISION%.bin.retro.winxp.%MPT_PKG_FORMAT%
mkdir libopenmpt-retro-winxp
cd libopenmpt-retro-winxp || goto error
mkdir openmpt123
mkdir openmpt123\x86
mkdir openmpt123\amd64
mkdir XMPlay
mkdir Winamp
mkdir Winamp2
rmdir /s /q Licenses
mkdir Licenses
copy /y ..\..\src\mpt\LICENSE.BSD-3-Clause.txt                          .\Licenses\License.mpt.BSD-3-Clause.txt || goto error
copy /y ..\..\src\mpt\LICENSE.BSL-1.0.txt                               .\Licenses\License.mpt.BSL-1.0.txt || goto error
rem copy /y ..\..\include\ancient\LICENSE                                   .\Licenses\License.ancient.txt || goto error
rem copy /y ..\..\include\ancient\LICENSE.bzip2                             .\Licenses\License.ancient.bzip2.txt || goto error
rem copy /y ..\..\include\cryptopp\License.txt                              .\Licenses\License.CryptoPP.txt || goto error
copy /y ..\..\include\flac\COPYING.Xiph                                 .\Licenses\License.FLAC.txt || goto error
rem copy /y ..\..\include\lame\COPYING                                      .\Licenses\License.lame.txt || goto error
rem copy /y ..\..\include\lhasa\COPYING.md                                  .\Licenses\License.lhasa.txt || goto error
rem copy /y ..\..\include\minimp3\LICENSE                                   .\Licenses\License.minimp3.txt || goto error
rem copy /y ..\..\include\miniz\miniz.c                                     .\Licenses\License.miniz.txt || goto error
copy /y ..\..\include\mpg123\COPYING                                    .\Licenses\License.mpg123.txt || goto error
copy /y ..\..\include\mpg123\AUTHORS                                    .\Licenses\License.mpg123.Authors.txt || goto error
rem copy /y ..\..\include\nlohmann-json\LICENSE.MIT                          .\Licenses\License.nlohmann-json.txt || goto error
copy /y ..\..\include\ogg\COPYING                                       .\Licenses\License.ogg.txt || goto error
rem copy /y ..\..\include\opus\COPYING                                      .\Licenses\License.Opus.txt || goto error
rem copy /y ..\..\include\opusenc\COPYING                                   .\Licenses\License.Opusenc.txt || goto error
rem copy /y ..\..\include\opusenc\AUTHORS                                   .\Licenses\License.Opusenc.Authors.txt || goto error
rem copy /y ..\..\include\opusfile\COPYING                                  .\Licenses\License.Opusfile.txt || goto error
copy /y ..\..\include\portaudio\LICENSE.txt                             .\Licenses\License.PortAudio.txt || goto error
copy /y ..\..\include\portaudio\bindings\cpp\COPYING                    .\Licenses\License.portaudiocpp.txt || goto error
copy /y ..\..\include\pugixml\LICENSE.md                                .\Licenses\License.PugiXML.txt || goto error
rem copy /y ..\..\include\r8brain\LICENSE                                   .\Licenses\License.R8Brain.txt || goto error
rem copy /y ..\..\include\rtaudio\README.md                                 .\Licenses\License.RtAudio.txt || goto error
rem copy /y ..\..\include\rtmidi\LICENSE                                    .\Licenses\License.RtMidi.txt || goto error
rem copy /y ..\..\include\SignalsmithStretch\SignalsmithStretch\LICENSE.txt .\Licenses\License.SignalsmithStretch.txt || goto error
rem copy /y ..\..\include\stb_vorbis\stb_vorbis.c                           .\Licenses\License.stb_vorbis.txt || goto error
rem copy /y ..\..\include\unrar\license.txt                                 .\Licenses\License.UnRAR.txt || goto error
copy /y ..\..\include\vorbis\COPYING                                    .\Licenses\License.Vorbis.txt || goto error
copy /y ..\..\include\zlib\README                                       .\Licenses\License.zlib.txt || goto error
rem copy /y ..\..\include\zlib\contrib\minizip\MiniZip64_info.txt           .\Licenses\License.minizip.txt || goto error
copy /y ..\..\LICENSE .\LICENSE.txt || goto error
copy /y ..\..\doc\libopenmpt\changelog.md .\ || goto error
copy /y ..\..\libopenmpt\xmp-openmpt\xmp-openmpt.txt .\XMPlay\ || goto error
copy /y ..\..\libopenmpt\in_openmpt\in_openmpt.txt .\Winamp\ || goto error
copy /y ..\..\libopenmpt\in_openmpt\in_openmpt.txt .\Winamp2\ || goto error
copy /y ..\..\bin\release\vs2017-winxp-static\x86\openmpt123.exe .\openmpt123\x86\ || goto error
copy /y ..\..\bin\release\vs2017-winxp-static\x86\openmpt-mpg123.dll .\openmpt123\x86\ || goto error
copy /y ..\..\bin\release\vs2017-winxp-static\amd64\openmpt123.exe .\openmpt123\amd64\ || goto error
copy /y ..\..\bin\release\vs2017-winxp-static\amd64\openmpt-mpg123.dll .\openmpt123\amd64\ || goto error
copy /y ..\..\bin\release\vs2017-winxp-static\x86\xmp-openmpt.dll .\XMPlay\ || goto error
copy /y ..\..\bin\release\vs2017-winxp-static\x86\openmpt-mpg123.dll .\XMPlay\ || goto error
copy /y ..\..\bin\release\vs2017-winxp-static\x86\in_openmpt.dll .\Winamp\ || goto error
copy /y ..\..\bin\release\vs2017-winxp-static\x86\openmpt-mpg123.dll .\Winamp\ || goto error
copy /y ..\..\bin\release\vs2017-winxpansi-static\x86\in_openmpt.dll .\Winamp2\ || goto error
copy /y ..\..\bin\release\vs2017-winxpansi-static\x86\openmpt-mpg123.dll .\Winamp2\ || goto error
..\..\build\tools\7zip\7z.exe a -t%MPT_PKG_FORMAT% -mx=9 %MPT_PKG_FORMAT_OPTIONS% ..\libopenmpt\bin.retro.winxp\%LIBOPENMPT_VERSION_STRING%\libopenmpt-%MPT_REVISION%.bin.retro.winxp.%MPT_PKG_FORMAT% ^
 LICENSE.txt ^
 Licenses ^
 changelog.md ^
 openmpt123\x86\openmpt123.exe ^
 openmpt123\x86\openmpt-mpg123.dll ^
 openmpt123\amd64\openmpt123.exe ^
 openmpt123\amd64\openmpt-mpg123.dll ^
 XMPlay\xmp-openmpt.txt ^
 XMPlay\xmp-openmpt.dll ^
 XMPlay\openmpt-mpg123.dll ^
 Winamp\in_openmpt.txt ^
 Winamp\in_openmpt.dll ^
 Winamp\openmpt-mpg123.dll ^
 Winamp2\in_openmpt.txt ^
 Winamp2\in_openmpt.dll ^
 Winamp2\openmpt-mpg123.dll ^
 || goto error
cd .. || goto error
rmdir /s /q libopenmpt-retro-winxp
..\build\tools\7zip\7z.exe a -r -ttar libopenmpt-retro-winxp.tar libopenmpt || goto error
del /f /q libopenmpt\bin.retro.winxp\%LIBOPENMPT_VERSION_STRING%\libopenmpt-%MPT_REVISION%.bin.retro.winxp.%MPT_PKG_FORMAT%
rmdir /s /q libopenmpt
cd .. || goto error

cd bin || goto error
rmdir /s /q libopenmpt
mkdir libopenmpt || goto error
mkdir libopenmpt\dev.windows.retro.winxp
mkdir libopenmpt\dev.windows.retro.winxp\%LIBOPENMPT_VERSION_STRING%
rmdir /s /q libopenmpt-dev-retro-winxp
del /f /q libopenmpt-dev-retro-winxp.tar
del /f /q libopenmpt-%MPT_REVISION%.dev.windows.retro.winxp.%MPT_PKG_FORMAT%
mkdir libopenmpt-dev-retro-winxp
cd libopenmpt-dev-retro-winxp || goto error
mkdir inc
mkdir inc\libopenmpt
mkdir lib
mkdir lib\x86-retro-winxp
mkdir lib\amd64-retro-winxp
mkdir bin
mkdir bin\x86-retro-winxp
mkdir bin\amd64-retro-winxp
copy /y ..\..\LICENSE .\LICENSE.txt || goto error
rmdir /s /q Licenses
mkdir Licenses
copy /y ..\..\src\mpt\LICENSE.BSD-3-Clause.txt                          .\Licenses\License.mpt.BSD-3-Clause.txt || goto error
copy /y ..\..\src\mpt\LICENSE.BSL-1.0.txt                               .\Licenses\License.mpt.BSL-1.0.txt || goto error
rem copy /y ..\..\include\ancient\LICENSE                                   .\Licenses\License.ancient.txt || goto error
rem copy /y ..\..\include\ancient\LICENSE.bzip2                             .\Licenses\License.ancient.bzip2.txt || goto error
rem copy /y ..\..\include\flac\COPYING.Xiph                                 .\Licenses\License.FLAC.txt || goto error
rem copy /y ..\..\include\lame\COPYING                                      .\Licenses\License.lame.txt || goto error
rem copy /y ..\..\include\lhasa\COPYING.md                                  .\Licenses\License.lhasa.txt || goto error
rem copy /y ..\..\include\minimp3\LICENSE                                   .\Licenses\License.minimp3.txt || goto error
rem copy /y ..\..\include\miniz\LICENSE                                     .\Licenses\License.miniz.txt || goto error
copy /y ..\..\include\mpg123\COPYING                                    .\Licenses\License.mpg123.txt || goto error
copy /y ..\..\include\mpg123\AUTHORS                                    .\Licenses\License.mpg123.Authors.txt || goto error
rem copy /y ..\..\include\nlohmann-json\LICENSE.MIT                         .\Licenses\License.nlohmann-json.txt || goto error
copy /y ..\..\include\ogg\COPYING                                       .\Licenses\License.ogg.txt || goto error
rem copy /y ..\..\include\opus\COPYING                                      .\Licenses\License.Opus.txt || goto error
rem copy /y ..\..\include\opusenc\COPYING                                   .\Licenses\License.Opusenc.txt || goto error
rem copy /y ..\..\include\opusenc\AUTHORS                                   .\Licenses\License.Opusenc.Authors.txt || goto error
rem copy /y ..\..\include\opusfile\COPYING                                  .\Licenses\License.Opusfile.txt || goto error
rem copy /y ..\..\include\portaudio\LICENSE.txt                             .\Licenses\License.PortAudio.txt || goto error
rem copy /y ..\..\include\portaudio\bindings\cpp\COPYING                    .\Licenses\License.portaudiocpp.txt || goto error
rem copy /y ..\..\include\pugixml\LICENSE.md                                .\Licenses\License.PugiXML.txt || goto error
rem copy /y ..\..\include\r8brain\LICENSE                                   .\Licenses\License.R8Brain.txt || goto error
rem copy /y ..\..\include\rtaudio\README.md                                 .\Licenses\License.RtAudio.txt || goto error
rem copy /y ..\..\include\rtmidi\LICENSE                                    .\Licenses\License.RtMidi.txt || goto error
rem copy /y ..\..\include\SignalsmithStretch\SignalsmithStretch\LICENSE.txt .\Licenses\License.SignalsmithStretch.txt || goto error
rem copy /y ..\..\include\stb_vorbis\stb_vorbis.c                           .\Licenses\License.stb_vorbis.txt || goto error
rem copy /y ..\..\include\unrar\license.txt                                 .\Licenses\License.UnRAR.txt || goto error
copy /y ..\..\include\vorbis\COPYING                                    .\Licenses\License.Vorbis.txt || goto error
copy /y ..\..\include\zlib\README                                       .\Licenses\License.zlib.txt || goto error
rem copy /y ..\..\include\zlib\contrib\minizip\MiniZip64_info.txt           .\Licenses\License.minizip.txt || goto error
copy /y ..\..\doc\libopenmpt\changelog.md .\changelog.md || goto error
copy /y ..\..\libopenmpt\libopenmpt.h inc\libopenmpt\ || goto error
copy /y ..\..\libopenmpt\libopenmpt.hpp inc\libopenmpt\ || goto error
copy /y ..\..\libopenmpt\libopenmpt_config.h inc\libopenmpt\ || goto error
copy /y ..\..\libopenmpt\libopenmpt_version.h inc\libopenmpt\ || goto error
copy /y ..\..\libopenmpt\libopenmpt_ext.h inc\libopenmpt\ || goto error
copy /y ..\..\libopenmpt\libopenmpt_ext.hpp inc\libopenmpt\ || goto error
copy /y ..\..\libopenmpt\libopenmpt_stream_callbacks_buffer.h inc\libopenmpt\ || goto error
copy /y ..\..\libopenmpt\libopenmpt_stream_callbacks_fd.h inc\libopenmpt\ || goto error
copy /y ..\..\libopenmpt\libopenmpt_stream_callbacks_file.h inc\libopenmpt\ || goto error
copy /y ..\..\libopenmpt\libopenmpt_stream_callbacks_file_mingw.h inc\libopenmpt\ || goto error
copy /y ..\..\libopenmpt\libopenmpt_stream_callbacks_file_msvcrt.h inc\libopenmpt\ || goto error
copy /y ..\..\libopenmpt\libopenmpt_stream_callbacks_file_posix.h inc\libopenmpt\ || goto error
copy /y ..\..\libopenmpt\libopenmpt_stream_callbacks_file_posix_lfs64.h inc\libopenmpt\ || goto error
copy /y ..\..\bin\release\vs2017-winxp-shared\x86\libopenmpt.lib lib\x86-retro-winxp\ || goto error
copy /y ..\..\bin\release\vs2017-winxp-shared\x86\libopenmpt.dll bin\x86-retro-winxp\ || goto error
copy /y ..\..\bin\release\vs2017-winxp-shared\x86\openmpt-mpg123.dll bin\x86-retro-winxp\ || goto error
copy /y ..\..\bin\release\vs2017-winxp-shared\x86\openmpt-ogg.dll bin\x86-retro-winxp\ || goto error
copy /y ..\..\bin\release\vs2017-winxp-shared\x86\openmpt-vorbis.dll bin\x86-retro-winxp\ || goto error
copy /y ..\..\bin\release\vs2017-winxp-shared\x86\openmpt-zlib.dll bin\x86-retro-winxp\ || goto error
copy /y ..\..\bin\release\vs2017-winxp-shared\amd64\libopenmpt.lib lib\amd64-retro-winxp\ || goto error
copy /y ..\..\bin\release\vs2017-winxp-shared\amd64\libopenmpt.dll bin\amd64-retro-winxp\ || goto error
copy /y ..\..\bin\release\vs2017-winxp-shared\amd64\openmpt-mpg123.dll bin\amd64-retro-winxp\ || goto error
copy /y ..\..\bin\release\vs2017-winxp-shared\amd64\openmpt-ogg.dll bin\amd64-retro-winxp\ || goto error
copy /y ..\..\bin\release\vs2017-winxp-shared\amd64\openmpt-vorbis.dll bin\amd64-retro-winxp\ || goto error
copy /y ..\..\bin\release\vs2017-winxp-shared\amd64\openmpt-zlib.dll bin\amd64-retro-winxp\ || goto error
..\..\build\tools\7zip\7z.exe a -t%MPT_PKG_FORMAT% -mx=9 %MPT_PKG_FORMAT_OPTIONS% ..\libopenmpt\dev.windows.retro.winxp\%LIBOPENMPT_VERSION_STRING%\libopenmpt-%MPT_REVISION%.dev.windows.retro.winxp.%MPT_PKG_FORMAT% ^
 LICENSE.txt ^
 Licenses ^
 changelog.md ^
 inc\libopenmpt\libopenmpt.h ^
 inc\libopenmpt\libopenmpt.hpp ^
 inc\libopenmpt\libopenmpt_config.h ^
 inc\libopenmpt\libopenmpt_version.h ^
 inc\libopenmpt\libopenmpt_ext.h ^
 inc\libopenmpt\libopenmpt_ext.hpp ^
 inc\libopenmpt\libopenmpt_stream_callbacks_buffer.h ^
 inc\libopenmpt\libopenmpt_stream_callbacks_fd.h ^
 inc\libopenmpt\libopenmpt_stream_callbacks_file.h ^
 inc\libopenmpt\libopenmpt_stream_callbacks_file_mingw.h ^
 inc\libopenmpt\libopenmpt_stream_callbacks_file_msvcrt.h ^
 inc\libopenmpt\libopenmpt_stream_callbacks_file_posix.h ^
 inc\libopenmpt\libopenmpt_stream_callbacks_file_posix_lfs64.h ^
 lib\x86-retro-winxp\libopenmpt.lib ^
 lib\amd64-retro-winxp\libopenmpt.lib ^
 bin\x86-retro-winxp\libopenmpt.dll ^
 bin\x86-retro-winxp\openmpt-mpg123.dll ^
 bin\x86-retro-winxp\openmpt-ogg.dll ^
 bin\x86-retro-winxp\openmpt-vorbis.dll ^
 bin\x86-retro-winxp\openmpt-zlib.dll ^
 bin\amd64-retro-winxp\libopenmpt.dll ^
 bin\amd64-retro-winxp\openmpt-mpg123.dll ^
 bin\amd64-retro-winxp\openmpt-ogg.dll ^
 bin\amd64-retro-winxp\openmpt-vorbis.dll ^
 bin\amd64-retro-winxp\openmpt-zlib.dll ^
 || goto error
cd .. || goto error
rmdir /s /q libopenmpt-dev-retro-winxp
..\build\tools\7zip\7z.exe a -r -ttar libopenmpt-dev-retro-winxp.tar libopenmpt || goto error
del /f /q libopenmpt\dev.windows.retro.winxp\%LIBOPENMPT_VERSION_STRING%\libopenmpt-%MPT_REVISION%.dev.windows.retro.winxp.%MPT_PKG_FORMAT%
rmdir /s /q libopenmpt
cd .. || goto error



goto noerror

:error
cd "%MY_DIR%"
exit 1

:noerror
cd "%MY_DIR%"
exit 0
