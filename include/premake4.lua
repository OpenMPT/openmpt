 
 project "UnRAR"
  uuid "95CC809B-03FC-4EDB-BB20-FD07A698C05F"
  language "C++"
  location "../build/gen"
  objdir "../build/obj/unrar"
  includedirs { "../include/unrar" }
  files {
   "../include/unrar/archive.cpp",
   "../include/unrar/arcread.cpp",
   "../include/unrar/blake2s.cpp",
   "../include/unrar/cmddata.cpp",
   "../include/unrar/consio.cpp",
   "../include/unrar/crc.cpp",
   "../include/unrar/crypt.cpp",
   "../include/unrar/encname.cpp",
   "../include/unrar/errhnd.cpp",
   "../include/unrar/extinfo.cpp",
   "../include/unrar/extract.cpp",
   "../include/unrar/filcreat.cpp",
   "../include/unrar/file.cpp",
   "../include/unrar/filefn.cpp",
   "../include/unrar/filestr.cpp",
   "../include/unrar/find.cpp",
   "../include/unrar/getbits.cpp",
   "../include/unrar/global.cpp",
   "../include/unrar/hash.cpp",
   "../include/unrar/headers.cpp",
   "../include/unrar/isnt.cpp",
   "../include/unrar/list.cpp",
   "../include/unrar/match.cpp",
   "../include/unrar/options.cpp",
   "../include/unrar/pathfn.cpp",
   "../include/unrar/qopen.cpp",
   "../include/unrar/rar.cpp",
   "../include/unrar/rarpch.cpp",
   "../include/unrar/rarvm.cpp",
   "../include/unrar/rawread.cpp",
   "../include/unrar/rdwrfn.cpp",
   "../include/unrar/recvol.cpp",
   "../include/unrar/resource.cpp",
   "../include/unrar/rijndael.cpp",
   "../include/unrar/rs.cpp",
   "../include/unrar/rs16.cpp",
   "../include/unrar/scantree.cpp",
   "../include/unrar/secpassword.cpp",
   "../include/unrar/sha1.cpp",
   "../include/unrar/sha256.cpp",
   "../include/unrar/smallfn.cpp",
   "../include/unrar/strfn.cpp",
   "../include/unrar/strlist.cpp",
   "../include/unrar/system.cpp",
   "../include/unrar/threadpool.cpp",
   "../include/unrar/timefn.cpp",
   "../include/unrar/ui.cpp",
   "../include/unrar/unicode.cpp",
   "../include/unrar/unpack.cpp",
   "../include/unrar/volume.cpp",
  }
  files {
   "../include/unrar/archive.hpp",
   "../include/unrar/array.hpp",
   "../include/unrar/blake2s.hpp",
   "../include/unrar/cmddata.hpp",
   "../include/unrar/coder.hpp",
   "../include/unrar/compress.hpp",
   "../include/unrar/consio.hpp",
   "../include/unrar/crc.hpp",
   "../include/unrar/crypt.hpp",
   "../include/unrar/dll.hpp",
   "../include/unrar/encname.hpp",
   "../include/unrar/errhnd.hpp",
   "../include/unrar/extinfo.hpp",
   "../include/unrar/extract.hpp",
   "../include/unrar/filcreat.hpp",
   "../include/unrar/file.hpp",
   "../include/unrar/filefn.hpp",
   "../include/unrar/filestr.hpp",
   "../include/unrar/find.hpp",
   "../include/unrar/getbits.hpp",
   "../include/unrar/global.hpp",
   "../include/unrar/hash.hpp",
   "../include/unrar/headers.hpp",
   "../include/unrar/headers5.hpp",
   "../include/unrar/isnt.hpp",
   "../include/unrar/list.hpp",
   "../include/unrar/loclang.hpp",
   "../include/unrar/log.hpp",
   "../include/unrar/match.hpp",
   "../include/unrar/model.hpp",
   "../include/unrar/openmpt.hpp",
   "../include/unrar/openmpt-callback.hpp",
   "../include/unrar/options.hpp",
   "../include/unrar/os.hpp",
   "../include/unrar/pathfn.hpp",
   "../include/unrar/qopen.hpp",
   "../include/unrar/rar.hpp",
   "../include/unrar/rardefs.hpp",
   "../include/unrar/rarlang.hpp",
   "../include/unrar/raros.hpp",
   "../include/unrar/rartypes.hpp",
   "../include/unrar/rarvm.hpp",
   "../include/unrar/rawread.hpp",
   "../include/unrar/rdwrfn.hpp",
   "../include/unrar/recvol.hpp",
   "../include/unrar/resource.hpp",
   "../include/unrar/rijndael.hpp",
   "../include/unrar/rs.hpp",
   "../include/unrar/rs16.hpp",
   "../include/unrar/savepos.hpp",
   "../include/unrar/scantree.hpp",
   "../include/unrar/secpassword.hpp",
   "../include/unrar/sha1.hpp",
   "../include/unrar/sha256.hpp",
   "../include/unrar/smallfn.hpp",
   "../include/unrar/strfn.hpp",
   "../include/unrar/strlist.hpp",
   "../include/unrar/suballoc.hpp",
   "../include/unrar/system.hpp",
   "../include/unrar/threadpool.hpp",
   "../include/unrar/timefn.hpp",
   "../include/unrar/ulinks.hpp",
   "../include/unrar/ui.hpp",
   "../include/unrar/unicode.hpp",
   "../include/unrar/unpack.hpp",
   "../include/unrar/version.hpp",
   "../include/unrar/volume.hpp",
  }
  dofile "../build/premake4-defaults-LIB.lua"
  dofile "../build/premake4-defaults-static.lua"
  
  
  
 project "zlib"
  uuid "1654FB18-FDE6-406F-9D84-BA12BFBD67B2"
  language "C"
  location "../build/gen"
  objdir "../build/obj/zlib"
  includedirs { "../include/zlib" }
  files {
   "../include/zlib/adler32.c",
   "../include/zlib/compress.c",
   "../include/zlib/crc32.c",
   "../include/zlib/deflate.c",
   "../include/zlib/gzclose.c",
   "../include/zlib/gzlib.c",
   "../include/zlib/gzread.c",
   "../include/zlib/gzwrite.c",
   "../include/zlib/infback.c",
   "../include/zlib/inffast.c",
   "../include/zlib/inflate.c",
   "../include/zlib/inftrees.c",
   "../include/zlib/trees.c",
   "../include/zlib/uncompr.c",
   "../include/zlib/zutil.c",
  }
  files {
   "../include/zlib/crc32.h",
   "../include/zlib/deflate.h",
   "../include/zlib/gzguts.h",
   "../include/zlib/inffast.h",
   "../include/zlib/inffixed.h",
   "../include/zlib/inflate.h",
   "../include/zlib/inftrees.h",
   "../include/zlib/trees.h",
   "../include/zlib/zconf.h",
   "../include/zlib/zlib.h",
   "../include/zlib/zutil.h",
  }
  dofile "../build/premake4-defaults-LIB.lua"
  dofile "../build/premake4-defaults-static.lua"
  
  
  
 project "minizip"
  uuid "63AF9025-A6CE-4147-A05D-6E2CCFD3A0D7"
  language "C"
  location "../build/gen"
  objdir "../build/obj/minizip"
  includedirs { "../include/zlib", "../include/zlib/contrib/minizip" }
  files {
   "../include/zlib/contrib/minizip/ioapi.c",
   "../include/zlib/contrib/minizip/iowin32.c",
   "../include/zlib/contrib/minizip/mztools.c",
   "../include/zlib/contrib/minizip/unzip.c",
   "../include/zlib/contrib/minizip/zip.c",
  }
  files {
   "../include/zlib/contrib/minizip/crypt.h",
   "../include/zlib/contrib/minizip/ioapi.h",
   "../include/zlib/contrib/minizip/iowin32.h",
   "../include/zlib/contrib/minizip/mztools.h",
   "../include/zlib/contrib/minizip/unzip.h",
   "../include/zlib/contrib/minizip/zip.h",
  }
  dofile "../build/premake4-defaults-LIB.lua"
  dofile "../build/premake4-defaults-static.lua"
   
   
   
 project "miniz"
  uuid "B5E0C06B-8121-426A-8FFB-4293ECAAE29C"
  language "C"
  location "../build/gen"
  objdir "../build/obj/miniz"
  files {
   "../include/miniz/miniz.c",
  }
  dofile "../build/premake4-defaults-LIB.lua"
  dofile "../build/premake4-defaults-static.lua"



 project "smbPitchShift"
  uuid "89AF16DD-32CC-4A7E-B219-5F117D761F9F"
  language "C++"
  location "../build/gen"
  objdir "../build/obj/smbPitchShift"
  includedirs { }
  files {
   "../include/smbPitchShift/smbPitchShift.cpp",
  }
  files {
   "../include/smbPitchShift/smbPitchShift.h",
  }
  dofile "../build/premake4-defaults-LIB.lua"
  dofile "../build/premake4-defaults-static.lua"

  
  
 project "lhasa"
  uuid "6B11F6A8-B131-4D2B-80EF-5731A9016436"
  language "C"
  location "../build/gen"
  objdir "../build/obj/lhasa"
  includedirs { "../include/msinttypes/inttypes" }
  files {
   "../include/lhasa/lib/crc16.c",
   "../include/lhasa/lib/ext_header.c",
   "../include/lhasa/lib/lh1_decoder.c",
   "../include/lhasa/lib/lh5_decoder.c",
   "../include/lhasa/lib/lh6_decoder.c",
   "../include/lhasa/lib/lh7_decoder.c",
   "../include/lhasa/lib/lha_arch_unix.c",
   "../include/lhasa/lib/lha_arch_win32.c",
   "../include/lhasa/lib/lha_basic_reader.c",
   "../include/lhasa/lib/lha_decoder.c",
   "../include/lhasa/lib/lha_endian.c",
   "../include/lhasa/lib/lha_file_header.c",
   "../include/lhasa/lib/lha_input_stream.c",
   "../include/lhasa/lib/lha_reader.c",
   "../include/lhasa/lib/lz5_decoder.c",
   "../include/lhasa/lib/lzs_decoder.c",
   "../include/lhasa/lib/macbinary.c",
   "../include/lhasa/lib/null_decoder.c",
   "../include/lhasa/lib/pm1_decoder.c",
   "../include/lhasa/lib/pm2_decoder.c",
  }
  files {
   "../include/lhasa/lib/crc16.h",
   "../include/lhasa/lib/ext_header.h",
   "../include/lhasa/lib/lha_arch.h",
   "../include/lhasa/lib/lha_basic_reader.h",
   "../include/lhasa/lib/lha_decoder.h",
   "../include/lhasa/lib/lha_endian.h",
   "../include/lhasa/lib/lha_file_header.h",
   "../include/lhasa/lib/lha_input_stream.h",
   "../include/lhasa/lib/macbinary.h",
   "../include/lhasa/lib/public/lha_decoder.h",
   "../include/lhasa/lib/public/lha_file_header.h",
   "../include/lhasa/lib/public/lha_input_stream.h",
   "../include/lhasa/lib/public/lha_reader.h",
   "../include/lhasa/lib/public/lhasa.h",
  }
  configuration "vs2008"
   includedirs { "../include/msinttypes/stdint" }
  dofile "../build/premake4-defaults-LIB.lua"
  dofile "../build/premake4-defaults-static.lua"


    
 project "flac"
  uuid "E599F5AA-F9A3-46CC-8DB0-C8DEFCEB90C5"
  language "C"
  location "../build/gen"
  objdir "../build/obj/flac"
  includedirs { "../include/flac/include", "../include/flac/src/libFLAC/include" }
  files {
   "../include/flac/src/libFLAC/bitmath.c",
   "../include/flac/src/libFLAC/bitreader.c",
   "../include/flac/src/libFLAC/bitwriter.c",
   "../include/flac/src/libFLAC/cpu.c",
   "../include/flac/src/libFLAC/crc.c",
   "../include/flac/src/libFLAC/fixed.c",
   "../include/flac/src/libFLAC/float.c",
   "../include/flac/src/libFLAC/format.c",
   "../include/flac/src/libFLAC/lpc.c",
   "../include/flac/src/libFLAC/md5.c",
   "../include/flac/src/libFLAC/memory.c",
   "../include/flac/src/libFLAC/metadata_iterators.c",
   "../include/flac/src/libFLAC/metadata_object.c",
   "../include/flac/src/libFLAC/stream_decoder.c",
   "../include/flac/src/libFLAC/stream_encoder.c",
   "../include/flac/src/libFLAC/stream_encoder_framing.c",
   "../include/flac/src/libFLAC/window.c",
  }
  files {
   "../include/flac/src/share/win_utf8_io/win_utf8_io.c",
  }
  files {
   "../include/flac/src/libFLAC/include/private/all.h",
   "../include/flac/src/libFLAC/include/private/bitmath.h",
   "../include/flac/src/libFLAC/include/private/bitreader.h",
   "../include/flac/src/libFLAC/include/private/bitwriter.h",
   "../include/flac/src/libFLAC/include/private/cpu.h",
   "../include/flac/src/libFLAC/include/private/crc.h",
   "../include/flac/src/libFLAC/include/private/fixed.h",
   "../include/flac/src/libFLAC/include/private/float.h",
   "../include/flac/src/libFLAC/include/private/format.h",
   "../include/flac/src/libFLAC/include/private/lpc.h",
   "../include/flac/src/libFLAC/include/private/md5.h",
   "../include/flac/src/libFLAC/include/private/memory.h",
   "../include/flac/src/libFLAC/include/private/metadata.h",
   "../include/flac/src/libFLAC/include/private/stream_encoder_framing.h",
   "../include/flac/src/libFLAC/include/private/window.h",
   "../include/flac/src/libFLAC/include/protected/all.h",
   "../include/flac/src/libFLAC/include/protected/stream_decoder.h",
   "../include/flac/src/libFLAC/include/protected/stream_encoder.h",
  }
  files {
   "../include/flac/include/FLAC/all.h",
   "../include/flac/include/FLAC/assert.h",
   "../include/flac/include/FLAC/callback.h",
   "../include/flac/include/FLAC/export.h",
   "../include/flac/include/FLAC/format.h",
   "../include/flac/include/FLAC/metadata.h",
   "../include/flac/include/FLAC/ordinals.h",
   "../include/flac/include/FLAC/stream_decoder.h",
   "../include/flac/include/FLAC/stream_encoder.h",
  }
  buildoptions { "/wd4244", "/wd4267", "/wd4334" }
  configuration "*Lib"
   defines { "FLAC__NO_DLL" }
  configuration "vs2010"
   defines { "VERSION=\"1.3.0\"" }
  configuration "vs2008"
   defines { "VERSION=\\\"1.3.0\\\"" }
  dofile "../build/premake4-defaults-LIB.lua"
  dofile "../build/premake4-defaults-static.lua"

  
  
 project "portaudio"
  uuid "189B867F-FF4B-45A1-B741-A97492EE69AF"
  language "C"
  location "../build/gen"
  objdir "../build/obj/portaudio"
  includedirs { "../include/portaudio/include", "../include/portaudio/src/common", "../include/portaudio/src/os/win" }
  defines {
   "PAWIN_USE_WDMKS_DEVICE_INFO",
   "PA_USE_ASIO=0",
   "PA_USE_DS=0",
   "PA_USE_WMME=1",
   "PA_USE_WASAPI=1",
   "PA_USE_WDMKS=1",
  }
  files {
   "../include/portaudio/src/common/pa_allocation.c",
   "../include/portaudio/src/common/pa_allocation.h",
   "../include/portaudio/src/common/pa_converters.c",
   "../include/portaudio/src/common/pa_converters.h",
   "../include/portaudio/src/common/pa_cpuload.c",
   "../include/portaudio/src/common/pa_cpuload.h",
   "../include/portaudio/src/common/pa_debugprint.c",
   "../include/portaudio/src/common/pa_debugprint.h",
   "../include/portaudio/src/common/pa_dither.c",
   "../include/portaudio/src/common/pa_dither.h",
   "../include/portaudio/src/common/pa_endianness.h",
   "../include/portaudio/src/common/pa_front.c",
   "../include/portaudio/src/common/pa_hostapi.h",
   "../include/portaudio/src/common/pa_memorybarrier.h",
   "../include/portaudio/src/common/pa_process.c",
   "../include/portaudio/src/common/pa_process.h",
   "../include/portaudio/src/common/pa_ringbuffer.c",
   "../include/portaudio/src/common/pa_ringbuffer.h",
   "../include/portaudio/src/common/pa_stream.c",
   "../include/portaudio/src/common/pa_stream.h",
   "../include/portaudio/src/common/pa_trace.c",
   "../include/portaudio/src/common/pa_trace.h",
   "../include/portaudio/src/common/pa_types.h",
   "../include/portaudio/src/common/pa_util.h",
   "../include/portaudio/src/hostapi/skeleton/pa_hostapi_skeleton.c",
   "../include/portaudio/src/hostapi/wasapi/pa_win_wasapi.c",
   "../include/portaudio/src/hostapi/wdmks/pa_win_wdmks.c",
   "../include/portaudio/src/hostapi/wmme/pa_win_wmme.c",
   "../include/portaudio/src/os/win/pa_win_coinitialize.c",
   "../include/portaudio/src/os/win/pa_win_coinitialize.h",
   "../include/portaudio/src/os/win/pa_win_hostapis.c",
   "../include/portaudio/src/os/win/pa_win_util.c",
   "../include/portaudio/src/os/win/pa_win_waveformat.c",
   "../include/portaudio/src/os/win/pa_win_wdmks_utils.c",
   "../include/portaudio/src/os/win/pa_win_wdmks_utils.h",
   "../include/portaudio/src/os/win/pa_x86_plain_converters.c",
   "../include/portaudio/src/os/win/pa_x86_plain_converters.h",
  }
  files {
   "../include/portaudio/include/pa_asio.h",
   "../include/portaudio/include/pa_jack.h",
   "../include/portaudio/include/pa_linux_alsa.h",
   "../include/portaudio/include/pa_mac_core.h",
   "../include/portaudio/include/pa_win_ds.h",
   "../include/portaudio/include/pa_win_wasapi.h",
   "../include/portaudio/include/pa_win_waveformat.h",
   "../include/portaudio/include/pa_win_wdmks.h",
   "../include/portaudio/include/pa_win_wmme.h",
   "../include/portaudio/include/portaudio.h",
  }
  buildoptions { "/wd4018", "/wd4267" }
  configuration "Debug*"
   defines { "PA_ENABLE_DEBUG_OUTPUT" }
  dofile "../build/premake4-defaults-LIB.lua"
  dofile "../build/premake4-defaults-static.lua"
  

  
 project "portmidi"
  uuid "2512E2CA-578A-4F10-9363-4BFC9A5EF960"
  language "C"
  location "../build/gen"
  objdir "../build/obj/portmidi"
  includedirs { "../include/portmidi/porttime", "../include/portmidi/pm_common", "../include/portmidi/pm_win" }
  files {
   "../include/portmidi/porttime/porttime.c",
   "../include/portmidi/porttime/ptwinmm.c",
   "../include/portmidi/pm_common/pmutil.c",
   "../include/portmidi/pm_common/portmidi.c",
   "../include/portmidi/pm_win/pmwin.c",
   "../include/portmidi/pm_win/pmwinmm.c",
  }
  files {
   "../include/portmidi/porttime/porttime.h",
   "../include/portmidi/pm_common/pminternal.h",
   "../include/portmidi/pm_common/pmutil.h",
   "../include/portmidi/pm_common/portmidi.h",
   "../include/portmidi/pm_win/pmwinmm.h",
  }
  dofile "../build/premake4-defaults-LIB.lua"
  dofile "../build/premake4-defaults-static.lua"

 
 
 project "soundtouch"
  uuid "F5F8F6DE-84CF-4E9D-91EA-D9B5E2AA36CD"
  language "C++"
  location "../build/gen"
  objdir "../build/obj/soundtouch"
  targetname "OpenMPT_SoundTouch_f32"
  includedirs { "../include/soundtouch/include" }
  files {
   "../include/soundtouch/include/BPMDetect.h",
   "../include/soundtouch/include/FIFOSampleBuffer.h",
   "../include/soundtouch/include/FIFOSamplePipe.h",
   "../include/soundtouch/include/SoundTouch.h",
   "../include/soundtouch/include/STTypes.h",
  }
  files {
   "../include/soundtouch/source/SoundTouch/AAFilter.cpp",
   "../include/soundtouch/source/SoundTouch/BPMDetect.cpp",
   "../include/soundtouch/source/SoundTouch/cpu_detect_x86.cpp",
   "../include/soundtouch/source/SoundTouch/FIFOSampleBuffer.cpp",
   "../include/soundtouch/source/SoundTouch/FIRFilter.cpp",
   "../include/soundtouch/source/SoundTouch/InterpolateCubic.cpp",
   "../include/soundtouch/source/SoundTouch/InterpolateLinear.cpp",
   "../include/soundtouch/source/SoundTouch/InterpolateShannon.cpp",
   "../include/soundtouch/source/SoundTouch/mmx_optimized.cpp",
   "../include/soundtouch/source/SoundTouch/PeakFinder.cpp",
   "../include/soundtouch/source/SoundTouch/RateTransposer.cpp",
   "../include/soundtouch/source/SoundTouch/SoundTouch.cpp",
   "../include/soundtouch/source/SoundTouch/sse_optimized.cpp",
   "../include/soundtouch/source/SoundTouch/TDStretch.cpp",
  }
  files {
   "../include/soundtouch/source/SoundTouch/AAFilter.h",
   "../include/soundtouch/source/SoundTouch/cpu_detect.h",
   "../include/soundtouch/source/SoundTouch/FIRFilter.h",
   "../include/soundtouch/source/SoundTouch/InterpolateCubic.h",
   "../include/soundtouch/source/SoundTouch/InterpolateLinear.h",
   "../include/soundtouch/source/SoundTouch/InterpolateShannon.h",
   "../include/soundtouch/source/SoundTouch/PeakFinder.h",
   "../include/soundtouch/source/SoundTouch/RateTransposer.h",
   "../include/soundtouch/source/SoundTouch/TDStretch.h",
  }
  files {
   "../include/soundtouch/source/SoundTouchDLL/SoundTouchDLL.cpp",
   "../include/soundtouch/source/SoundTouchDLL/SoundTouchDLL.h",
  }
  defines { "DLL_EXPORTS" }
  dofile "../build/premake4-defaults-DLL.lua"
  dofile "../build/premake4-defaults-static.lua"
 
 
 
 