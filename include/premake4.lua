solution "include"
 configurations { "DebugLib", "NormalLib", "ReleaseLib" }
 platforms { "x32", "x64" }
 
 
 
 project "zlib"
  uuid "1654FB18-FDE6-406F-9D84-BA12BFBD67B2"
  language "C"
  location "zlib"
  includedirs { "zlib" }
  files {
   "zlib/adler32.c",
   "zlib/compress.c",
   "zlib/crc32.c",
   "zlib/deflate.c",
   "zlib/gzclose.c",
   "zlib/gzlib.c",
   "zlib/gzread.c",
   "zlib/gzwrite.c",
   "zlib/infback.c",
   "zlib/inffast.c",
   "zlib/inflate.c",
   "zlib/inftrees.c",
   "zlib/trees.c",
   "zlib/uncompr.c",
   "zlib/zutil.c",
  }
  files {
   "zlib/crc32.h",
   "zlib/deflate.h",
   "zlib/gzguts.h",
   "zlib/inffast.h",
   "zlib/inffixed.h",
   "zlib/inflate.h",
   "zlib/inftrees.h",
   "zlib/trees.h",
   "zlib/zconf.h",
   "zlib/zlib.h",
   "zlib/zutil.h",
  }
  configuration "*DLL"
   defines { "ZLIB_DLL" }
  dofile "premake4-defaults.lua"
  
  
  
 project "minizip"
  uuid "63AF9025-A6CE-4147-A05D-6E2CCFD3A0D7"
  language "C"
  location "zlib"
  includedirs { "zlib", "zlib/contrib/minizip" }
  files {
   "zlib/contrib/minizip/ioapi.c",
   "zlib/contrib/minizip/iowin32.c",
   "zlib/contrib/minizip/mztools.c",
   "zlib/contrib/minizip/unzip.c",
   "zlib/contrib/minizip/zip.c",
  }
  files {
   "zlib/contrib/minizip/crypt.h",
   "zlib/contrib/minizip/ioapi.h",
   "zlib/contrib/minizip/iowin32.h",
   "zlib/contrib/minizip/mztools.h",
   "zlib/contrib/minizip/unzip.h",
   "zlib/contrib/minizip/zip.h",
  }
  links { "zlib" }
  configuration "*DLL"
   defines { "ZLIB_DLL" }
  dofile "premake4-defaults.lua"
   
   
   
 project "smbPitchShift"
  uuid "89AF16DD-32CC-4A7E-B219-5F117D761F9F"
  language "C++"
  location "smbPitchShift"
  includedirs { }
  files {
   "smbPitchShift/smbPitchShift.cpp",
  }
  files {
   "smbPitchShift/smbPitchShift.h",
  }
  dofile "premake4-defaults.lua"

  
  
 project "lhasa"
  uuid "6B11F6A8-B131-4D2B-80EF-5731A9016436"
  language "C"
  location "lhasa"
  includedirs { "msinttypes/inttypes" }
  files {
   "lhasa/lib/crc16.c",
   "lhasa/lib/ext_header.c",
   "lhasa/lib/lh1_decoder.c",
   "lhasa/lib/lh5_decoder.c",
   "lhasa/lib/lh6_decoder.c",
   "lhasa/lib/lh7_decoder.c",
   "lhasa/lib/lha_arch_unix.c",
   "lhasa/lib/lha_arch_win32.c",
   "lhasa/lib/lha_basic_reader.c",
   "lhasa/lib/lha_decoder.c",
   "lhasa/lib/lha_endian.c",
   "lhasa/lib/lha_file_header.c",
   "lhasa/lib/lha_input_stream.c",
   "lhasa/lib/lha_reader.c",
   "lhasa/lib/lz5_decoder.c",
   "lhasa/lib/lzs_decoder.c",
   "lhasa/lib/macbinary.c",
   "lhasa/lib/null_decoder.c",
   "lhasa/lib/pm1_decoder.c",
   "lhasa/lib/pm2_decoder.c",
  }
  files {
   "lhasa/lib/crc16.h",
   "lhasa/lib/ext_header.h",
   "lhasa/lib/lha_arch.h",
   "lhasa/lib/lha_basic_reader.h",
   "lhasa/lib/lha_decoder.h",
   "lhasa/lib/lha_endian.h",
   "lhasa/lib/lha_file_header.h",
   "lhasa/lib/lha_input_stream.h",
   "lhasa/lib/macbinary.h",
   "lhasa/lib/public/lha_decoder.h",
   "lhasa/lib/public/lha_file_header.h",
   "lhasa/lib/public/lha_input_stream.h",
   "lhasa/lib/public/lha_reader.h",
   "lhasa/lib/public/lhasa.h",
  }
  configuration "vs2008"
   includedirs { "msinttypes/stdint" }
  dofile "premake4-defaults.lua"


    
 project "flac"
  uuid "E599F5AA-F9A3-46CC-8DB0-C8DEFCEB90C5"
  language "C"
  location "flac"
  includedirs { "flac/include", "flac/src/libFLAC/include" }
  files {
   "flac/src/libFLAC/bitmath.c",
   "flac/src/libFLAC/bitreader.c",
   "flac/src/libFLAC/bitwriter.c",
   "flac/src/libFLAC/cpu.c",
   "flac/src/libFLAC/crc.c",
   "flac/src/libFLAC/fixed.c",
   "flac/src/libFLAC/float.c",
   "flac/src/libFLAC/format.c",
   "flac/src/libFLAC/lpc.c",
   "flac/src/libFLAC/md5.c",
   "flac/src/libFLAC/memory.c",
   "flac/src/libFLAC/metadata_iterators.c",
   "flac/src/libFLAC/metadata_object.c",
   "flac/src/libFLAC/stream_decoder.c",
   "flac/src/libFLAC/stream_encoder.c",
   "flac/src/libFLAC/stream_encoder_framing.c",
   "flac/src/libFLAC/window.c",
  }
  files {
   "flac/src/libFLAC/include/private/all.h",
   "flac/src/libFLAC/include/private/bitmath.h",
   "flac/src/libFLAC/include/private/bitreader.h",
   "flac/src/libFLAC/include/private/bitwriter.h",
   "flac/src/libFLAC/include/private/cpu.h",
   "flac/src/libFLAC/include/private/crc.h",
   "flac/src/libFLAC/include/private/fixed.h",
   "flac/src/libFLAC/include/private/float.h",
   "flac/src/libFLAC/include/private/format.h",
   "flac/src/libFLAC/include/private/lpc.h",
   "flac/src/libFLAC/include/private/md5.h",
   "flac/src/libFLAC/include/private/memory.h",
   "flac/src/libFLAC/include/private/metadata.h",
   "flac/src/libFLAC/include/private/stream_encoder_framing.h",
   "flac/src/libFLAC/include/private/window.h",
   "flac/src/libFLAC/include/protected/all.h",
   "flac/src/libFLAC/include/protected/stream_decoder.h",
   "flac/src/libFLAC/include/protected/stream_encoder.h",
  }
  files {
   "flac/include/FLAC/all.h",
   "flac/include/FLAC/assert.h",
   "flac/include/FLAC/callback.h",
   "flac/include/FLAC/export.h",
   "flac/include/FLAC/format.h",
   "flac/include/FLAC/metadata.h",
   "flac/include/FLAC/ordinals.h",
   "flac/include/FLAC/stream_decoder.h",
   "flac/include/FLAC/stream_encoder.h",
  }
  defines { "VERSION=\\\"1.2.1\\\"" }
  configuration "*Lib"
   defines { "FLAC__NO_DLL" }
  dofile "premake4-defaults.lua"

  
  
 project "portaudio"
  uuid "189B867F-FF4B-45A1-B741-A97492EE69AF"
  language "C"
  location "portaudio"
  includedirs { "portaudio/include", "portaudio/src/common", "portaudio/src/os/win" }
  defines {
   "PA_ENABLE_DEBUG_OUTPUT",
   "PAWIN_USE_WDMKS_DEVICE_INFO",
   "PA_USE_ASIO=0",
   "PA_USE_DS=0",
   "PA_USE_WMME=1",
   "PA_USE_WASAPI=1",
   "PA_USE_WDMKS=0",
  }
  files {
   "portaudio/src/common/pa_allocation.c",
   "portaudio/src/common/pa_allocation.h",
   "portaudio/src/common/pa_converters.c",
   "portaudio/src/common/pa_converters.h",
   "portaudio/src/common/pa_cpuload.c",
   "portaudio/src/common/pa_cpuload.h",
   "portaudio/src/common/pa_debugprint.c",
   "portaudio/src/common/pa_debugprint.h",
   "portaudio/src/common/pa_dither.c",
   "portaudio/src/common/pa_dither.h",
   "portaudio/src/common/pa_endianness.h",
   "portaudio/src/common/pa_front.c",
   "portaudio/src/common/pa_hostapi.h",
   "portaudio/src/common/pa_memorybarrier.h",
   "portaudio/src/common/pa_process.c",
   "portaudio/src/common/pa_process.h",
   "portaudio/src/common/pa_ringbuffer.c",
   "portaudio/src/common/pa_ringbuffer.h",
   "portaudio/src/common/pa_stream.c",
   "portaudio/src/common/pa_stream.h",
   "portaudio/src/common/pa_trace.c",
   "portaudio/src/common/pa_trace.h",
   "portaudio/src/common/pa_types.h",
   "portaudio/src/common/pa_util.h",
   "portaudio/src/hostapi/skeleton/pa_hostapi_skeleton.c",
   "portaudio/src/hostapi/wasapi/pa_win_wasapi.c",
   "portaudio/src/hostapi/wmme/pa_win_wmme.c",
   "portaudio/src/os/win/pa_win_coinitialize.c",
   "portaudio/src/os/win/pa_win_coinitialize.h",
   "portaudio/src/os/win/pa_win_hostapis.c",
   "portaudio/src/os/win/pa_win_util.c",
   "portaudio/src/os/win/pa_win_waveformat.c",
   "portaudio/src/os/win/pa_win_wdmks_utils.c",
   "portaudio/src/os/win/pa_win_wdmks_utils.h",
   "portaudio/src/os/win/pa_x86_plain_converters.c",
   "portaudio/src/os/win/pa_x86_plain_converters.h",
  }
  files {
   "portaudio/include/pa_asio.h",
   "portaudio/include/pa_jack.h",
   "portaudio/include/pa_linux_alsa.h",
   "portaudio/include/pa_mac_core.h",
   "portaudio/include/pa_win_ds.h",
   "portaudio/include/pa_win_wasapi.h",
   "portaudio/include/pa_win_waveformat.h",
   "portaudio/include/pa_win_wmme.h",
   "portaudio/include/portaudio.h",
  }
  dofile "premake4-defaults.lua"
  

  
  
  
