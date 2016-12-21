 
 project "mpg123"
  uuid "7adfafb9-0a83-4d35-9891-fb24fdf30b53"
  language "C"
  location ( "../../build/" .. mpt_projectpathname .. "/ext" )
  mpt_projectname = "mpg123"
  dofile "../../build/premake/premake-defaults-DLL.lua"
  dofile "../../build/premake/premake-defaults.lua"
  dofile "../../build/premake/premake-defaults-winver.lua"
  targetname "openmpt-mpg123"
  includedirs {
   "../../include/mpg123/ports/MSVC++",
   "../../include/mpg123/src/libmpg123",
   "../../include/mpg123/src/compat",
   "../../include/mpg123/src",
  }
	filter {}
	filter { "action:vs*" }
		characterset "Unicode"
	filter {}
  files {
   "../../include/mpg123/ports/MSVC++/msvc.c",
  }
  files {
   "../../include/mpg123/src/compat/compat.c",
   "../../include/mpg123/src/compat/compat_str.c",
  }
  files {
   "../../include/mpg123/src/libmpg123/dct64.c",
   --"../../include/mpg123/src/libmpg123/dither.c",
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
   --"../../include/mpg123/src/libmpg123/lfs_alias.c",
   --"../../include/mpg123/src/libmpg123/lfs_wrap.c",
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
