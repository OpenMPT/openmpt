 
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
   "../../include/mpg123/src/compat/compat.c",
   "../../include/mpg123/src/compat/compat_str.c",
  }
  files {
   --"../../include/mpg123/src/libmpg123/calctables.c",
   "../../include/mpg123/src/libmpg123/dct64.c",
   "../../include/mpg123/src/libmpg123/dither.c",
   "../../include/mpg123/src/libmpg123/equalizer.c",
   "../../include/mpg123/src/libmpg123/feature.c",
   "../../include/mpg123/src/libmpg123/format.c",
   "../../include/mpg123/src/libmpg123/frame.c",
   "../../include/mpg123/src/libmpg123/icy.c",
   "../../include/mpg123/src/libmpg123/icy2utf8.c",
   "../../include/mpg123/src/libmpg123/id3.c",
   "../../include/mpg123/src/libmpg123/index.c",
   "../../include/mpg123/src/libmpg123/layer1.c",
   "../../include/mpg123/src/libmpg123/layer2.c",
   "../../include/mpg123/src/libmpg123/layer3.c",
   "../../include/mpg123/src/libmpg123/lfs_wrap.c",
   "../../include/mpg123/src/libmpg123/libmpg123.c",
   "../../include/mpg123/src/libmpg123/ntom.c",
   "../../include/mpg123/src/libmpg123/optimize.c",
   "../../include/mpg123/src/libmpg123/parse.c",
   "../../include/mpg123/src/libmpg123/readers.c",
   "../../include/mpg123/src/libmpg123/stringbuf.c",
   "../../include/mpg123/src/libmpg123/synth.c",
   "../../include/mpg123/src/libmpg123/synth_8bit.c",
   "../../include/mpg123/src/libmpg123/synth_real.c",
   "../../include/mpg123/src/libmpg123/synth_s32.c",
   "../../include/mpg123/src/libmpg123/tabinit.c",
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
	filter {}
end
