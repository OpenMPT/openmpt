
 project "PluginBridge"
  uuid "1A147336-891E-49AC-9EAD-A750599A224C"
  language "C++"
  location ( "../../build/" .. mpt_projectpathname )
  vpaths { ["*"] = "../../" }
  mpt_projectname = "PluginBridge"
  dofile "../../build/premake/premake-defaults-EXEGUI.lua"
  dofile "../../build/premake/premake-defaults.lua"
	dofile "../../build/premake/premake-defaults-strict.lua"
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
  files {
   "../../pluginBridge/PluginBridge.manifest",
  }
  defines { "MODPLUG_TRACKER" }
  largeaddressaware ( true )
  characterset "Unicode"
  warnings "Extra"
  prebuildcommands { "..\\..\\build\\svn_version\\update_svn_version_vs_premake.cmd $(IntDir)" }
  filter { "architecture:x86" }
   targetsuffix "32"
  filter { "architecture:x86_64" }
   targetsuffix "64"
  filter {}
	filter { "action:vs*", "architecture:x86_64" }
		linkoptions { "/HIGHENTROPYVA:NO" }
	filter {}

	if _OPTIONS["win10"] then
		filter { "architecture:x86", "configurations:*Shared" }
			postbuildcommands {
				"if not exist \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-win10-shared\\amd64\" mkdir \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-win10-shared\\amd64\"",
				"copy /y \"$(TargetDir)\\$(TargetFileName)\" \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-win10-shared\\amd64\\$(TargetFileName)\"",
				"copy /y \"$(TargetDir)\\$(TargetName).pdb\" \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-win10-shared\\amd64\\$(TargetName).pdb\"",
			}
		filter { "architecture:x86_64", "configurations:*Shared" }
			postbuildcommands {
				"if not exist \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-win10-shared\\x86\" mkdir \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-win10-shared\\x86\"",
				"copy /y \"$(TargetDir)\\$(TargetFileName)\" \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-win10-shared\\x86\\$(TargetFileName)\"",
				"copy /y \"$(TargetDir)\\$(TargetName).pdb\" \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-win10-shared\\x86\\$(TargetName).pdb\"",
			}
		filter { "architecture:x86", "not configurations:*Shared" }
			postbuildcommands {
				"if not exist \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-win10-static\\amd64\" mkdir \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-win10-static\\amd64\"",
				"copy /y \"$(TargetDir)\\$(TargetFileName)\" \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-win10-static\\amd64\\$(TargetFileName)\"",
				"copy /y \"$(TargetDir)\\$(TargetName).pdb\" \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-win10-static\\amd64\\$(TargetName).pdb\"",
			}
		filter { "architecture:x86_64", "not configurations:*Shared" }
			postbuildcommands {
				"if not exist \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-win10-static\\x86\" mkdir \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-win10-static\\x86\"",
				"copy /y \"$(TargetDir)\\$(TargetFileName)\" \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-win10-static\\x86\\$(TargetFileName)\"",
				"copy /y \"$(TargetDir)\\$(TargetName).pdb\" \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-win10-static\\x86\\$(TargetName).pdb\"",
			}
	elseif _OPTIONS["xp"] then
		filter { "architecture:x86", "configurations:*Shared" }
			postbuildcommands {
				"if not exist \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-winxp-shared\\amd64\" mkdir \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-winxp-shared\\amd64\"",
				"copy /y \"$(TargetDir)\\$(TargetFileName)\" \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-winxp-shared\\amd64\\$(TargetFileName)\"",
				"copy /y \"$(TargetDir)\\$(TargetName).pdb\" \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-winxp-shared\\amd64\\$(TargetName).pdb\"",
			}
		filter { "architecture:x86_64", "configurations:*Shared" }
			postbuildcommands {
				"if not exist \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-winxp-shared\\x86\" mkdir \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-winxp-shared\\x86\"",
				"copy /y \"$(TargetDir)\\$(TargetFileName)\" \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-winxp-shared\\x86\\$(TargetFileName)\"",
				"copy /y \"$(TargetDir)\\$(TargetName).pdb\" \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-winxp-shared\\x86\\$(TargetName).pdb\"",
			}
		filter { "architecture:x86", "not configurations:*Shared" }
			postbuildcommands {
				"if not exist \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-winxp-static\\amd64\" mkdir \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-winxp-static\\amd64\"",
				"copy /y \"$(TargetDir)\\$(TargetFileName)\" \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-winxp-static\\amd64\\$(TargetFileName)\"",
				"copy /y \"$(TargetDir)\\$(TargetName).pdb\" \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-winxp-static\\amd64\\$(TargetName).pdb\"",
			}
		filter { "architecture:x86_64", "not configurations:*Shared" }
			postbuildcommands {
				"if not exist \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-winxp-static\\x86\" mkdir \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-winxp-static\\x86\"",
				"copy /y \"$(TargetDir)\\$(TargetFileName)\" \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-winxp-static\\x86\\$(TargetFileName)\"",
				"copy /y \"$(TargetDir)\\$(TargetName).pdb\" \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-winxp-static\\x86\\$(TargetName).pdb\"",
			}
	else
		filter { "architecture:x86", "configurations:*Shared" }
			postbuildcommands {
				"if not exist \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-win7-shared\\amd64\" mkdir \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-win7-shared\\amd64\"",
				"copy /y \"$(TargetDir)\\$(TargetFileName)\" \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-win7-shared\\amd64\\$(TargetFileName)\"",
				"copy /y \"$(TargetDir)\\$(TargetName).pdb\" \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-win7-shared\\amd64\\$(TargetName).pdb\"",
			}
		filter { "architecture:x86_64", "configurations:*Shared" }
			postbuildcommands {
				"if not exist \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-win7-shared\\x86\" mkdir \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-win7-shared\\x86\"",
				"copy /y \"$(TargetDir)\\$(TargetFileName)\" \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-win7-shared\\x86\\$(TargetFileName)\"",
				"copy /y \"$(TargetDir)\\$(TargetName).pdb\" \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-win7-shared\\x86\\$(TargetName).pdb\"",
			}
		filter { "architecture:x86", "not configurations:*Shared" }
			postbuildcommands {
				"if not exist \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-win7-static\\amd64\" mkdir \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-win7-static\\amd64\"",
				"copy /y \"$(TargetDir)\\$(TargetFileName)\" \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-win7-static\\amd64\\$(TargetFileName)\"",
				"copy /y \"$(TargetDir)\\$(TargetName).pdb\" \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-win7-static\\amd64\\$(TargetName).pdb\"",
			}
		filter { "architecture:x86_64", "not configurations:*Shared" }
			postbuildcommands {
				"if not exist \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-win7-static\\x86\" mkdir \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-win7-static\\x86\"",
				"copy /y \"$(TargetDir)\\$(TargetFileName)\" \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-win7-static\\x86\\$(TargetFileName)\"",
				"copy /y \"$(TargetDir)\\$(TargetName).pdb\" \"$(TargetDir)\\..\\..\\" .. _ACTION .. "-win7-static\\x86\\$(TargetName).pdb\"",
			}
	end

	filter {}

