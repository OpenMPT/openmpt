
 project "PluginBridge"
  uuid "1A147336-891E-49AC-9EAD-A750599A224C"
  language "C++"
  location ( "../../build/" .. mpt_projectpathname )
  vpaths { ["*"] = "../../pluginBridge/" }
  mpt_projectname = "PluginBridge"
  dofile "../../build/premake/premake-defaults-EXEGUI.lua"
  dofile "../../build/premake/premake-defaults.lua"
  local extincludedirs = {
   "../include/vstsdk2.4",
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
  }
  files {
   "../../pluginBridge/PluginBridge.rc",
  }
  files {
   "../../pluginBridge/PluginBridge.manifest",
  }
  defines { "MODPLUG_TRACKER" }
  largeaddressaware ( true )
  characterset "Unicode"
  flags { "Unicode", "WinMain", "ExtraWarnings" }
  prebuildcommands { "..\\..\\build\\svn_version\\update_svn_version_vs_premake.cmd $(IntDir)" }
  filter { "architecture:x86" }
   targetsuffix "32"
  filter { "architecture:x86_64" }
   targetsuffix "64"
  filter {}

	if _OPTIONS["w2k"] then
		filter { "architecture:x86" }
			postbuildcommands {
				"if not exist \"$(TargetDir)\\..\\x86-64-win2000\" mkdir \"$(TargetDir)\\..\\x86-64-win2000\"",
				"copy /y \"$(TargetDir)\\$(TargetFileName)\" \"$(TargetDir)\\..\\x86-64-win2000\\$(TargetFileName)\"",
			}
		filter { "architecture:x86_64" }
			postbuildcommands {
				"if not exist \"$(TargetDir)\\..\\x86-32-win2000\" mkdir \"$(TargetDir)\\..\\x86-32-win2000\"",
				"copy /y \"$(TargetDir)\\$(TargetFileName)\" \"$(TargetDir)\\..\\x86-32-win2000\\$(TargetFileName)\"",
			}
	elseif _OPTIONS["xp"] then
		filter { "architecture:x86" }
			postbuildcommands {
				"if not exist \"$(TargetDir)\\..\\x86-64-winxp\" mkdir \"$(TargetDir)\\..\\x86-64-winxp\"",
				"copy /y \"$(TargetDir)\\$(TargetFileName)\" \"$(TargetDir)\\..\\x86-64-winxp\\$(TargetFileName)\"",
			}
		filter { "architecture:x86_64" }
			postbuildcommands {
				"if not exist \"$(TargetDir)\\..\\x86-32-winxp\" mkdir \"$(TargetDir)\\..\\x86-32-winxp\"",
				"copy /y \"$(TargetDir)\\$(TargetFileName)\" \"$(TargetDir)\\..\\x86-32-winxp\\$(TargetFileName)\"",
			}
	else
		filter { "architecture:x86", "action:vs2008" }
			postbuildcommands {
				"if not exist \"$(TargetDir)\\..\\x86-64-vista\" mkdir \"$(TargetDir)\\..\\x86-64-vista\"",
				"copy /y \"$(TargetDir)\\$(TargetFileName)\" \"$(TargetDir)\\..\\x86-64-vista\\$(TargetFileName)\"",
			}
		filter { "architecture:x86_64", "action:vs2008" }
			postbuildcommands {
				"if not exist \"$(TargetDir)\\..\\x86-32-vista\" mkdir \"$(TargetDir)\\..\\x86-32-vista\"",
				"copy /y \"$(TargetDir)\\$(TargetFileName)\" \"$(TargetDir)\\..\\x86-32-vista\\$(TargetFileName)\"",
			}
		filter { "architecture:x86", "not action:vs2008" }
			postbuildcommands {
				"if not exist \"$(TargetDir)\\..\\x86-64-win7\" mkdir \"$(TargetDir)\\..\\x86-64-win7\"",
				"copy /y \"$(TargetDir)\\$(TargetFileName)\" \"$(TargetDir)\\..\\x86-64-win7\\$(TargetFileName)\"",
			}
		filter { "architecture:x86_64", "not action:vs2008" }
			postbuildcommands {
				"if not exist \"$(TargetDir)\\..\\x86-32-win7\" mkdir \"$(TargetDir)\\..\\x86-32-win7\"",
				"copy /y \"$(TargetDir)\\$(TargetFileName)\" \"$(TargetDir)\\..\\x86-32-win7\\$(TargetFileName)\"",
			}
	end

	filter {}

