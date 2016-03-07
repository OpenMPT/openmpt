 
 project "openmpt123"
  uuid "2879F62E-9E2F-4EAB-AE7D-F60C194DD5CB"
  language "C++"
  location ( "../../build/" .. _ACTION )
  objdir "../../build/obj/openmpt123"
  dofile "../../build/premake/premake-defaults-EXE.lua"
  dofile "../../build/premake/premake-defaults.lua"
  includedirs {
   "../..",
   "../../openmpt123",
   "../../include/flac/include",
   "../../include/portaudio/include",
   "$(IntDir)/svn_version",
   "../../build/svn_version",
  }
  files {
   "../../openmpt123/*.cpp",
   "../../openmpt123/*.hpp",
  }
  defines { }
  characterset "Unicode"
  flags { "Unicode" }
  links {
   "libopenmpt",
   "miniz",
   "stb_vorbis",
   "flac",
   "ogg",
   "portaudio",
   "ksuser",
   "winmm",
  }
  filter { "not action:vs2008" }
  links { "delayimp" }
  linkoptions {
   "/DELAYLOAD:mf.dll",
   "/DELAYLOAD:mfplat.dll",
   "/DELAYLOAD:mfreadwrite.dll",
--   "/DELAYLOAD:mfuuid.dll", -- static library
   "/DELAYLOAD:propsys.dll",
  }
  filter {}
  prebuildcommands { "..\\..\\build\\svn_version\\update_svn_version_vs_premake.cmd $(IntDir)" }
