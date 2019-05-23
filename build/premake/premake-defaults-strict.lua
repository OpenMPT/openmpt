	filter {}
		if _OPTIONS["win10"] then
			buildoptions { "/permissive-" }
		end
	filter {}
	filter { "action:vs*", "language:C++" }
		cppdialect "C++17"
	filter {}
