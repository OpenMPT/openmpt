 
 project "rtmidi"
  uuid "05BBD03D-0985-4D76-8DDD-534DA631C3A8"
  language "C++"
  location ( "%{wks.location}" .. "/ext" )
  mpt_kind "static"
  targetname "openmpt-rtmidi"
	filter {}
  files {
   "../../include/rtmidi/RtMidi.cpp"
  }
  files {
   "../../include/rtmidi/RtMidi.h"
  }
  defines {
   "__WINDOWS_MM__",
   "RTMIDI_DO_NOT_ENSURE_UNIQUE_PORTNAMES"
  }
	if _OPTIONS["clang"] then
		filter { "not kind:StaticLib" }
			links { "winmm" }
		filter {}
	else
		filter {}
			links { "winmm" }
		filter {}
	end

function mpt_use_rtmidi ()
	filter {}
	dependencyincludedirs {
		"../../include",
	}
	filter {}
	links {
		"rtmidi",
	}
	filter {}
end
