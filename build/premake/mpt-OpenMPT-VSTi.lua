
 project "OpenMPT-VSTi"
  uuid "d1ad4072-c810-4eea-95d0-27fdfa764834"
  language "C++"
  location ( "../../build/" .. _ACTION )
  objdir "../../build/obj/OpenMPT-VSTi"
  dofile "../../build/premake/premake-defaults-DLL.lua"
  dofile "../../build/premake/premake-defaults.lua"
  includedirs {
   "../../common",
   "../../soundlib",
   "../../include",
   "../../include/msinttypes/inttypes",
   "../../include/vstsdk2.4",
   "../../include/ASIOSDK2/common",
   "../../include/lhasa/lib/public",
   "../../include/ogg/include",
   "../../include/vorbis/include",
   "../../include/zlib",
   "$(IntDir)/svn_version",
   "../../build/svn_version",
  }
  files {
   "../../mptrack/res/OpenMPT.manifest",
  }
  files {
   "../../common/*.cpp",
   "../../common/*.h",
   "../../soundlib/*.cpp",
   "../../soundlib/*.h",
   "../../soundlib/plugins/*.cpp",
   "../../soundlib/plugins/*.h",
   "../../sounddsp/*.cpp",
   "../../sounddsp/*.h",
   "../../sounddev/*.cpp",
   "../../sounddev/*.h",
   "../../unarchiver/*.cpp",
   "../../unarchiver/*.h",
   "../../mptrack/*.cpp",
   "../../mptrack/*.h",
   "../../test/*.cpp",
   "../../test/*.h",
   "../../pluginBridge/BridgeCommon.h",
   "../../pluginBridge/BridgeWrapper.cpp",
   "../../pluginBridge/BridgeWrapper.h",
  }
  files {
   "../../mptrack/mptrack.rc",
   "../../mptrack/res/*.*", -- resource data files
  }
  excludes {
    "../../mptrack/res/rt_manif.bin", -- the old build system manifest
  }
	pchheader "stdafx.h"
	pchsource "../../common/stdafx.cpp"
  defines { "MODPLUG_TRACKER" }
  defines { "OPENMPT_VST" }
  characterset "MBCS"
  flags { "MFC", "SEH", "ExtraWarnings", "WinMain" }
  links {
   "UnRAR",
   "zlib",
   "minizip",
   "smbPitchShift",
   "lhasa",
   "flac",
   "ogg",
   "portaudio",
   "r8brain",
   "soundtouch",
   "stb_vorbis",
  }
  linkoptions {
   "/DELAYLOAD:uxtheme.dll",
  }
  filter { "configurations:*Shared" }
  filter { "not configurations:*Shared" }
   linkoptions {
    "/DELAYLOAD:OpenMPT_SoundTouch_f32.dll",
   }
   targetname "mptrack"
  filter {}
  filter { "not action:vs2008" }
  linkoptions {
   "/DELAYLOAD:mf.dll",
   "/DELAYLOAD:mfplat.dll",
   "/DELAYLOAD:mfreadwrite.dll",
--   "/DELAYLOAD:mfuuid.dll", -- static library
   "/DELAYLOAD:propsys.dll",
  }
  filter {}
  prebuildcommands { "..\\..\\build\\svn_version\\update_svn_version_vs_premake.cmd $(IntDir)" }