solution "includeDLL"
 configurations { "DebugDLL", "NormalDLL", "ReleaseDLL" }
 platforms { "x32", "x64" }
 
 
 
 project "soundtouch"
  uuid "F5F8F6DE-84CF-4E9D-91EA-D9B5E2AA36CD"
  language "C++"
  location "soundtouch"
  targetname "OpenMPT_SoundTouch_i16"
  includedirs { }
  files {
   "soundtouch/AAFilter.cpp",
   "soundtouch/BPMDetect.cpp",
   "soundtouch/cpu_detect_x86_win.cpp",
   "soundtouch/FIFOSampleBuffer.cpp",
   "soundtouch/FIRFilter.cpp",
   "soundtouch/mmx_optimized.cpp",
   "soundtouch/PeakFinder.cpp",
   "soundtouch/RateTransposer.cpp",
   "soundtouch/SoundTouch.cpp",
   "soundtouch/SoundTouchDLL.cpp",
   "soundtouch/sse_optimized.cpp",
   "soundtouch/TDStretch.cpp",
  }
  files {
   "soundtouch/AAFilter.h",
   "soundtouch/BPMDetect.h",
   "soundtouch/cpu_detect.h",
   "soundtouch/FIFOSampleBuffer.h",
   "soundtouch/FIFOSamplePipe.h",
   "soundtouch/FIRFilter.h",
   "soundtouch/PeakFinder.h",
   "soundtouch/RateTransposer.h",
   "soundtouch/SoundTouch.h",
   "soundtouch/SoundTouchDLL.h",
   "soundtouch/STTypes.h",
   "soundtouch/TDStretch.h",
  }
  defines { "DLL_EXPORTS", "SOUNDTOUCH_INTEGER_SAMPLES=1" }
  dofile "premake4-defaults.lua"
 
 
 
 