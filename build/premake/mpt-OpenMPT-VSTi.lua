
 project "OpenMPT-VSTi"
  uuid "d1ad4072-c810-4eea-95d0-27fdfa764834"
  language "C++"
  location ( "../../build/" .. mpt_projectpathname )
  vpaths { ["*"] = "../../" }
  mpt_projectname = "OpenMPT-VSTi"
  dofile "../../build/premake/premake-defaults-DLL.lua"
  dofile "../../build/premake/premake-defaults.lua"
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
  defines { "OPENMPT_VST" }
  characterset "MBCS"
  flags { "MFC", "ExtraWarnings", "WinMain" }
  links {
   "UnRAR",
   "zlib",
   "minizip",
   "smbPitchShift",
   "lhasa",
   "mpg123",
   "flac",
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
  filter "action:vs*"
    files {
      "../../build/vs/debug/openmpt.natvis",
    }
  filter {}
  prebuildcommands { "..\\..\\build\\svn_version\\update_svn_version_vs_premake.cmd $(IntDir)" }
