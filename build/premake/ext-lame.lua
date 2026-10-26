
 project "lame"
  uuid "b545694a-ce2a-44f8-ba88-147c36369308"
  language "C"
  location ( "%{wks.location}" .. "/ext" )
  mpt_kind "shared"
  targetname "openmpt-lame"

	mpt_use_mpg123()

  includedirs { "../../include/lame/include" }
  includedirs { "../../include/lame/libmp3lame" }
  includedirs { "../../include/lame" }
  includedirs { "../../build/premake/inc/lame" }
	filter {}
	files {
		"../../include/lame/include/lame.def",
	}
	files {
		"../../include/lame/include/lame.h",
	}
	files {
		"../../include/lame/mpglib/mpglib.h",
	}
	files {
		"../../include/lame/libmp3lame/bitstream.c",
		"../../include/lame/libmp3lame/bitstream.h",
		"../../include/lame/libmp3lame/encoder.c",
		"../../include/lame/libmp3lame/encoder.h",
		"../../include/lame/libmp3lame/fft.c",
		"../../include/lame/libmp3lame/fft.h",
		"../../include/lame/libmp3lame/gain_analysis.c",
		"../../include/lame/libmp3lame/gain_analysis.h",
		"../../include/lame/libmp3lame/id3tag.c",
		"../../include/lame/libmp3lame/id3tag.h",
		"../../include/lame/libmp3lame/l3side.h",
		"../../include/lame/libmp3lame/lame.c",
		"../../include/lame/libmp3lame/lame_global_flags.h",
		"../../include/lame/libmp3lame/lame-analysis.h",
		"../../include/lame/libmp3lame/lameerror.h",
		"../../include/lame/libmp3lame/machine.h",
		"../../include/lame/libmp3lame/mpglib_interface.c",
		"../../include/lame/libmp3lame/newmdct.c",
		"../../include/lame/libmp3lame/newmdct.h",
		"../../include/lame/libmp3lame/presets.c",
		"../../include/lame/libmp3lame/psymodel.c",
		"../../include/lame/libmp3lame/psymodel.h",
		"../../include/lame/libmp3lame/quantize.c",
		"../../include/lame/libmp3lame/quantize.h",
		"../../include/lame/libmp3lame/quantize_pvt.c",
		"../../include/lame/libmp3lame/quantize_pvt.h",
		"../../include/lame/libmp3lame/reservoir.c",
		"../../include/lame/libmp3lame/reservoir.h",
		"../../include/lame/libmp3lame/set_get.c",
		"../../include/lame/libmp3lame/set_get.h",
		"../../include/lame/libmp3lame/tables.c",
		"../../include/lame/libmp3lame/tables.h",
		"../../include/lame/libmp3lame/takehiro.c",
		"../../include/lame/libmp3lame/util.c",
		"../../include/lame/libmp3lame/util.h",
		"../../include/lame/libmp3lame/vbrquantize.c",
		"../../include/lame/libmp3lame/vbrquantize.h",
		"../../include/lame/libmp3lame/VbrTag.c",
		"../../include/lame/libmp3lame/VbrTag.h",
		"../../include/lame/libmp3lame/VbrTagCrc.h",
		"../../include/lame/libmp3lame/version.c",
		"../../include/lame/libmp3lame/version.h",
	}
	files {
		"../../include/lame/libmp3lame/vector/xmm_quantize_sub.c",
	}
	defines {
		"HAVE_CONFIG_H",
		"HAVE_MPGLIB",
		"HAVE_MPG123",
		"USE_LAYER_2"
	}
  filter {}
  filter { "action:vs*" }
    buildoptions { "/wd4305", "/wd4267", "/wd4334" }
	filter {}
	filter { "action:vs*" }
		buildoptions { "/wd6031", "/wd6262" } -- analyze
	filter {}
		if _OPTIONS["clang"] then
			buildoptions {
				"-Wno-absolute-value",
				"-Wno-tautological-pointer-compare",
				"-Wno-unused-but-set-variable",
				"-Wno-unused-const-variable",
				"-Wno-unused-function",
			}
		end
	filter {}
	if _OPTIONS["windows-version"] ~= "winxp" then
		-- WinXP builds do not use SSE2 by default
		filter {}
		filter { "architecture:x86", "configurations:Checked" }
			defines {
				"HAVE_XMMINTRIN_H",
				"MIN_ARCH_SSE",
			}
		filter { "architecture:x86", "configurations:CheckedShared" }
			defines {
				"HAVE_XMMINTRIN_H",
				"MIN_ARCH_SSE",
			}
		filter { "architecture:x86", "configurations:Release" }
			defines {
				"HAVE_XMMINTRIN_H",
				"MIN_ARCH_SSE",
			}
		filter { "architecture:x86", "configurations:ReleaseShared" }
			defines {
				"HAVE_XMMINTRIN_H",
				"MIN_ARCH_SSE",
			}
		filter {}
	end
	filter {}
	filter { "architecture:x86_64" }
		defines {
			"HAVE_XMMINTRIN_H",
			"MIN_ARCH_SSE",
		}
	filter {}

function mpt_use_lame ()
	filter {}
	filter { "action:vs*" }
		includedirs {
			"../../include/lame/include",
		}
	filter { "not action:vs*" }
		externalincludedirs {
			"../../include/lame/include",
		}
	filter {}
	links {
		"lame",
	}
	filter {}
end
