  
 project "opusenc"
  uuid "290bbf89-2572-4291-9d9c-ff021d4fd313"
  language "C"
  location ( "%{wks.location}" .. "/ext" )
  mpt_kind "default"
  targetname "openmpt-opusenc"
	
	mpt_use_ogg()
	mpt_use_opus()
	
  includedirs {
   "../../include/opusenc/include",
   "../../include/opusenc/win32",
  }
	filter {}
  files {
   "../../include/opusenc/include/opusenc.h",
  }
  files {
   "../../include/opusenc/src/*.c",
   "../../include/opusenc/src/*.h",
   "../../include/opusenc/win32/*.c",
   "../../include/opusenc/win32/*.h",
  }
	defines { "HAVE_CONFIG_H" }
	defines { "OUTSIDE_SPEEX", "RANDOM_PREFIX=libopusenc" }
  filter { "action:vs*" }
    buildoptions {
			"/wd4018",
			"/wd4100",
			"/wd4101",
			"/wd4127",
			"/wd4244",
			"/wd4267",
			"/wd4456",
			"/wd4706",
		}
  filter {}
	filter { "action:vs*" }
		buildoptions {
			"/wd6262",
		} -- analyze
	filter {}
  filter { "kind:StaticLib" }
   defines { }
  filter { "kind:SharedLib" }
   defines { "DLL_EXPORT" }
  filter {}

function mpt_use_opusenc ()
	filter {}
	filter { "action:vs*" }
		includedirs {
			"../../include/opusenc/include",
		}
	filter { "not action:vs*" }
		externalincludedirs {
			"../../include/opusenc/include",
		}
	filter {}
	links {
		"opusenc",
	}
	filter {}
end
