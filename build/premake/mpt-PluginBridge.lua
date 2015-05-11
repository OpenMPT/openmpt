
 project "PluginBridge"
  uuid "1A147336-891E-49AC-9EAD-A750599A224C"
  language "C++"
  location ( "../../build/" .. _ACTION )
  objdir "../../build/obj/PluginBridge"
  includedirs {
   "../../common",
   "../include/vstsdk2.4",
   "$(IntDir)/svn_version",
   "../../build/svn_version",
  }
  files {
   "../../pluginBridge/AEffectWrapper.h",
   "../../pluginBridge/Bridge.cpp",
   "../../pluginBridge/Bridge.h",
   "../../pluginBridge/BridgeCommon.h",
   "../../pluginBridge/PluginBridge.rc",
  }
  defines { "MODPLUG_TRACKER" }
  flags { "SEH", "Unicode", "WinMain", "ExtraWarnings" }
  prebuildcommands { "..\\..\\build\\svn_version\\update_svn_version_vs_premake.cmd $(IntDir)" }
  filter { "architecture:x86" }
   targetsuffix "32"
  filter { "architecture:x86_64" }
   targetsuffix "64"
  filter {}
  dofile "../../build/premake/premake-defaults-EXEGUI.lua"
  dofile "../../build/premake/premake-defaults.lua"
  flags { "StaticRuntime" }

