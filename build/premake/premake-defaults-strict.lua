	filter {}
		if _OPTIONS["win10"] then
			buildoptions { "/permissive-" }
		end
	filter {}
	filter { "action:vs*", "language:C++", "not action:vs2015" }
		cppdialect "C++17"
	filter {}
