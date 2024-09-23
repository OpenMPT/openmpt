 
 project "updatesigntool"
  uuid "89b3630f-5728-4902-8258-d4dbc532e185"
  language "C++"
  vpaths { ["*"] = "../../" }
  mpt_kind "Console"
	mpt_use_nlohmannjson()
	defines { "MPT_WITH_NLOHMANNJSON" }
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
   "../../installer/updatesigntool/*.cpp",
   "../../installer/updatesigntool/*.h",
  }
	excludes {
		"../../src/mpt/main/**.cpp",
		"../../src/mpt/main/**.hpp",
		"../../src/openmpt/fileformat_base/**.cpp",
		"../../src/openmpt/fileformat_base/**.hpp",
		"../../src/openmpt/soundbase/**.cpp",
		"../../src/openmpt/soundbase/**.hpp",
		"../../src/openmpt/soundfile_data/**.cpp",
		"../../src/openmpt/soundfile_data/**.hpp",
		"../../src/openmpt/soundfile_write/**.cpp",
		"../../src/openmpt/soundfile_write/**.hpp",
		"../../src/openmpt/sounddevice/**.cpp",
		"../../src/openmpt/sounddevice/**.hpp",
		"../../src/openmpt/streamencoder/**.cpp",
		"../../src/openmpt/streamencoder/**.hpp",
	}
  defines { "MODPLUG_TRACKER", "MPT_BUILD_UPDATESIGNTOOL" }
	if _OPTIONS["charset"] ~= "Unicode" then
		defines { "MPT_CHECK_WINDOWS_IGNORE_WARNING_NO_UNICODE" }
	end
  warnings "Extra"
  filter {}
  prebuildcommands { "..\\..\\build\\svn_version\\update_svn_version_vs_premake.cmd $(IntDir)" }
