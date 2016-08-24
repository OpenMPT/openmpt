
if layout == "custom" then
 project "OpenMPT-custom"
	uuid "6b9af880-af37-4268-bb91-2b982ff6499a"
else
 project "OpenMPT"
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
  mpt_projectname = "OpenMPT"
  dofile "../../build/premake/premake-defaults-EXEGUI.lua"
  dofile "../../build/premake/premake-defaults.lua"
  filter { "configurations:*Shared" }
   targetname "OpenMPT"
  filter { "not configurations:*Shared" }
   targetname "mptrack"
  filter { "action:vs2008" }
   includedirs { "../../include/msinttypes/inttypes" }
  filter { "action:vs2010" }
   includedirs { "../../include/msinttypes/inttypes" }
  filter { "action:vs2012" }
   includedirs { "../../include/msinttypes/inttypes" }
  filter {}
  local extincludedirs = {
   "../../include",
   "../../include/vstsdk2.4",
   "../../include/ASIOSDK2/common",
   "../../include/flac/include",
   "../../include/lhasa/lib/public",
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
  excludes {
    "../../mptrack/res/rt_manif.bin", -- the old build system manifest
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
   "ogg",
   "opus",
   "opusfile",
   "portaudio",
   "portmidi",
   "r8brain",
   "soundtouch",
   "vorbis",
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
  filter { "action:vs2015*" }
    files {
      "../../build/vs/debug/openmpt.natvis",
    }
  filter {}
  prebuildcommands { "..\\..\\build\\svn_version\\update_svn_version_vs_premake.cmd $(IntDir)" }
