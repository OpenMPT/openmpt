 
 project "signtool"
  uuid "89b3630f-5728-4902-8258-d4dbc532e185"
  language "C++"
  location ( "../../build/" .. mpt_projectpathname )
  vpaths { ["*"] = "../../" }
  mpt_projectname = "singtool"
  dofile "../../build/premake/premake-defaults-EXE.lua"
  dofile "../../build/premake/premake-defaults.lua"
  includedirs {
   "../../common",
   "../../include",
   "../../include/nlohmann-json/include",
   "$(IntDir)/svn_version",
   "../../build/svn_version",
  }
  files {
   "../../common/*.cpp",
   "../../common/*.h",
   "../../misc/*.cpp",
   "../../misc/*.h",
   "../../installer/signtool/*.cpp",
   "../../installer/signtool/*.h",
  }
  defines { "MODPLUG_TRACKER", "MPT_BUILD_SIGNTOOL" }
  largeaddressaware ( true )
  characterset "Unicode"
  warnings "Extra"
  links {
   "portaudio",
   "rtaudio",
  }
  filter {}
  prebuildcommands { "..\\..\\build\\svn_version\\update_svn_version_vs_premake.cmd $(IntDir)" }
