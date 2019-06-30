	filter {}
		if _OPTIONS["win10"] then
			standardconformance "On"
		end
	filter {}
	filter { "action:vs*", "language:C++" }
		cppdialect "C++17"
	filter {}
