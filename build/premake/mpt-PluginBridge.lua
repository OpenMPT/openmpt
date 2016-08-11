
 project "PluginBridge"
  uuid "1A147336-891E-49AC-9EAD-A750599A224C"
  language "C++"
  location ( "../../build/" .. mpt_projectpathname )
  vpaths { ["*"] = "../../pluginBridge/" }
  objdir "../../build/obj/PluginBridge"
  dofile "../../build/premake/premake-defaults-EXEGUI.lua"
  dofile "../../build/premake/premake-defaults.lua"
  includedirs {
   "../include/vstsdk2.4",
  }
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
  filter { "configurations:Release*", "architecture:x86" }
   postbuildcommands {
    "if not exist \"$(TargetDir)\\..\\x64\" mkdir \"$(TargetDir)\\..\\x64\"",
    "copy /y \"$(TargetDir)\\$(TargetFileName)\" \"$(TargetDir)\\..\\x64\\$(TargetFileName)\"",
   }
  filter { "configurations:Release*", "architecture:x86_64" }
   postbuildcommands {
    "if not exist \"$(TargetDir)\\..\\Win32\" mkdir \"$(TargetDir)\\..\\Win32\"",
    "copy /y \"$(TargetDir)\\$(TargetFileName)\" \"$(TargetDir)\\..\\Win32\\$(TargetFileName)\"",
   }
  filter { "configurations:Debug*", "architecture:x86" }
   postbuildcommands {
    "if not exist \"$(TargetDir)\\..\\x64-Debug\" mkdir \"$(TargetDir)\\..\\x64-Debug\"",
    "copy /y \"$(TargetDir)\\$(TargetFileName)\" \"$(TargetDir)\\..\\x64-Debug\\$(TargetFileName)\"",
   }
  filter { "configurations:Debug*", "architecture:x86_64" }
   postbuildcommands {
    "if not exist \"$(TargetDir)\\..\\Win32-Debug\" mkdir \"$(TargetDir)\\..\\Win32-Debug\"",
    "copy /y \"$(TargetDir)\\$(TargetFileName)\" \"$(TargetDir)\\..\\Win32-Debug\\$(TargetFileName)\"",
   }
  filter {}

