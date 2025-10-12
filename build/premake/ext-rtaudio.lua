  
 project "rtaudio"
  uuid "4886456b-1342-4ec8-ad3f-d92aeb8c1097"
  language "C++"
  location ( "%{wks.location}" .. "/ext" )
  mpt_kind "static"
  targetname "openmpt-rtaudio"
	filter {}
	if MPT_WIN_BEFORE(MPT_WIN["7"]) then
		defines {
			"__WINDOWS_DS__",
		}
	end
	filter {}
	if not MPT_MSVC_BEFORE(2019) then
		defines {
			-- WASAPI causes link failure due to confused SDK headers
			"__WINDOWS_WASAPI__",
		}
	end
	filter {}
  files {
   "../../include/rtaudio/RtAudio.cpp",
   "../../include/rtaudio/RtAudio.h",
  }
	if MPT_WIN_BEFORE(MPT_WIN["7"]) then
		if MPT_COMPILER_CLANGCL or MPT_COMPILER_CLANG then
			filter { "not kind:StaticLib" }
				links { "dsound" }
			filter {}
		else
			filter {}
				links { "dsound" }
			filter {}
		end
	end
	filter {}
	if MPT_COMPILER_MSVC or MPT_COMPILER_CLANGCL then
		defines { " _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING" }
		buildoptions { "/wd4267" }
		buildoptions { "/wd6031" } -- analyze
	end
	filter {}

function mpt_use_rtaudio ()
	filter {}
	dependencyincludedirs {
		"../../include/rtaudio",
	}
	filter {}
	links {
		"rtaudio",
	}
	filter {}
end
