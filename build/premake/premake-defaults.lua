
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

	filter { "not action:vs2015", "action:vs*" }
		buildoptions { "/Qspectre" }
	filter {}

  filter { "kind:StaticLib", "configurations:Debug", "architecture:x86" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/x86/Debug" )
  filter { "kind:StaticLib", "configurations:DebugShared", "architecture:x86" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/x86/DebugShared" )
  filter { "kind:StaticLib", "configurations:Checked", "architecture:x86" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/x86/Checked" )
  filter { "kind:StaticLib", "configurations:CheckedShared", "architecture:x86" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/x86/CheckedShared" )
  filter { "kind:StaticLib", "configurations:Release", "architecture:x86" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/x86/Release" )
  filter { "kind:StaticLib", "configurations:ReleaseShared", "architecture:x86" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/x86/ReleaseShared" )
  filter { "kind:StaticLib", "configurations:Debug", "architecture:x86_64" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/x86_64/Debug" )
  filter { "kind:StaticLib", "configurations:DebugShared", "architecture:x86_64" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/x86_64/DebugShared" )
  filter { "kind:StaticLib", "configurations:Checked", "architecture:x86_64" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/x86_64/Checked" )
  filter { "kind:StaticLib", "configurations:CheckedShared", "architecture:x86_64" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/x86_64/CheckedShared" )
  filter { "kind:StaticLib", "configurations:Release", "architecture:x86_64" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/x86_64/Release" )
  filter { "kind:StaticLib", "configurations:ReleaseShared", "architecture:x86_64" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/x86_64/ReleaseShared" )
	 
  filter { "kind:StaticLib", "configurations:Debug", "architecture:ARM" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/arm/Debug" )
  filter { "kind:StaticLib", "configurations:DebugShared", "architecture:ARM" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/arm/DebugShared" )
  filter { "kind:StaticLib", "configurations:Checked", "architecture:ARM" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/arm/Checked" )
  filter { "kind:StaticLib", "configurations:CheckedShared", "architecture:ARM" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/arm/CheckedShared" )
  filter { "kind:StaticLib", "configurations:Release", "architecture:ARM" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/arm/Release" )
  filter { "kind:StaticLib", "configurations:ReleaseShared", "architecture:ARM" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/arm/ReleaseShared" )
  filter { "kind:StaticLib", "configurations:Debug", "architecture:ARM64" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/arm64/Debug" )
  filter { "kind:StaticLib", "configurations:DebugShared", "architecture:ARM64" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/arm64/DebugShared" )
  filter { "kind:StaticLib", "configurations:Checked", "architecture:ARM64" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/arm64/Checked" )
  filter { "kind:StaticLib", "configurations:CheckedShared", "architecture:ARM64" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/arm64/CheckedShared" )
  filter { "kind:StaticLib", "configurations:Release", "architecture:ARM64" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/arm64/Release" )
  filter { "kind:StaticLib", "configurations:ReleaseShared", "architecture:ARM64" }
   targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/arm64/ReleaseShared" )
  	
  filter { "kind:not StaticLib", "configurations:Debug", "architecture:x86" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-static/x86-32-" .. mpt_bindirsuffix32 )
  filter { "kind:not StaticLib", "configurations:DebugShared", "architecture:x86" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-shared/x86-32-" .. mpt_bindirsuffix32 )
  filter { "kind:not StaticLib", "configurations:Checked", "architecture:x86" }
		targetdir ( "../../bin/checked/" .. _ACTION .. "-static/x86-32-" .. mpt_bindirsuffix32 )
  filter { "kind:not StaticLib", "configurations:CheckedShared", "architecture:x86" }
		targetdir ( "../../bin/checked/" .. _ACTION .. "-shared/x86-32-" .. mpt_bindirsuffix32 )
  filter { "kind:not StaticLib", "configurations:Release", "architecture:x86" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-static/x86-32-" .. mpt_bindirsuffix32 )
  filter { "kind:not StaticLib", "configurations:ReleaseShared", "architecture:x86" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-shared/x86-32-" .. mpt_bindirsuffix32 )
  filter { "kind:not StaticLib", "configurations:Debug", "architecture:x86_64" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-static/x86-64-" .. mpt_bindirsuffix64 )
  filter { "kind:not StaticLib", "configurations:DebugShared", "architecture:x86_64" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-shared/x86-64-" .. mpt_bindirsuffix64 )
  filter { "kind:not StaticLib", "configurations:Checked", "architecture:x86_64" }
		targetdir ( "../../bin/checked/" .. _ACTION .. "-static/x86-64-" .. mpt_bindirsuffix64 )
  filter { "kind:not StaticLib", "configurations:CheckedShared", "architecture:x86_64" }
		targetdir ( "../../bin/checked/" .. _ACTION .. "-shared/x86-64-" .. mpt_bindirsuffix64 )
  filter { "kind:not StaticLib", "configurations:Release", "architecture:x86_64" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-static/x86-64-" .. mpt_bindirsuffix64 )
  filter { "kind:not StaticLib", "configurations:ReleaseShared", "architecture:x86_64" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-shared/x86-64-" .. mpt_bindirsuffix64 )
		
  filter { "kind:not StaticLib", "configurations:Debug", "architecture:ARM" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-static/arm-32-" .. mpt_bindirsuffix32 )
  filter { "kind:not StaticLib", "configurations:DebugShared", "architecture:ARM" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-shared/arm-32-" .. mpt_bindirsuffix32 )
  filter { "kind:not StaticLib", "configurations:Checked", "architecture:ARM" }
		targetdir ( "../../bin/checked/" .. _ACTION .. "-static/arm-32-" .. mpt_bindirsuffix32 )
  filter { "kind:not StaticLib", "configurations:CheckedShared", "architecture:ARM" }
		targetdir ( "../../bin/checked/" .. _ACTION .. "-shared/arm-32-" .. mpt_bindirsuffix32 )
  filter { "kind:not StaticLib", "configurations:Release", "architecture:ARM" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-static/arm-32-" .. mpt_bindirsuffix32 )
  filter { "kind:not StaticLib", "configurations:ReleaseShared", "architecture:ARM" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-shared/arm-32-" .. mpt_bindirsuffix32 )
  filter { "kind:not StaticLib", "configurations:Debug", "architecture:ARM64" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-static/arm-64-" .. mpt_bindirsuffix64 )
  filter { "kind:not StaticLib", "configurations:DebugShared", "architecture:ARM64" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-shared/arm-64-" .. mpt_bindirsuffix64 )
  filter { "kind:not StaticLib", "configurations:Checked", "architecture:ARM64" }
		targetdir ( "../../bin/checked/" .. _ACTION .. "-static/arm-64-" .. mpt_bindirsuffix64 )
  filter { "kind:not StaticLib", "configurations:CheckedShared", "architecture:ARM64" }
		targetdir ( "../../bin/checked/" .. _ACTION .. "-shared/arm-64-" .. mpt_bindirsuffix64 )
  filter { "kind:not StaticLib", "configurations:Release", "architecture:ARM64" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-static/arm-64-" .. mpt_bindirsuffix64 )
  filter { "kind:not StaticLib", "configurations:ReleaseShared", "architecture:ARM64" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-shared/arm-64-" .. mpt_bindirsuffix64 )


	filter { "configurations:Debug" }
   defines { "DEBUG" }
   defines { "MPT_BUILD_DEBUG" }
   defines { "MPT_BUILD_MSVC_STATIC" }
		if _ACTION == "vs2015" then
			symbols "On"
		else
			symbols "FastLink"
		end
   staticruntime "On"
	 runtime "Debug"
   optimize "Debug"

  filter { "configurations:DebugShared" }
   defines { "DEBUG" }
   defines { "MPT_BUILD_DEBUG" }
   defines { "MPT_BUILD_MSVC_SHARED" }
   flags { "MultiProcessorCompile" }  -- implies NotIncremental
   symbols "On"
	 runtime "Debug"
   optimize "Debug"

	 
  filter { "configurations:Checked" }
   defines { "DEBUG" }
   defines { "MPT_BUILD_MSVC_STATIC" }
   defines { "MPT_BUILD_CHECKED" }
   symbols "On"
   flags { "MultiProcessorCompile" }
   staticruntime "On"
	 runtime "Release"
   optimize "On"
	 omitframepointer "Off"
   floatingpoint "Default"

  filter { "configurations:CheckedShared" }
   defines { "DEBUG" }
   defines { "MPT_BUILD_MSVC_SHARED" }
   defines { "MPT_BUILD_CHECKED" }
   symbols "On"
   flags { "MultiProcessorCompile" }
	 runtime "Release"
   optimize "On"
	 omitframepointer "Off"
   floatingpoint "Default"

	 
  filter { "configurations:Release" }
   defines { "NDEBUG" }
   defines { "MPT_BUILD_MSVC_STATIC" }
   symbols "On"
   flags { "MultiProcessorCompile", "LinkTimeOptimization" }
   staticruntime "On"
	 runtime "Release"
   optimize "Speed"
   floatingpoint "Fast"

  filter { "configurations:ReleaseShared" }
   defines { "NDEBUG" }
   defines { "MPT_BUILD_MSVC_SHARED" }
   symbols "On"
   flags { "MultiProcessorCompile", "LinkTimeOptimization" }
	 runtime "Release"
   optimize "Speed"
   floatingpoint "Fast"


	filter {}

	if _OPTIONS["xp"] then

		filter { "architecture:x86" }
			vectorextensions "IA32"
		filter {}

	else

		filter {}

		filter { "architecture:x86", "configurations:Checked" }
			vectorextensions "SSE2"

		filter { "architecture:x86", "configurations:CheckedShared" }
			vectorextensions "SSE2"

		filter { "architecture:x86", "configurations:Release" }
			vectorextensions "SSE2"

		filter { "architecture:x86", "configurations:ReleaseShared" }
			vectorextensions "SSE2"

		filter {}
	
	end

  filter {}
	defines { "MPT_BUILD_MSVC" }

  filter {}
   defines { "WIN32", "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_DEPRECATE", "_CRT_SECURE_NO_DEPRECATE", "_CRT_NONSTDC_NO_WARNINGS" }

  filter {}
