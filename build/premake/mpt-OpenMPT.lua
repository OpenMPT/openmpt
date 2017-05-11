
if layout == "custom" then
  project "OpenMPT-custom"
  mpt_projectname = "OpenMPT-custom"
   uuid "6b9af880-af37-4268-bb91-2b982ff6499a"
else
  project "OpenMPT"
  mpt_projectname = "OpenMPT"
   uuid "37FC32A4-8DDC-4A9C-A30C-62989DD8ACE9"
end
  language "C++"
  location ( "../../build/" .. mpt_projectpathname )
if layout == "custom" then
  vpaths {
		["*"] = "../../",
		["Sources/*"] = "../../**.cpp",
		["Headers/*"] = "../../**.h",
	}
else
  vpaths { ["*"] = "../../" }
end
  dofile "../../build/premake/premake-defaults-EXEGUI.lua"
  dofile "../../build/premake/premake-defaults.lua"
  filter { "configurations:*Shared" }
   targetname "OpenMPT"
  filter { "not configurations:*Shared" }
   targetname "mptrack"
  filter {}
  local extincludedirs = {
   "../../include",
   "../../include/vstsdk2.4",
   "../../include/ASIOSDK2/common",
   "../../include/flac/include",
   "../../include/lame/include",
   "../../include/lhasa/lib/public",
   "../../include/mpg123/ports/MSVC++",
   "../../include/mpg123/src/libmpg123",
   "../../include/ogg/include",
   "../../include/opus/include",
   "../../include/opusfile/include",
   "../../include/portaudio/include",
   "../../include/vorbis/include",
   "../../include/zlib",
  }
	filter { "action:vs*" }
		includedirs ( extincludedirs )
	filter { "not action:vs*" }
		sysincludedirs ( extincludedirs )
	filter {}
  includedirs {
   "../../common",
   "../../soundlib",
   "$(IntDir)/svn_version",
   "../../build/svn_version",
  }
  files {
   "../../mptrack/res/OpenMPT.manifest",
  }
  files {
   "../../common/*.cpp",
   "../../common/*.h",
   "../../soundbase/*.cpp",
   "../../soundbase/*.h",
   "../../soundlib/*.cpp",
   "../../soundlib/*.h",
   "../../soundlib/plugins/*.cpp",
   "../../soundlib/plugins/*.h",
   "../../soundlib/plugins/dmo/*.cpp",
   "../../soundlib/plugins/dmo/*.h",
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
   "../../plugins/MidiInOut/*.cpp",
   "../../plugins/MidiInOut/*.h",
  }
  files {
   "../../mptrack/mptrack.rc",
   "../../mptrack/res/*.*", -- resource data files
  }
	pchheader "stdafx.h"
	pchsource "../../common/stdafx.cpp"
  defines { "MODPLUG_TRACKER" }
  largeaddressaware ( true )
  characterset "MBCS"
  flags { "MFC", "ExtraWarnings", "WinMain" }
  links {
   "UnRAR",
   "zlib",
   "minizip",
   "smbPitchShift",
   "lhasa",
   "flac",
   "mpg123",
   "ogg",
   "opus",
   "opusfile",
   "portaudio",
   "r8brain",
   "rtmidi",
   "soundtouch",
   "vorbis",
  }
  filter { "configurations:*Shared" }
  filter { "not configurations:*Shared" }
   linkoptions {
    "/DELAYLOAD:openmpt-mpg123.dll",
    "/DELAYLOAD:OpenMPT_SoundTouch_f32.dll",
   }
   targetname "mptrack"
  filter {}
  linkoptions {
   "/DELAYLOAD:mf.dll",
   "/DELAYLOAD:mfplat.dll",
   "/DELAYLOAD:mfreadwrite.dll",
--   "/DELAYLOAD:mfuuid.dll", -- static library
   "/DELAYLOAD:propsys.dll",
  }
  filter { "action:vs*" }
    files {
      "../../build/vs/debug/openmpt.natvis",
    }
  filter {}
  prebuildcommands { "..\\..\\build\\svn_version\\update_svn_version_vs_premake.cmd $(IntDir)" }

 project "OpenMPT-NativeSupport"
  uuid "563a631d-fe07-47bc-a98f-9fe5b3ebabfa"
  language "C++"
  location ( "../../build/" .. mpt_projectpathname )
  vpaths { ["*"] = "../../" }
  mpt_projectname = "OpenMPT-NativeSupport"
  dofile "../../build/premake/premake-defaults-DLL.lua"
  dofile "../../build/premake/premake-defaults.lua"
  includedirs {
   "../../common",
   "../../include",
   "../../include/msinttypes/inttypes",
   "../../include/ASIOSDK2/common",
   "../../include/portaudio/include",
   "$(IntDir)/svn_version",
   "../../build/svn_version",
  }
  files {
   "../../common/*.cpp",
   "../../common/*.h",
   "../../soundbase/*.cpp",
   "../../soundbase/*.h",
   "../../sounddev/*.cpp",
   "../../sounddev/*.h",
   "../../mptrack/wine/*.cpp",
   "../../mptrack/wine/*.h",
  }
  excludes {
   "../../mptrack/wine/WineWrapper.cpp",
  }
  defines { "MODPLUG_TRACKER", "MPT_BUILD_WINESUPPORT" }
  largeaddressaware ( true )
  characterset "Unicode"
  flags { "ExtraWarnings" }
  links {
   "portaudio",
  }
  filter {}
  prebuildcommands { "..\\..\\build\\svn_version\\update_svn_version_vs_premake.cmd $(IntDir)" }

 project "OpenMPT-WineWrapper"
  uuid "f3da2bf5-e84a-4f71-80ab-884594863d3a"
  language "C"
  location ( "../../build/" .. mpt_projectpathname )
  vpaths { ["*"] = "../../" }
  mpt_projectname = "OpenMPT-WineWrapper"
  dofile "../../build/premake/premake-defaults-DLL.lua"
  dofile "../../build/premake/premake-defaults.lua"
  includedirs {
   "../../common",
   "../../include",
   "../../include/msinttypes/inttypes",
   "$(IntDir)/svn_version",
   "../../build/svn_version",
  }
  files {
   "../../mptrack/wine/WineWrapper.c",
  }
  defines { "MODPLUG_TRACKER", "MPT_BUILD_WINESUPPORT_WRAPPER" }
  largeaddressaware ( true )
  characterset "Unicode"
  flags { "ExtraWarnings" }
  links {
   "OpenMPT-NativeSupport",
  }
  filter {}
  postbuildcommands { "..\\..\\build\\wine\\build_wine_support.cmd $(IntDir) $(OutDir)" }
