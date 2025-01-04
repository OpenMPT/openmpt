 
 project "mpg123"
  uuid "7adfafb9-0a83-4d35-9891-fb24fdf30b53"
  language "C"
  location ( "%{wks.location}" .. "/ext" )
  mpt_kind "shared"
  targetname "openmpt-mpg123"
  includedirs {
   "../../include/mpg123/ports/generic",
   "../../include/mpg123/src/include",
  }
	filter {}
  files {
   "../../include/mpg123/ports/generic/config.h",
  }
  files {
   "../../include/mpg123/src/common/abi_align.h",
   "../../include/mpg123/src/common/debug.h",
   "../../include/mpg123/src/common/sample.h",
   "../../include/mpg123/src/common/swap_bytes_impl.h",
   "../../include/mpg123/src/common/true.h",
  }
  files {
   "../../include/mpg123/src/compat/compat.c",
   "../../include/mpg123/src/compat/compat.h",
   "../../include/mpg123/src/compat/compat_str.c",
   "../../include/mpg123/src/compat/wpathconv.h",
  }
  files {
   "../../include/mpg123/src/include/fmt123.h",
   "../../include/mpg123/src/include/mpg123.h",
  }
  files {
   --"../../include/mpg123/src/libmpg123/calctables.c",
   "../../include/mpg123/src/libmpg123/costabs.h",
   "../../include/mpg123/src/libmpg123/dct64.c",
   "../../include/mpg123/src/libmpg123/decode.h",
   "../../include/mpg123/src/libmpg123/dither.c",
   "../../include/mpg123/src/libmpg123/dither.h",
   "../../include/mpg123/src/libmpg123/dither_impl.h",
   "../../include/mpg123/src/libmpg123/equalizer.c",
   "../../include/mpg123/src/libmpg123/feature.c",
   "../../include/mpg123/src/libmpg123/format.c",
   "../../include/mpg123/src/libmpg123/frame.c",
   "../../include/mpg123/src/libmpg123/frame.h",
   "../../include/mpg123/src/libmpg123/gapless.h",
   "../../include/mpg123/src/libmpg123/getbits.h",
   "../../include/mpg123/src/libmpg123/getcpuflags.h",
   "../../include/mpg123/src/libmpg123/huffman.h",
   "../../include/mpg123/src/libmpg123/icy.c",
   "../../include/mpg123/src/libmpg123/icy.h",
   "../../include/mpg123/src/libmpg123/icy2utf8.c",
   "../../include/mpg123/src/libmpg123/icy2utf8.h",
   "../../include/mpg123/src/libmpg123/id3.c",
   "../../include/mpg123/src/libmpg123/id3.h",
   "../../include/mpg123/src/libmpg123/index.c",
   "../../include/mpg123/src/libmpg123/index.h",
   "../../include/mpg123/src/libmpg123/init_costabs.h",
   "../../include/mpg123/src/libmpg123/init_layer3.h",
   "../../include/mpg123/src/libmpg123/init_layer12.h",
   "../../include/mpg123/src/libmpg123/l2tables.h",
   "../../include/mpg123/src/libmpg123/l3bandgain.h",
   "../../include/mpg123/src/libmpg123/l12tabs.h",
   "../../include/mpg123/src/libmpg123/layer1.c",
   "../../include/mpg123/src/libmpg123/layer2.c",
   "../../include/mpg123/src/libmpg123/layer3.c",
   "../../include/mpg123/src/libmpg123/lfs_wrap.c",
   "../../include/mpg123/src/libmpg123/lfs_wrap.h",
   "../../include/mpg123/src/libmpg123/libmpg123.c",
   "../../include/mpg123/src/libmpg123/mangle.h",
   "../../include/mpg123/src/libmpg123/mpeghead.h",
   "../../include/mpg123/src/libmpg123/mpg123lib_intern.h",
   "../../include/mpg123/src/libmpg123/newhuffman.h",
   "../../include/mpg123/src/libmpg123/ntom.c",
   "../../include/mpg123/src/libmpg123/optimize.c",
   "../../include/mpg123/src/libmpg123/optimize.h",
   "../../include/mpg123/src/libmpg123/parse.c",
   "../../include/mpg123/src/libmpg123/parse.h",
   "../../include/mpg123/src/libmpg123/readers.c",
   "../../include/mpg123/src/libmpg123/reader.h",
   "../../include/mpg123/src/libmpg123/stringbuf.c",
   "../../include/mpg123/src/libmpg123/synth.c",
   "../../include/mpg123/src/libmpg123/synth.h",
   "../../include/mpg123/src/libmpg123/synth_8bit.c",
   "../../include/mpg123/src/libmpg123/synth_8bit.h",
   "../../include/mpg123/src/libmpg123/synth_mono.h",
   "../../include/mpg123/src/libmpg123/synth_ntom.h",
   "../../include/mpg123/src/libmpg123/synth_real.c",
   "../../include/mpg123/src/libmpg123/synth_s32.c",
   "../../include/mpg123/src/libmpg123/synths.h",
   "../../include/mpg123/src/libmpg123/tabinit.c",
   --"../../include/mpg123/src/libmpg123/testcpu.c",
  }
  defines { "DYNAMIC_BUILD", "OPT_GENERIC" }
  links {
   "shlwapi",
  }
  filter {}
  filter { "action:vs*" }
    buildoptions { "/wd4018", "/wd4244", "/wd4267", "/wd4305", "/wd4334" }
  filter {}
  filter { "action:vs*" }
    buildoptions { "/wd6011", "/wd6285", "/wd6297", "/wd6305", "/wd6385", "/wd6386" } -- /analyze
	filter {}
		if _OPTIONS["clang"] then
			buildoptions {
				"-Wno-unused-function",
			}
		end
	filter {}

function mpt_use_mpg123 ()
	filter {}
	filter { "action:vs*" }
		includedirs {
			"../../include/mpg123/src/include",
		}
	filter { "not action:vs*" }
		externalincludedirs {
			"../../include/mpg123/src/include",
		}
	filter {}
		defines { "MPG123_NO_LARGENAME" }
		links {
			"mpg123",
		}
	filter { "action:vs*" }
		defines { "LINK_MPG123_DLL" }
	filter {}
end
