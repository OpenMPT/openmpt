 
 project "libopenmpt"
  uuid "9C5101EF-3E20-4558-809B-277FDD50E878"
  language "C++"
  location ( "../../build/" .. mpt_projectpathname )
  vpaths { ["*"] = "../../" }
  objdir "../../build/obj/libopenmpt"
  dofile "../../build/premake/premake-defaults-LIBorDLL.lua"
  dofile "../../build/premake/premake-defaults.lua"
  includedirs {
   "../..",
   "../../common",
   "../../soundlib",
   "../../include",
   "../../include/ogg/include",
   "../../include/vorbis/include",
   "../../include/zlib",
   "$(IntDir)/svn_version",
   "../../build/svn_version",
  }
  files {
   "../../common/*.cpp",
   "../../common/*.h",
   "../../soundlib/*.cpp",
   "../../soundlib/*.h",
   "../../soundlib/plugins/*.cpp",
   "../../soundlib/plugins/*.h",
   "../../soundlib/plugins/dmo/*.cpp",
   "../../soundlib/plugins/dmo/*.h",
   "../../libopenmpt/libopenmpt.h",
   "../../libopenmpt/libopenmpt.hpp",
   "../../libopenmpt/libopenmpt_config.h",
   "../../libopenmpt/libopenmpt_ext.hpp",
   "../../libopenmpt/libopenmpt_impl.hpp",
   "../../libopenmpt/libopenmpt_internal.h",
   "../../libopenmpt/libopenmpt_stream_callbacks_fd.h",
   "../../libopenmpt/libopenmpt_stream_callbacks_file.h",
   "../../libopenmpt/libopenmpt_version.h",
   "../../libopenmpt/libopenmpt_c.cpp",
   "../../libopenmpt/libopenmpt_cxx.cpp",
   "../../libopenmpt/libopenmpt_ext.cpp",
   "../../libopenmpt/libopenmpt_impl.cpp",
  }
  characterset "Unicode"
  flags { "Unicode", "ExtraWarnings" }
  defines { "LIBOPENMPT_BUILD" }
  filter { "kind:SharedLib" }
   defines { "LIBOPENMPT_BUILD_DLL" }
  filter { "kind:SharedLib", "not action:vs2008" }
   links { "delayimp" }
   linkoptions {
    "/DELAYLOAD:mf.dll",
    "/DELAYLOAD:mfplat.dll",
    "/DELAYLOAD:mfreadwrite.dll",
--   "/DELAYLOAD:mfuuid.dll", -- static library
    "/DELAYLOAD:propsys.dll",
   }
  filter {}
  links {
   "vorbis",
   "ogg",
   "zlib",
  }
  prebuildcommands { "..\\..\\build\\svn_version\\update_svn_version_vs_premake.cmd $(IntDir)" }
