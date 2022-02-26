
 project "OpenMPT-NativeSupport"
  uuid "563a631d-fe07-47bc-a98f-9fe5b3ebabfa"
  language "C++"
  vpaths { ["*"] = "../../" }
  mpt_kind "shared"
  includedirs {
   "../../src",
   "../../common",
   "../../include",
   "../../include/asiomodern/include",
   "../../include/ASIOSDK2/common",
   "../../include/nlohmann-json/include",
   "../../include/portaudio/include",
   "../../include/rtaudio",
   "$(IntDir)/svn_version",
   "../../build/svn_version",
  }
  files {
   "../../include/asiomodern/include/ASIOModern/*.hpp",
   "../../src/mpt/**.cpp",
   "../../src/mpt/**.hpp",
   "../../src/openmpt/**.cpp",
   "../../src/openmpt/**.hpp",
   "../../common/*.cpp",
   "../../common/*.h",
   "../../misc/*.cpp",
   "../../misc/*.h",
   "../../mptrack/wine/*.cpp",
   "../../mptrack/wine/*.h",
  }
  excludes {
   "../../mptrack/wine/WineWrapper.cpp",
  }
  defines { "MODPLUG_TRACKER", "MPT_BUILD_WINESUPPORT" }
  largeaddressaware ( true )
  characterset "Unicode"
  warnings "Extra"
  links {
   "portaudio",
   "rtaudio",
  }
  filter {}
  prebuildcommands { "..\\..\\build\\svn_version\\update_svn_version_vs_premake.cmd $(IntDir)" }
