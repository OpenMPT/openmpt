
include_dependency "ext-ogg.lua"
include_dependency "ext-opus.lua"

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
	filter {}
	if MPT_COMPILER_MSVC or MPT_COMPILER_CLANGCL then
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
		buildoptions {
			"/wd6262",
		} -- analyze
	end
	filter {}
	if MPT_OS_WINDOWS then
		filter {}
		filter { "kind:StaticLib" }
			defines { }
		filter { "kind:SharedLib" }
			defines { "DLL_EXPORT" }
		filter {}
	end
	filter {}

function mpt_use_opusenc ()
	filter {}
	dependencyincludedirs {
		"../../include/opusenc/include",
	}
	filter {}
	links {
		"opusenc",
	}
	filter {}
end
