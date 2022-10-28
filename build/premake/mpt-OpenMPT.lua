
if charset == "Unicode" then
if stringmode == "WCHAR" then
  project "OpenMPT"
	uuid "37FC32A4-8DDC-4A9C-A30C-62989DD8ACE9"
else
  project "OpenMPT-UTF8"
	uuid "e89507fa-a251-457e-9957-f6b453c77daf"
end
else
	project "OpenMPT-ANSI"
	uuid "ba66db50-e2f0-4c9e-b650-0cca6c66e1c1"
end
  language "C++"
  vpaths { ["*"] = "../../" }
  mpt_kind "GUI"
if stringmode == "UTF8" then
   targetname "OpenMPT-UTF8"
elseif charset == "MBCS" then
   targetname "OpenMPT-ANSI"
else
   targetname "OpenMPT"
end
  filter {}
	
	mpt_use_ancient()
	if _OPTIONS["windows-version"] ~= "winxp" then
		mpt_use_asiomodern()
	end
	mpt_use_flac()
	mpt_use_lame()
	mpt_use_lhasa()
	mpt_use_minizip()
	mpt_use_mpg123()
	mpt_use_nlohmannjson()
	mpt_use_ogg()
	mpt_use_opus()
	mpt_use_opusenc()
	mpt_use_opusfile()
	mpt_use_portaudio()
	mpt_use_r8brain()
	mpt_use_rtaudio()
	mpt_use_rtmidi()
	mpt_use_smbpitchshift()
	mpt_use_soundtouch()
	mpt_use_unrar()
	mpt_use_vorbis()
	mpt_use_zlib()
	
  includedirs {
   "../../src",
   "../../common",
   "../../soundlib",
   "$(IntDir)/svn_version",
  }
	files {
		"../../mptrack/res/OpenMPT.manifest",
	}
  files {
   "../../src/mpt/**.cpp",
   "../../src/mpt/**.hpp",
   "../../src/openmpt/**.cpp",
   "../../src/openmpt/**.hpp",
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
   "../../unarchiver/*.cpp",
   "../../unarchiver/*.h",
   "../../misc/*.cpp",
   "../../misc/*.h",
   "../../tracklib/*.cpp",
   "../../tracklib/*.h",
   "../../mptrack/*.cpp",
   "../../mptrack/*.h",
   "../../mptrack/plugins/*.cpp",
   "../../mptrack/plugins/*.h",
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

	defines { "MPT_BUILD_ENABLE_PCH" }
	pchsource "../../build/pch/PCH.cpp"
	pchheader "PCH.h"
	files {
		"../../build/pch/PCH.cpp",
		"../../build/pch/PCH.h"
	}
	includedirs {
		"../../build/pch"
	}
	forceincludes {
		"PCH.h"
	}

  defines { "MODPLUG_TRACKER", "MPT_BUILD_DEFAULT_DEPENDENCIES" }
  dpiawareness "None"
  largeaddressaware ( true )
  characterset(charset)
if charset == "Unicode" then
else
	defines { "NO_WARN_MBCS_MFC_DEPRECATION" }
	defines { "MPT_CHECK_WINDOWS_IGNORE_WARNING_NO_UNICODE" }
end
if stringmode == "UTF8" then
	defines { "MPT_USTRING_MODE_UTF8_FORCE" }
end

	mpt_use_mfc()

  warnings "Extra"
  filter {}
	if _OPTIONS["windows-version"] ~= "winxp" then
  linkoptions {
   "/DELAYLOAD:mf.dll",
   "/DELAYLOAD:mfplat.dll",
   "/DELAYLOAD:mfreadwrite.dll",
--   "/DELAYLOAD:mfuuid.dll", -- static library
   "/DELAYLOAD:propsys.dll",
  }
	end
  filter { "action:vs*" }
    files {
      "../../build/vs/debug/openmpt.natvis",
    }
  filter {}
  prebuildcommands { "..\\..\\build\\svn_version\\update_svn_version_vs_premake.cmd $(IntDir)" }
