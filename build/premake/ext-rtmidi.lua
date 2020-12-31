 
 project "rtmidi"
  uuid "05BBD03D-0985-4D76-8DDD-534DA631C3A8"
  language "C++"
  location ( "../../build/" .. mpt_projectpathname .. "/ext" )
  mpt_projectname = "rtmidi"
  dofile "../../build/premake/premake-defaults-LIB.lua"
  dofile "../../build/premake/premake-defaults.lua"
  targetname "openmpt-rtmidi"
	filter {}
	filter { "action:vs*" }
		characterset "Unicode"
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
