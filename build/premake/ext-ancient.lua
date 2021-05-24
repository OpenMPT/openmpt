
project "ancient"
	uuid "e1184509-74f7-421d-a8c8-feec2c28ecc2"
	language "C++"
	location ( "../../build/" .. mpt_projectpathname .. "/ext" )
	mpt_projectname = "ancient"
	dofile "../../build/premake/premake-defaults-LIBorDLL.lua"
	dofile "../../build/premake/premake-defaults.lua"
	targetname "openmpt-ancient"
	includedirs {
		"../../include/ancient/api",
		"../../include/ancient/api/ancient",
		"../../include/ancient/src",
	}
	filter {}
	filter { "action:vs*" }
		characterset "Unicode"
	filter {}
	files {
		"../../include/ancient/api/ancient/**.hpp",
	}
	files {
		"../../include/ancient/src/**.hpp",
		"../../include/ancient/src/**.cpp",
	}
	filter { "action:vs*" }
		buildoptions {
			"/wd4146",
			"/wd4244",
		}
		buildoptions {
			"/wd4251",
			"/wd4275",
		}
	filter {}
	filter { "kind:SharedLib" }
		defines { "ANCIENT_API_DECLSPEC_DLLEXPORT" }
	filter {}
