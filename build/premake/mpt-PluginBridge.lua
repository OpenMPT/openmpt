
 project "PluginBridge"
  uuid "1A147336-891E-49AC-9EAD-A750599A224C"
  language "C++"
  location ( "../../build/" .. mpt_projectpathname )
  vpaths { ["*"] = "../../" }
  mpt_projectname = "PluginBridge"
  dofile "../../build/premake/premake-defaults-EXEGUI.lua"
  dofile "../../build/premake/premake-defaults.lua"
  local extincludedirs = {
  }
	filter { "action:vs*" }
		includedirs ( extincludedirs )
	filter { "not action:vs*" }
		sysincludedirs ( extincludedirs )
	filter {}
  includedirs {
   "../../common",
   "$(IntDir)/svn_version",
   "../../build/svn_version",
  }
  files {
   "../../pluginBridge/AEffectWrapper.h",
   "../../pluginBridge/Bridge.cpp",
   "../../pluginBridge/Bridge.h",
   "../../pluginBridge/BridgeCommon.h",
   "../../pluginBridge/BridgeOpCodes.h",
   "../../misc/WriteMemoryDump.h",
   "../../common/versionNumber.h",
  }
  files {
   "../../pluginBridge/PluginBridge.rc",
  }
	if _OPTIONS["win10"] then
		files {
			"../../pluginBridge/PluginBridge-win10.manifest",
		}
	elseif _OPTIONS["win81"] then
		files {
			"../../pluginBridge/PluginBridge-win81.manifest",
		}
	elseif _OPTIONS["win7"] then
		files {
			"../../pluginBridge/PluginBridge-win7.manifest",
		}
	end
  defines { "MODPLUG_TRACKER" }
  dpiawareness "None"
  largeaddressaware ( true )
  characterset "Unicode"
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

 project "PluginBridgeLegacy"
  uuid "BDEC2D44-C957-4940-A32B-02824AF6E21D"
  language "C++"
  location ( "../../build/" .. mpt_projectpathname )
  vpaths { ["*"] = "../../" }
  mpt_projectname = "PluginBridgeLegacy"
  dofile "../../build/premake/premake-defaults-EXEGUI.lua"
  dofile "../../build/premake/premake-defaults.lua"
  local extincludedirs = {
  }
	filter { "action:vs*" }
		includedirs ( extincludedirs )
	filter { "not action:vs*" }
		sysincludedirs ( extincludedirs )
	filter {}
  includedirs {
   "../../common",
   "$(IntDir)/svn_version",
   "../../build/svn_version",
  }
  files {
   "../../pluginBridge/AEffectWrapper.h",
   "../../pluginBridge/Bridge.cpp",
   "../../pluginBridge/Bridge.h",
   "../../pluginBridge/BridgeCommon.h",
   "../../pluginBridge/BridgeOpCodes.h",
   "../../misc/WriteMemoryDump.h",
   "../../common/versionNumber.h",
  }
  files {
   "../../pluginBridge/PluginBridge.rc",
  }
	if _OPTIONS["win10"] then
		files {
			"../../pluginBridge/PluginBridge-win10.manifest",
		}
	else
		files {
			"../../pluginBridge/PluginBridge-win7.manifest",
		}
	end
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
  filter {}
	filter {}
	filter { "action:vs*", "architecture:x86_64" }
		linkoptions { "/HIGHENTROPYVA:NO" }
	filter { "action:vs*", "architecture:ARM64" }
		linkoptions { "/HIGHENTROPYVA:NO" }
	filter {}
