  
 project "rtaudio"
  uuid "4886456b-1342-4ec8-ad3f-d92aeb8c1097"
  language "C++"
  location ( "../../build/" .. mpt_projectpathname .. "/ext" )
  mpt_projectname = "rtaudio"
  dofile "../../build/premake/premake-defaults-LIB.lua"
  dofile "../../build/premake/premake-defaults.lua"
  targetname "openmpt-rtaudio"
	filter {}
	filter { "action:vs*" }
		characterset "Unicode"
	filter {}
	filter { "action:vs2017" }
		if _OPTIONS["winxp"] then
			defines {
				"__WINDOWS_DS__",
			}
		else
			defines {
				-- WASAPI causes link failure due to confused SDK headers
			}
		end
	filter { "not action:vs2017" }
		if _OPTIONS["winxp"] then
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
	if _OPTIONS["winxp"] then
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
    buildoptions { "/wd4267" }
  filter {}
	filter { "action:vs*" }
		buildoptions { "/wd6031" } -- analyze
	filter {}
