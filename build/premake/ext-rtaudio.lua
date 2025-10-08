  
 project "rtaudio"
  uuid "4886456b-1342-4ec8-ad3f-d92aeb8c1097"
  language "C++"
  location ( "%{wks.location}" .. "/ext" )
  mpt_kind "static"
  targetname "openmpt-rtaudio"
	filter {}
	filter { "action:vs2017" }
		if _OPTIONS["windows-version"] == "winxp" or _OPTIONS["windows-version"] == "winxpx64" then
			defines {
				"__WINDOWS_DS__",
			}
		else
			defines {
				-- WASAPI causes link failure due to confused SDK headers
			}
		end
	filter { "not action:vs2017" }
		if _OPTIONS["windows-version"] == "winxp" or _OPTIONS["windows-version"] == "winxpx64" then
			defines {
				"__WINDOWS_DS__",
			}
		else
			defines {
				"__WINDOWS_WASAPI__",
			}
		end
	filter {}
  files {
   "../../include/rtaudio/RtAudio.cpp",
   "../../include/rtaudio/RtAudio.h",
  }
	if _OPTIONS["windows-version"] == "winxp" or _OPTIONS["windows-version"] == "winxpx64" then
		if _OPTIONS["clang"] then
			filter { "not kind:StaticLib" }
				links { "dsound" }
			filter {}
		else
			filter {}
				links { "dsound" }
			filter {}
		end
	end
  filter { }
  filter { "action:vs*" }
		defines { " _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING" }
    buildoptions { "/wd4267" }
  filter {}
	filter { "action:vs*" }
		buildoptions { "/wd6031" } -- analyze
	filter {}

function mpt_use_rtaudio ()
	filter {}
	filter { "action:vs*" }
		includedirs {
			"../../include/rtaudio",
		}
	filter { "not action:vs*" }
		externalincludedirs {
			"../../include/rtaudio",
		}
	filter {}
	links {
		"rtaudio",
	}
	filter {}
end
