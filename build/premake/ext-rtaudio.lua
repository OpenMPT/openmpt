  
 project "rtaudio"
  uuid "4886456b-1342-4ec8-ad3f-d92aeb8c1097"
  language "C++"
  location ( "../../build/" .. mpt_projectpathname .. "/ext" )
  mpt_projectname = "rtaudio"
  dofile "../../build/premake/premake-defaults-LIB.lua"
  dofile "../../build/premake/premake-defaults.lua"
	dofile "../../build/premake/premake-defaults-strict.lua"
  dofile "../../build/premake/premake-defaults-winver.lua"
  targetname "openmpt-rtaudio"
	filter {}
	filter { "action:vs*" }
		characterset "Unicode"
	filter {}
  defines {
   "__WINDOWS_DS__",
--   "__WINDOWS_WASAPI__",
  }
  files {
   "../../include/rtaudio/RtAudio.cpp",
   "../../include/rtaudio/RtAudio.h",
  }
	links {
		"dsound",
	}
