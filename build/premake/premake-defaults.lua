
  filter {}
	
	objdir ( "../../build/obj/" .. mpt_projectpathname .. "/" .. mpt_projectname )

  filter {}
	filter { "not action:vs*", "language:C++" }
		buildoptions { "-std=c++11" }
	filter { "not action:vs*", "language:C" }
		buildoptions { "-std=c99" }
	filter {}

	filter {}
		if _OPTIONS["w2k"] then
			if _ACTION == "vs2012" then
				toolset "v110_xp"
			elseif _ACTION == "vs2013" then
				toolset "v120_xp"
			elseif _ACTION == "vs2015" then
				toolset "v140_xp"
			end
			defines { "MPT_BUILD_TARGET_2000" }
		elseif _OPTIONS["xp"] then
			if _ACTION == "vs2012" then
				toolset "v110_xp"
			elseif _ACTION == "vs2013" then
				toolset "v120_xp"
			elseif _ACTION == "vs2015" then
				toolset "v140_xp"
			end
			defines { "MPT_BUILD_TARGET_XP" }
		end

  filter { "kind:StaticLib", "configurations:Debug", "architecture:x86" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/x86/Debug" )
  filter { "kind:StaticLib", "configurations:DebugShared", "architecture:x86" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/x86/DebugShared" )
  filter { "kind:StaticLib", "configurations:DebugMDd", "architecture:x86" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/x86/DebugMDd" )
  filter { "kind:StaticLib", "configurations:Release", "architecture:x86" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/x86/Release" )
  filter { "kind:StaticLib", "configurations:ReleaseShared", "architecture:x86" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/x86/ReleaseShared" )
  filter { "kind:StaticLib", "configurations:ReleaseLTCG", "architecture:x86" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/x86/ReleaseLTCG" )
  filter { "kind:StaticLib", "configurations:Debug", "architecture:x86_64" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/x86_64/Debug" )
  filter { "kind:StaticLib", "configurations:DebugShared", "architecture:x86_64" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/x86_64/DebugShared" )
  filter { "kind:StaticLib", "configurations:DebugMDd", "architecture:x86_64" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/x86_64/DebugMDd" )
  filter { "kind:StaticLib", "configurations:Release", "architecture:x86_64" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/x86_64/Release" )
  filter { "kind:StaticLib", "configurations:ReleaseShared", "architecture:x86_64" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/x86_64/ReleaseShared" )
  filter { "kind:StaticLib", "configurations:ReleaseLTCG", "architecture:x86_64" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/x86_64/ReleaseLTCG" )
  	
  filter { "kind:not StaticLib", "configurations:Debug", "architecture:x86" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-static/x86-32-" .. mpt_bindirsuffix32 )
  filter { "kind:not StaticLib", "configurations:DebugShared", "architecture:x86" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-shared/x86-32-" .. mpt_bindirsuffix32 )
  filter { "kind:not StaticLib", "configurations:DebugMDd", "architecture:x86" }
		targetdir ( "../../bin/debug-MDd/" .. _ACTION .. "-static/x86-32-" .. mpt_bindirsuffix32 )
  filter { "kind:not StaticLib", "configurations:Release", "architecture:x86" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-static/x86-32-" .. mpt_bindirsuffix32 )
  filter { "kind:not StaticLib", "configurations:ReleaseShared", "architecture:x86" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-shared/x86-32-" .. mpt_bindirsuffix32 )
  filter { "kind:not StaticLib", "configurations:ReleaseLTCG", "architecture:x86" }
		targetdir ( "../../bin/release-LTCG/" .. _ACTION .. "-static/x86-32-" .. mpt_bindirsuffix32 )
  filter { "kind:not StaticLib", "configurations:Debug", "architecture:x86_64" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-static/x86-64-" .. mpt_bindirsuffix64 )
  filter { "kind:not StaticLib", "configurations:DebugShared", "architecture:x86_64" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-shared/x86-64-" .. mpt_bindirsuffix64 )
  filter { "kind:not StaticLib", "configurations:DebugMDd", "architecture:x86_64" }
		targetdir ( "../../bin/debug-MDd/" .. _ACTION .. "-static/x86-64-" .. mpt_bindirsuffix64 )
  filter { "kind:not StaticLib", "configurations:Release", "architecture:x86_64" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-static/x86-64-" .. mpt_bindirsuffix64 )
  filter { "kind:not StaticLib", "configurations:ReleaseShared", "architecture:x86_64" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-shared/x86-64-" .. mpt_bindirsuffix64 )
  filter { "kind:not StaticLib", "configurations:ReleaseLTCG", "architecture:x86_64" }
		targetdir ( "../../bin/release-LTCG/" .. _ACTION .. "-static/x86-64-" .. mpt_bindirsuffix64 )

  filter { "action:vs2008" }
   includedirs { "../../include/msinttypes/stdint" }

  filter { "configurations:Debug" }
   defines { "DEBUG" }
   defines { "MPT_BUILD_MSVC_STATIC" }
   flags { "Symbols" }
   flags { "StaticRuntime" }
   optimize "Debug"

  filter { "configurations:DebugShared" }
   defines { "DEBUG" }
   defines { "MPT_BUILD_MSVC_SHARED" }
   flags { "Symbols" }
   optimize "Debug"

  filter { "configurations:DebugMDd" }
   defines { "DEBUG" }
   defines { "MPT_BUILD_MSVC_STATIC" }
   flags { "Symbols" }
   optimize "Debug"

  filter { "configurations:Release" }
   defines { "NDEBUG" }
   defines { "MPT_BUILD_MSVC_STATIC" }
   flags { "Symbols", "MultiProcessorCompile" }
   flags { "StaticRuntime" }
   optimize "Speed"
   floatingpoint "Fast"

  filter { "configurations:ReleaseShared" }
   defines { "NDEBUG" }
   defines { "MPT_BUILD_MSVC_SHARED" }
   flags { "Symbols", "MultiProcessorCompile" }
   optimize "Speed"
   floatingpoint "Fast"

  filter { "configurations:ReleaseLTCG" }
   defines { "NDEBUG" }
   defines { "MPT_BUILD_MSVC_STATIC" }
   flags { "Symbols", "MultiProcessorCompile", "LinkTimeOptimization" }
   flags { "StaticRuntime" }
   optimize "Full"
   floatingpoint "Fast"

	filter {}

	if _OPTIONS["w2k"] or _OPTIONS["xp"] then

		-- https://github.com/premake/premake-core/issues/560
	
		filter {}

		filter { "configurations:Release", "action:vs2012", "architecture:x86" }
			buildoptions { "/arch:IA32" }

		filter { "configurations:ReleaseShared", "action:vs2012", "architecture:x86" }
			buildoptions { "/arch:IA32" }

		filter { "configurations:ReleaseLTCG", "action:vs2012", "architecture:x86" }
			buildoptions { "/arch:IA32" }

		filter { "configurations:Release", "action:vs2013", "architecture:x86" }
			buildoptions { "/arch:IA32" }

		filter { "configurations:ReleaseShared", "action:vs2013", "architecture:x86" }
			buildoptions { "/arch:IA32" }

		filter { "configurations:ReleaseLTCG", "action:vs2013", "architecture:x86" }
			buildoptions { "/arch:IA32" }

		filter { "configurations:Release", "action:vs2015", "architecture:x86" }
			buildoptions { "/arch:IA32" }

		filter { "configurations:ReleaseShared", "action:vs2015", "architecture:x86" }
			buildoptions { "/arch:IA32" }

		filter { "configurations:ReleaseLTCG", "action:vs2015", "architecture:x86" }
			buildoptions { "/arch:IA32" }

		filter {}

	else

		filter {}

		filter { "configurations:Release", "not action:vs2008" }
			vectorextensions "SSE2"

		filter { "configurations:ReleaseShared", "not action:vs2008" }
		 vectorextensions "SSE2"

		filter { "configurations:ReleaseLTCG", "not action:vs2008" }
		 vectorextensions "SSE2"

		filter {}
	
	end

  filter {}
   defines { "WIN32", "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_DEPRECATE", "_CRT_SECURE_NO_DEPRECATE", "_CRT_NONSTDC_NO_WARNINGS" }

  filter {}
