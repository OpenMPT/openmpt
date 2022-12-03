
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
	
	mpt_use_mfc()
	defines { "MPT_WITH_MFC" }
	if _OPTIONS["windows-version"] == "winxp" then
		defines { "MPT_WITH_DIRECTSOUND" }
	end

	mpt_use_ancient()
	defines { "MPT_WITH_ANCIENT" }
	if _OPTIONS["windows-version"] ~= "winxp" and not _OPTIONS["clang"] then
		-- disabled for VS2017 because of multiple initialization of inline variables
		-- https://developercommunity.visualstudio.com/t/static-inline-variable-gets-destroyed-multiple-tim/297876
		mpt_use_asiomodern()
		defines { "MPT_WITH_ASIO" }
	end
	if _OPTIONS["windows-version"] == "winxp" then
		mpt_use_cryptopp()
		defines { "MPT_WITH_CRYPTOPP" }
	end
	mpt_use_flac()
	defines { "MPT_WITH_FLAC" }
	mpt_use_lame()
	defines { "MPT_WITH_LAME" }
	mpt_use_lhasa()
	defines { "MPT_WITH_LHASA" }
	mpt_use_minizip()
	defines { "MPT_WITH_MINIZIP" }
	mpt_use_mpg123()
	defines { "MPT_WITH_MPG123" }
	mpt_use_nlohmannjson()
	defines { "MPT_WITH_NLOHMANNJSON" }
	mpt_use_ogg()
	defines { "MPT_WITH_OGG" }
	mpt_use_opus()
	defines { "MPT_WITH_OPUS" }
	mpt_use_opusenc()
	defines { "MPT_WITH_OPUSENC" }
	mpt_use_opusfile()
	defines { "MPT_WITH_OPUSFILE" }
	mpt_use_portaudio()
	defines { "MPT_WITH_PORTAUDIO" }
	mpt_use_r8brain()
	defines { "MPT_WITH_R8BRAIN" }
	mpt_use_rtaudio()
	defines { "MPT_WITH_RTAUDIO" }
	mpt_use_rtmidi()
	defines { "MPT_WITH_RTMIDI" }
	mpt_use_smbpitchshift()
	defines { "MPT_WITH_SMBPITCHSHIFT" }
	mpt_use_soundtouch()
	defines { "MPT_WITH_SOUNDTOUCH" }
	mpt_use_unrar()
	defines { "MPT_WITH_UNRAR" }
	mpt_use_vorbis()
	defines { "MPT_WITH_VORBIS", "MPT_WITH_VORBISENC", "MPT_WITH_VORBISFILE" }
	mpt_use_zlib()
	defines { "MPT_WITH_ZLIB" }
	
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

  defines { "MODPLUG_TRACKER" }
  dpiawareness "None"
	if _OPTIONS["charset"] ~= "Unicode" then
		defines { "NO_WARN_MBCS_MFC_DEPRECATION" }
		defines { "MPT_CHECK_WINDOWS_IGNORE_WARNING_NO_UNICODE" }
	else
		characterset(charset)
		if charset == "Unicode" then
		else
			defines { "NO_WARN_MBCS_MFC_DEPRECATION" }
			defines { "MPT_CHECK_WINDOWS_IGNORE_WARNING_NO_UNICODE" }
		end
	end
	if stringmode == "UTF8" then
		defines { "MPT_USTRING_MODE_UTF8_FORCE" }
	end

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
