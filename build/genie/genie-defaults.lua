
  configuration {}

	objdir ( "../../build/obj/" .. mpt_projectpathname .. "/" .. mpt_projectname )
	
	flags { "Cpp17" }

	configuration { "Debug", "x32" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-static/x86" )
	configuration { "DebugShared", "x32" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-shared/x86" )
	configuration { "Release", "x32" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-static/x86" )
	configuration { "ReleaseShared", "x32" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-shared/x86" )

	configuration { "Debug", "x64" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-static/amd64" )
	configuration { "DebugShared", "x64" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-shared/amd64" )
	configuration { "Release", "x64" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-static/amd64" )
	configuration { "ReleaseShared", "x64" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-shared/amd64" )

	configuration { "Debug", "ARM" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-static/arm" )
	configuration { "DebugShared", "ARM" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-shared/arm" )
	configuration { "Release", "ARM" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-static/arm" )
	configuration { "ReleaseShared", "ARM" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-shared/arm" )
  
	configuration { "Debug", "ARM64" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-static/arm64" )
	configuration { "DebugShared", "ARM64" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-shared/arm64" )
	configuration { "Release", "ARM64" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-static/arm64" )
	configuration { "ReleaseShared", "ARM64" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-shared/arm64" )
  
  configuration "Debug"
   defines { "DEBUG" }
   defines { "MPT_BUILD_DEBUG" }
   defines { "MPT_BUILD_MSVC_STATIC" }
   flags { "Symbols" }

  configuration "DebugShared"
   defines { "DEBUG" }
   defines { "MPT_BUILD_DEBUG" }
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
   defines { "MPT_BUILD_MSVC" }
   
  configuration {}
   defines { "WIN32", "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_DEPRECATE", "_CRT_SECURE_NO_DEPRECATE", "_CRT_NONSTDC_NO_WARNINGS" }

  configuration {}
