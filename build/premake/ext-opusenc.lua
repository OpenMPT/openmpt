  
 project "opusenc"
  uuid "290bbf89-2572-4291-9d9c-ff021d4fd313"
  language "C"
  location ( "../../build/" .. mpt_projectpathname .. "/ext" )
  mpt_projectname = "opusenc"
  dofile "../../build/premake/premake-defaults-LIBorDLL.lua"
  dofile "../../build/premake/premake-defaults.lua"
  dofile "../../build/premake/premake-defaults-winver.lua"
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
  }
	defines { "OPE_BUILD", "FLOATING_POINT" }
	defines { "PACKAGE_NAME=\"libopusenc\"", "PACKAGE_VERSION=\"0.1\"" }
	defines { "OUTSIDE_SPEEX", "SPX_RESAMPLE_EXPORT=", "RANDOM_PREFIX=libopusenc" }
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
  filter { "kind:StaticLib" }
   defines { }
  filter { "kind:SharedLib" }
   defines { "DLL_EXPORT" }
  filter {}
