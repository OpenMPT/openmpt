
 project "PluginBridge"
  uuid "1A147336-891E-49AC-9EAD-A750599A224C"
  language "C++"
  vpaths { ["*"] = "../../" }
  mpt_kind "GUI"
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
   "../../pluginBridge/AEffectWrapper.h",
   "../../pluginBridge/Bridge.cpp",
   "../../pluginBridge/Bridge.h",
   "../../pluginBridge/BridgeCommon.h",
   "../../pluginBridge/BridgeOpCodes.h",
   "../../misc/WriteMemoryDump.h",
   "../../common/versionNumber.h",
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
  files {
   "../../pluginBridge/PluginBridge.rc",
  }
	files {
		"../../pluginBridge/PluginBridge.manifest",
	}
  defines { "MODPLUG_TRACKER" }
  dpiawareness "None"
	characterset "Unicode"
	if _OPTIONS["charset"] ~= "Unicode" then
		defines { "MPT_CHECK_WINDOWS_IGNORE_WARNING_NO_UNICODE" }
	end
  warnings "Extra"
  prebuildcommands { "..\\..\\build\\svn_version\\update_svn_version_vs_premake.cmd $(IntDir)" }
  filter { "architecture:x86" }
   targetsuffix "-x86"
  filter { "architecture:x86_64" }
   targetsuffix "-amd64"
  filter { "architecture:ARM" }
   targetsuffix "-arm"
  filter { "architecture:ARM64" }
   targetsuffix "-arm64"
  filter { "architecture:ARM64EC" }
   targetsuffix "-arm64ec"

 project "PluginBridgeLegacy"
  uuid "BDEC2D44-C957-4940-A32B-02824AF6E21D"
  language "C++"
  vpaths { ["*"] = "../../" }
  mpt_kind "GUI"
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
   "../../pluginBridge/AEffectWrapper.h",
   "../../pluginBridge/Bridge.cpp",
   "../../pluginBridge/Bridge.h",
   "../../pluginBridge/BridgeCommon.h",
   "../../pluginBridge/BridgeOpCodes.h",
   "../../misc/WriteMemoryDump.h",
   "../../common/versionNumber.h",
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
  files {
   "../../pluginBridge/PluginBridge.rc",
  }
	files {
		"../../pluginBridge/PluginBridge.manifest",
	}
  defines { "MODPLUG_TRACKER" }
  dpiawareness "None"
  largeaddressaware ( false )
	filter {}
	filter { "action:vs*", "architecture:x86" }
		dataexecutionprevention "Off"
	filter { "action:vs*", "architecture:x86_64" }
		dataexecutionprevention "Off"
	filter { "action:vs*", "architecture:ARM" }
		-- dataexecutionprevention "Off" -- not supported by windows loader on arm64
	filter { "action:vs*", "architecture:ARM64" }
		-- dataexecutionprevention "Off" -- not supported by windows loader on arm64
	filter {}
	characterset "Unicode"
	if _OPTIONS["charset"] ~= "Unicode" then
		defines { "MPT_CHECK_WINDOWS_IGNORE_WARNING_NO_UNICODE" }
	end
  warnings "Extra"
  prebuildcommands { "..\\..\\build\\svn_version\\update_svn_version_vs_premake.cmd $(IntDir)" }
  filter { "architecture:x86" }
   targetsuffix "-x86"
  filter { "architecture:x86_64" }
   targetsuffix "-amd64"
  filter { "architecture:ARM" }
   targetsuffix "-arm"
  filter { "architecture:ARM64" }
   targetsuffix "-arm64"
  filter { "architecture:ARM64EC" }
   targetsuffix "-arm64ec"
  filter {}
	filter {}
	filter { "action:vs*", "architecture:x86_64" }
		linkoptions { "/HIGHENTROPYVA:NO" }
	filter { "action:vs*", "architecture:ARM64" }
		linkoptions { "/HIGHENTROPYVA:NO" }
	filter { "action:vs*", "architecture:ARM64EC" }
		linkoptions { "/HIGHENTROPYVA:NO" }
	filter {}
