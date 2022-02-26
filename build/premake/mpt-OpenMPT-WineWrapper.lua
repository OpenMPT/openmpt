
 project "OpenMPT-WineWrapper"
  uuid "f3da2bf5-e84a-4f71-80ab-884594863d3a"
  language "C"
  vpaths { ["*"] = "../../" }
  mpt_kind "shared"
  includedirs {
   "../../src",
   "../../common",
   "../../include",
   "$(IntDir)/svn_version",
   "../../build/svn_version",
  }
  files {
   "../../mptrack/wine/WineWrapper.c",
  }
  defines { "MODPLUG_TRACKER", "MPT_BUILD_WINESUPPORT_WRAPPER" }
  largeaddressaware ( true )
  characterset "Unicode"
  warnings "Extra"
  links {
   "OpenMPT-NativeSupport",
  }
  filter {}
  postbuildcommands { "..\\..\\build\\wine\\build_wine_support.cmd $(IntDir) $(OutDir)" }
