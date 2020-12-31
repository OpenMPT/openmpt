  
 project "opusenc"
  uuid "290bbf89-2572-4291-9d9c-ff021d4fd313"
  language "C"
  location ( "../../build/" .. mpt_projectpathname .. "/ext" )
  mpt_projectname = "opusenc"
  dofile "../../build/premake/premake-defaults-LIBorDLL.lua"
  dofile "../../build/premake/premake-defaults.lua"
  targetname "openmpt-opusenc"
  local extincludedirs = {
   "../../include/ogg/include",
   "../../include/opus/include",
	}
	filter { "action:vs*" }
		includedirs ( extincludedirs )
	filter { "not action:vs*" }
		sysincludedirs ( extincludedirs )
	filter {}
  includedirs {
   "../../include/opusenc/include",
   "../../include/opusenc/win32",
  }
	filter {}
	filter { "action:vs*" }
		characterset "Unicode"
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
  links { "ogg", "opus" }
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
