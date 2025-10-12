
project "ancient"
	uuid "e1184509-74f7-421d-a8c8-feec2c28ecc2"
	language "C++"
	location ( "%{wks.location}" .. "/ext" )
	mpt_kind "default"
	targetname "openmpt-ancient"
	includedirs {
		"../../include/ancient/api",
		"../../include/ancient/api/ancient",
		"../../include/ancient/src",
	}
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
			"/wd4251",
			"/wd4275",
		}
	filter {}
	filter { "kind:SharedLib" }
		defines { "ANCIENT_API_DECLSPEC_DLLEXPORT" }
	filter {}

function mpt_use_ancient ()
	filter {}
	dependencyincludedirs {
		"../../include/ancient/api",
	}
	filter {}
	filter { "configurations:*Shared" }
		defines { "ANCIENT_API_DECLSPEC_DLLIMPORT" }
	filter { "not configurations:*Shared" }
	filter {}
	links {
		"ancient",
	}
	filter {}
end
