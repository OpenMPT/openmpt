	filter {}
	filter { "action:vs*", "action:vs2017" }
		if _OPTIONS["win10"] then
			standardconformance "On"
		end
	filter { "action:vs*", "not action:vs2017" }
		standardconformance "On"
	filter {}
	filter { "action:vs*", "language:C++" }
		cppdialect "C++17"
	filter {}
