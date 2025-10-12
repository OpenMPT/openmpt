
function mpt_use_nlohmannjson ()
	filter {}
	dependencyincludedirs {
		"../../include/nlohmann-json/include",
	}
	files {
		"../../include/nlohmann-json/include/**.hpp",
	}
	defines {
		"JSON_USE_IMPLICIT_CONVERSIONS=0",
	}
	filter {}
end
