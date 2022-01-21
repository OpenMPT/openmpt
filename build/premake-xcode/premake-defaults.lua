
	filter {}
		cppdialect "C++17"

	filter { "configurations:Debug" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-" .. _OPTIONS["target"] .. "/all" )
	filter {}

	filter { "configurations:Release" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-" .. _OPTIONS["target"] .. "/all" )
	filter {}

	filter { "configurations:Debug" }
		defines { "DEBUG" }
		defines { "MPT_BUILD_DEBUG" }
		symbols "On"
	filter {}

	filter { "configurations:Release" }
		defines { "NDEBUG" }
		optimize "Speed"
	filter {}

	filter {}
		defines { "MPT_BUILD_XCODE" }
		system = _OPTIONS["target"]

	filter {}
