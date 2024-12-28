
function mpt_use_nlohmannjson ()
	filter {}
	includedirs {
		"../../include/nlohmann-json/include",
	}
	files {
		"../../include/nlohmann-json/include/**.hpp",
	}
	filter {}
end
