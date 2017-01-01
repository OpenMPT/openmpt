 
 project "flac"
  uuid "E599F5AA-F9A3-46CC-8DB0-C8DEFCEB90C5"
  language "C"
  location ( "../../build/" .. mpt_projectpathname .. "/ext" )
  mpt_projectname = "flac"
  dofile "../../build/premake/premake-defaults-LIBorDLL.lua"
  dofile "../../build/premake/premake-defaults.lua"
  dofile "../../build/premake/premake-defaults-winver.lua"
  targetname "openmpt-flac"
  local extincludedirs = {
		"../../include/ogg/include",
	}
	filter { "action:vs*" }
		includedirs ( extincludedirs )
	filter { "not action:vs*" }
		sysincludedirs ( extincludedirs )
	filter {}
  includedirs {
		"../../include/flac/include",
		"../../include/flac/src/libFLAC/include",
	}
	filter {}
	filter { "action:vs*" }
		characterset "Unicode"
	filter {}
  files {
   "../../include/flac/src/libFLAC/bitmath.c",
   "../../include/flac/src/libFLAC/bitreader.c",
   "../../include/flac/src/libFLAC/bitwriter.c",
   "../../include/flac/src/libFLAC/cpu.c",
   "../../include/flac/src/libFLAC/crc.c",
   "../../include/flac/src/libFLAC/fixed.c",
   "../../include/flac/src/libFLAC/fixed_intrin_sse2.c",
   "../../include/flac/src/libFLAC/fixed_intrin_ssse3.c",
   "../../include/flac/src/libFLAC/float.c",
   "../../include/flac/src/libFLAC/format.c",
   "../../include/flac/src/libFLAC/lpc.c",
   "../../include/flac/src/libFLAC/lpc_intrin_avx2.c",
   "../../include/flac/src/libFLAC/lpc_intrin_sse2.c",
   "../../include/flac/src/libFLAC/lpc_intrin_sse41.c",
   "../../include/flac/src/libFLAC/lpc_intrin_sse.c",
   "../../include/flac/src/libFLAC/md5.c",
   "../../include/flac/src/libFLAC/memory.c",
   "../../include/flac/src/libFLAC/metadata_iterators.c",
   "../../include/flac/src/libFLAC/metadata_object.c",
   "../../include/flac/src/libFLAC/ogg_decoder_aspect.c",
   "../../include/flac/src/libFLAC/ogg_encoder_aspect.c",
   "../../include/flac/src/libFLAC/ogg_helper.c",
   "../../include/flac/src/libFLAC/ogg_mapping.c",
   "../../include/flac/src/libFLAC/stream_decoder.c",
   "../../include/flac/src/libFLAC/stream_encoder.c",
   "../../include/flac/src/libFLAC/stream_encoder_intrin_avx2.c",
   "../../include/flac/src/libFLAC/stream_encoder_intrin_sse2.c",
   "../../include/flac/src/libFLAC/stream_encoder_intrin_ssse3.c",
   "../../include/flac/src/libFLAC/stream_encoder_framing.c",
   "../../include/flac/src/libFLAC/window.c",
   "../../include/flac/src/libFLAC/windows_unicode_filenames.c",
  }
  files {
   "../../include/flac/src/libFLAC/include/private/all.h",
   "../../include/flac/src/libFLAC/include/private/bitmath.h",
   "../../include/flac/src/libFLAC/include/private/bitreader.h",
   "../../include/flac/src/libFLAC/include/private/bitwriter.h",
   "../../include/flac/src/libFLAC/include/private/cpu.h",
   "../../include/flac/src/libFLAC/include/private/crc.h",
   "../../include/flac/src/libFLAC/include/private/fixed.h",
   "../../include/flac/src/libFLAC/include/private/float.h",
   "../../include/flac/src/libFLAC/include/private/format.h",
   "../../include/flac/src/libFLAC/include/private/lpc.h",
   "../../include/flac/src/libFLAC/include/private/md5.h",
   "../../include/flac/src/libFLAC/include/private/memory.h",
   "../../include/flac/src/libFLAC/include/private/metadata.h",
   "../../include/flac/src/libFLAC/include/private/ogg_decoder_aspect.h",
   "../../include/flac/src/libFLAC/include/private/ogg_encoder_aspect.h",
   "../../include/flac/src/libFLAC/include/private/ogg_helper.h",
   "../../include/flac/src/libFLAC/include/private/ogg_mapping.h",
   "../../include/flac/src/libFLAC/include/private/stream_encoder.h",
   "../../include/flac/src/libFLAC/include/private/stream_encoder_framing.h",
   "../../include/flac/src/libFLAC/include/private/window.h",
   "../../include/flac/src/libFLAC/include/protected/all.h",
   "../../include/flac/src/libFLAC/include/protected/stream_decoder.h",
   "../../include/flac/src/libFLAC/include/protected/stream_encoder.h",
  }
  filter { "action:vs*" }
    files {
     "../../include/flac/src/share/win_utf8_io/win_utf8_io.c",
     "../../include/flac/src/share/windows_unicode_filenames.h",
    }
  filter {}
  files {
   "../../include/flac/include/FLAC/all.h",
   "../../include/flac/include/FLAC/assert.h",
   "../../include/flac/include/FLAC/callback.h",
   "../../include/flac/include/FLAC/export.h",
   "../../include/flac/include/FLAC/format.h",
   "../../include/flac/include/FLAC/metadata.h",
   "../../include/flac/include/FLAC/ordinals.h",
   "../../include/flac/include/FLAC/stream_decoder.h",
   "../../include/flac/include/FLAC/stream_encoder.h",
  }
  filter { "action:vs*" }
    buildoptions { "/wd4244", "/wd4267", "/wd4334" }
  filter {}
  defines { "FLAC__HAS_OGG=1" }
  links { "ogg" }
  defines { "PACKAGE_VERSION=\"1.3.2\"" }
  filter {}
  filter { "kind:StaticLib" }
   defines { "FLAC__NO_DLL" }
  filter { "kind:SharedLib" }
   defines { "FLAC_API_EXPORTS" }
  filter {}
