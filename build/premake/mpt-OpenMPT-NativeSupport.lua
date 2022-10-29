
 project "OpenMPT-NativeSupport"
  uuid "563a631d-fe07-47bc-a98f-9fe5b3ebabfa"
  language "C++"
  vpaths { ["*"] = "../../" }
  mpt_kind "shared"
	
	if _OPTIONS["windows-version"] ~= "winxp" and not _OPTIONS["clang"] then
		mpt_use_asiomodern()
		defines { "MPT_WITH_ASIO" }
	end
	mpt_use_nlohmannjson()
	defines { "MPT_WITH_NLOHMANNJSON" }
	mpt_use_portaudio()
	defines { "MPT_WITH_PORTAUDIO" }
	mpt_use_rtaudio()
	defines { "MPT_WITH_RTAUDIO" }
	
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
  defines { "MODPLUG_TRACKER", "MPT_BUILD_WINESUPPORT" }
	if _OPTIONS["charset"] ~= "Unicode" then
		defines { "MPT_CHECK_WINDOWS_IGNORE_WARNING_NO_UNICODE" }
	end
  warnings "Extra"
  filter {}
  prebuildcommands { "..\\..\\build\\svn_version\\update_svn_version_vs_premake.cmd $(IntDir)" }
