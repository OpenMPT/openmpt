
  configuration {}

	configuration { "Debug", "x32" }
		targetdir ( "../../bin/debug/" .. _OPTIONS["target"] .. "-static/x86-32" )
	configuration { "DebugShared", "x32" }
		targetdir ( "../../bin/debug/" .. _OPTIONS["target"] .. "-shared/x86-32" )
	configuration { "Release", "x32" }
		targetdir ( "../../bin/release/" .. _OPTIONS["target"] .. "-static/x86-32" )
	configuration { "ReleaseShared", "x32" }
		targetdir ( "../../bin/release/" .. _OPTIONS["target"] .. "-shared/x86-32" )

	configuration { "Debug", "x64" }
		targetdir ( "../../bin/debug/" .. _OPTIONS["target"] .. "-static/x86-64" )
	configuration { "DebugShared", "x64" }
		targetdir ( "../../bin/debug/" .. _OPTIONS["target"] .. "-shared/x86-64" )
	configuration { "Release", "x64" }
		targetdir ( "../../bin/release/" .. _OPTIONS["target"] .. "-static/x86-64" )
	configuration { "ReleaseShared", "x64" }
		targetdir ( "../../bin/release/" .. _OPTIONS["target"] .. "-shared/x86-64" )

	configuration { "Debug", "ARM" }
		targetdir ( "../../bin/debug/" .. _OPTIONS["target"] .. "-static/arm" )
	configuration { "DebugShared", "ARM" }
		targetdir ( "../../bin/debug/" .. _OPTIONS["target"] .. "-shared/arm" )
	configuration { "Release", "ARM" }
		targetdir ( "../../bin/release/" .. _OPTIONS["target"] .. "-static/arm" )
	configuration { "ReleaseShared", "ARM" }
		targetdir ( "../../bin/release/" .. _OPTIONS["target"] .. "-shared/arm" )
  
  configuration "Debug"
   defines { "DEBUG" }
   defines { "MPT_BUILD_MSVC_STATIC" }
   flags { "Symbols" }

  configuration "DebugShared"
   defines { "DEBUG" }
   defines { "MPT_BUILD_MSVC_SHARED" }
   flags { "Symbols" }

  configuration "Release"
   defines { "NDEBUG" }
   defines { "MPT_BUILD_MSVC_STATIC" }
   flags { "Symbols" }
   flags { "OptimizeSpeed" }
   flags { "FloatFast" }

  configuration "ReleaseShared"
   defines { "NDEBUG" }
   defines { "MPT_BUILD_MSVC_SHARED" }
   flags { "Symbols" }
   flags { "OptimizeSpeed" }
   flags { "FloatFast" }

  configuration {}
   defines { "WIN32", "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_DEPRECATE", "_CRT_SECURE_NO_DEPRECATE", "_CRT_NONSTDC_NO_WARNINGS" }

  configuration {}
