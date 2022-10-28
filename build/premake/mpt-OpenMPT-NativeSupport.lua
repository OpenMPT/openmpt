
 project "OpenMPT-NativeSupport"
  uuid "563a631d-fe07-47bc-a98f-9fe5b3ebabfa"
  language "C++"
  vpaths { ["*"] = "../../" }
  mpt_kind "shared"
	
	if _OPTIONS["windows-version"] ~= "winxp" then
		mpt_use_asiomodern()
	end
	mpt_use_nlohmannjson()
	mpt_use_portaudio()
	mpt_use_rtaudio()
	
  includedirs {
   "../../src",
   "../../common",
   "$(IntDir)/svn_version",
  }
  files {
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
  defines { "MODPLUG_TRACKER", "MPT_BUILD_WINESUPPORT", "MPT_BUILD_DEFAULT_DEPENDENCIES" }
  largeaddressaware ( true )
  characterset "Unicode"
  warnings "Extra"
  filter {}
  prebuildcommands { "..\\..\\build\\svn_version\\update_svn_version_vs_premake.cmd $(IntDir)" }
