
	configuration {}

	configuration { "Debug" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-" .. _OPTIONS["target"] .. "/all" )
	configuration { "Release" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-" .. _OPTIONS["target"] .. "/all" )

	configuration "Debug"
		defines { "DEBUG" }
		defines { "MPT_BUILD_DEBUG" }
		flags { "Symbols" }

	configuration "Release"
		defines { "NDEBUG" }
		flags { "OptimizeSpeed" }
		flags { "FloatFast" }

	configuration {}
		defines { "MPT_BUILD_XCODE" }
		premake.xcode.toolset = _OPTIONS["target"]

	configuration {}
