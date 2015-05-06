
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
  configuration { "x32" }
   targetsuffix "32"
  configuration { "x64" }
   targetsuffix "64"
  dofile "../../build/premake4-win/premake4-defaults-EXEGUI.lua"
  dofile "../../build/premake4-win/premake4-defaults-static.lua"

