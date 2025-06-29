 
 project "flac"
  uuid "E599F5AA-F9A3-46CC-8DB0-C8DEFCEB90C5"
  language "C"
  location ( "%{wks.location}" .. "/ext" )
  mpt_kind "default"
  targetname "openmpt-flac"
  local extincludedirs = {
		"../../include/ogg/include",
	}
	filter { "action:vs*" }
		includedirs ( extincludedirs )
	filter { "not action:vs*" }
		externalincludedirs ( extincludedirs )
	filter {}
  includedirs {
		"../../include/flac/include",
		"../../include/flac/src/libFLAC/include",
	}
	filter {}
  files {
   "../../include/flac/src/libFLAC/bitmath.c",
   "../../include/flac/src/libFLAC/bitreader.c",
   "../../include/flac/src/libFLAC/bitwriter.c",
   "../../include/flac/src/libFLAC/cpu.c",
   "../../include/flac/src/libFLAC/crc.c",
   "../../include/flac/src/libFLAC/fixed.c",
   "../../include/flac/src/libFLAC/fixed_intrin_avx2.c",
   "../../include/flac/src/libFLAC/fixed_intrin_sse2.c",
   "../../include/flac/src/libFLAC/fixed_intrin_ssse3.c",
   "../../include/flac/src/libFLAC/fixed_intrin_sse42.c",
   "../../include/flac/src/libFLAC/float.c",
   "../../include/flac/src/libFLAC/format.c",
   "../../include/flac/src/libFLAC/lpc.c",
   "../../include/flac/src/libFLAC/lpc_intrin_avx2.c",
   "../../include/flac/src/libFLAC/lpc_intrin_fma.c",
   "../../include/flac/src/libFLAC/lpc_intrin_neon.c",
   "../../include/flac/src/libFLAC/lpc_intrin_sse2.c",
   "../../include/flac/src/libFLAC/lpc_intrin_sse41.c",
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
  files {
   "../../include/flac/include/share/alloc.h",
   "../../include/flac/include/share/compat.h",
   "../../include/flac/include/share/compat_threads.h",
   "../../include/flac/include/share/endswap.h",
   "../../include/flac/include/share/macros.h",
   "../../include/flac/include/share/private.h",
   "../../include/flac/include/share/safe_str.h",
  }
  filter { "action:vs*" }
    files {
     "../../include/flac/include/share/win_utf8_io.h",
    }
  filter {}
  filter { "action:vs*" }
    buildoptions { "/wd4101", "/wd4244", "/wd4267", "/wd4334" }
  filter {}
  filter { "action:vs*" }
    buildoptions { "/wd6001", "/wd6011", "/wd6031", "/wd6297", "/wd6386", "/wd26110", "/wd28182" } -- /analyze
  filter {}
  defines { "FLAC__HAS_OGG=1" }
  links { "ogg" }
  defines { "PACKAGE_VERSION=\"1.5.0\"" }
  filter {}
  filter { "kind:StaticLib" }
   defines { "FLAC__NO_DLL" }
  filter { "kind:SharedLib" }
   defines { "FLAC_API_EXPORTS" }
	filter { "architecture:x86" }
		defines {
			"FLAC__HAS_X86INTRIN",
			"FLAC__USE_AVX",
		}
	filter { "architecture:x86_64" }
		defines {
			"FLAC__HAS_X86INTRIN",
			"FLAC__USE_AVX",
		}
	filter {}
		if _ACTION >= "vs2022" then
			filter {}
			filter {  "not configurations:DebugShared" }
				-- Debug DLL runtime is missing a DLL when using C11 threads
				if _ACTION >= "vs2022" then
					defines { "HAVE_C11THREADS" }
				end
			filter {}
		else
			mpt_use_pthread_win32()
			defines { "HAVE_PTHREAD" }
		end
	filter {}

function mpt_use_flac ()
	filter {}
	filter { "action:vs*" }
		includedirs {
			"../../include/flac/include",
		}
	filter { "not action:vs*" }
		externalincludedirs {
			"../../include/flac/include",
		}
	filter {}
	filter {}
	filter { "configurations:*Shared" }
	filter { "not configurations:*Shared" }
		defines { "FLAC__NO_DLL" }
	filter {}
	links {
		"flac",
	}
	filter {}
end
