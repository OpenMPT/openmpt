
  filter {}
	
	objdir ( "../../build/obj/" .. mpt_projectpathname .. "/" .. mpt_projectname )

  filter {}
	filter { "not action:vs*", "language:C++" }
		buildoptions { "-std=c++11" }
	filter { "not action:vs*", "language:C" }
		buildoptions { "-std=c99" }
	filter {}

	filter {}
		if _OPTIONS["xp"] then
			if _ACTION == "vs2015" then
				toolset "v140_xp"
			elseif _ACTION == "vs2017" then
				toolset "v141_xp"
			end
			defines { "MPT_BUILD_TARGET_XP" }
			filter { "action:vs*" }
				buildoptions { "/Zc:threadSafeInit-" }
			filter {}
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
	 
  filter { "kind:StaticLib", "configurations:Debug", "architecture:ARM" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/arm/Debug" )
  filter { "kind:StaticLib", "configurations:DebugShared", "architecture:ARM" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/arm/DebugShared" )
  filter { "kind:StaticLib", "configurations:DebugMDd", "architecture:ARM" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/arm/DebugMDd" )
  filter { "kind:StaticLib", "configurations:Release", "architecture:ARM" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/arm/Release" )
  filter { "kind:StaticLib", "configurations:ReleaseShared", "architecture:ARM" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/arm/ReleaseShared" )
  filter { "kind:StaticLib", "configurations:ReleaseLTCG", "architecture:ARM" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/arm/ReleaseLTCG" )
  filter { "kind:StaticLib", "configurations:Debug", "architecture:ARM64" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/arm64/Debug" )
  filter { "kind:StaticLib", "configurations:DebugShared", "architecture:ARM64" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/arm64/DebugShared" )
  filter { "kind:StaticLib", "configurations:DebugMDd", "architecture:ARM64" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/arm64/DebugMDd" )
  filter { "kind:StaticLib", "configurations:Release", "architecture:ARM64" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/arm64/Release" )
  filter { "kind:StaticLib", "configurations:ReleaseShared", "architecture:ARM64" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/arm64/ReleaseShared" )
  filter { "kind:StaticLib", "configurations:ReleaseLTCG", "architecture:ARM64" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/arm64/ReleaseLTCG" )
  	
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
		
  filter { "kind:not StaticLib", "configurations:Debug", "architecture:ARM" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-static/arm-32-" .. mpt_bindirsuffix32 )
  filter { "kind:not StaticLib", "configurations:DebugShared", "architecture:ARM" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-shared/arm-32-" .. mpt_bindirsuffix32 )
  filter { "kind:not StaticLib", "configurations:DebugMDd", "architecture:ARM" }
		targetdir ( "../../bin/debug-MDd/" .. _ACTION .. "-static/arm-32-" .. mpt_bindirsuffix32 )
  filter { "kind:not StaticLib", "configurations:Release", "architecture:ARM" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-static/arm-32-" .. mpt_bindirsuffix32 )
  filter { "kind:not StaticLib", "configurations:ReleaseShared", "architecture:ARM" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-shared/arm-32-" .. mpt_bindirsuffix32 )
  filter { "kind:not StaticLib", "configurations:ReleaseLTCG", "architecture:ARM" }
		targetdir ( "../../bin/release-LTCG/" .. _ACTION .. "-static/arm-32-" .. mpt_bindirsuffix32 )
  filter { "kind:not StaticLib", "configurations:Debug", "architecture:ARM64" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-static/arm-64-" .. mpt_bindirsuffix64 )
  filter { "kind:not StaticLib", "configurations:DebugShared", "architecture:ARM64" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-shared/arm-64-" .. mpt_bindirsuffix64 )
  filter { "kind:not StaticLib", "configurations:DebugMDd", "architecture:ARM64" }
		targetdir ( "../../bin/debug-MDd/" .. _ACTION .. "-static/arm-64-" .. mpt_bindirsuffix64 )
  filter { "kind:not StaticLib", "configurations:Release", "architecture:ARM64" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-static/arm-64-" .. mpt_bindirsuffix64 )
  filter { "kind:not StaticLib", "configurations:ReleaseShared", "architecture:ARM64" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-shared/arm-64-" .. mpt_bindirsuffix64 )
  filter { "kind:not StaticLib", "configurations:ReleaseLTCG", "architecture:ARM64" }
		targetdir ( "../../bin/release-LTCG/" .. _ACTION .. "-static/arm-64-" .. mpt_bindirsuffix64 )

  filter { "configurations:Debug" }
   defines { "DEBUG" }
   defines { "MPT_BUILD_MSVC_STATIC" }
   symbols "On"
   flags { "StaticRuntime" }
	 runtime "Debug"
   optimize "Debug"

  filter { "configurations:DebugShared" }
   defines { "DEBUG" }
   defines { "MPT_BUILD_MSVC_SHARED" }
   flags { "MultiProcessorCompile" }
   symbols "On"
	 runtime "Debug"
   optimize "Debug"

  filter { "configurations:DebugMDd" }
   defines { "DEBUG" }
   defines { "MPT_BUILD_MSVC_STATIC" }
   symbols "FastLink"
	 runtime "Debug"
   optimize "Debug"

  filter { "configurations:Release" }
   defines { "NDEBUG" }
   defines { "MPT_BUILD_MSVC_STATIC" }
   symbols "On"
   flags { "MultiProcessorCompile" }
   flags { "StaticRuntime" }
	 runtime "Release"
   optimize "Speed"
   floatingpoint "Fast"

  filter { "configurations:ReleaseShared" }
   defines { "NDEBUG" }
   defines { "MPT_BUILD_MSVC_SHARED" }
   symbols "On"
   flags { "MultiProcessorCompile" }
	 runtime "Release"
   optimize "Speed"
   floatingpoint "Fast"

  filter { "configurations:ReleaseLTCG" }
   defines { "NDEBUG" }
   defines { "MPT_BUILD_MSVC_STATIC" }
   symbols "On"
   flags { "MultiProcessorCompile", "LinkTimeOptimization" }
   flags { "StaticRuntime" }
	 runtime "Release"
   optimize "Full"
   floatingpoint "Fast"

	filter {}

	if _OPTIONS["xp"] then

		filter { "architecture:x86" }
			vectorextensions "IA32"
		filter {}

	else

		filter {}

		filter { "architecture:x86", "configurations:Release" }
			vectorextensions "SSE2"

		filter { "architecture:x86", "configurations:ReleaseShared" }
			vectorextensions "SSE2"

		filter { "architecture:x86", "configurations:ReleaseLTCG" }
			vectorextensions "SSE2"

		filter {}
	
	end

  filter {}
	defines { "MPT_BUILD_MSVC" }

  filter {}
   defines { "WIN32", "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_DEPRECATE", "_CRT_SECURE_NO_DEPRECATE", "_CRT_NONSTDC_NO_WARNINGS" }

  filter {}
