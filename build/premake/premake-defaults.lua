
	filter {}
		objdir ( "../../build/obj/" .. mpt_projectpathname .. "/" .. mpt_projectname )
	filter {}

	filter {}
		if _OPTIONS["clang"] then
			toolset "msc-ClangCL"
		end
	filter {}

	filter {}

	filter {}
	filter { "action:vs*", "language:C++" }
		cppdialect "C++17"
	filter { "action:vs*", "action:vs2017" }
		if _OPTIONS["win10"] then
			standardconformance "On"
		end
	filter { "action:vs*", "not action:vs2017" }
		standardconformance "On"
	filter { "not action:vs*", "language:C++" }
		buildoptions { "-std=c++17" }
	filter { "not action:vs*", "language:C" }
		buildoptions { "-std=c99" }
	filter {}

	filter {}
	filter { "action:vs*" }
		if not _OPTIONS["clang"] then
			spectremitigations "On"
		end
	filter {}
	filter { "action:vs*", "architecture:x86" }
		resdefines { "VER_ARCHNAME=\"x86\"" }
	filter { "action:vs*", "architecture:x86_64" }
		resdefines { "VER_ARCHNAME=\"amd64\"" }
	filter { "action:vs*", "architecture:ARM" }
		resdefines { "VER_ARCHNAME=\"arm\"" }
	filter { "action:vs*", "architecture:ARM64" }
		resdefines { "VER_ARCHNAME=\"arm64\"" }
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
		targetdir ( "../../bin/debug/" .. _ACTION .. "-" .. mpt_bindirsuffix32 .. "-static/x86" )
  filter { "kind:not StaticLib", "configurations:DebugShared", "architecture:x86" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-" .. mpt_bindirsuffix32 .. "-shared/x86" )
  filter { "kind:not StaticLib", "configurations:Checked", "architecture:x86" }
		targetdir ( "../../bin/checked/" .. _ACTION .. "-" .. mpt_bindirsuffix32 .. "-static/x86" )
  filter { "kind:not StaticLib", "configurations:CheckedShared", "architecture:x86" }
		targetdir ( "../../bin/checked/" .. _ACTION .. "-" .. mpt_bindirsuffix32 .. "-shared/x86" )
  filter { "kind:not StaticLib", "configurations:Release", "architecture:x86" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-" .. mpt_bindirsuffix32 .. "-static/x86" )
  filter { "kind:not StaticLib", "configurations:ReleaseShared", "architecture:x86" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-" .. mpt_bindirsuffix32 .. "-shared/x86" )
  filter { "kind:not StaticLib", "configurations:Debug", "architecture:x86_64" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-" .. mpt_bindirsuffix64 .. "-static/amd64" )
  filter { "kind:not StaticLib", "configurations:DebugShared", "architecture:x86_64" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-" .. mpt_bindirsuffix64 .. "-shared/amd64" )
  filter { "kind:not StaticLib", "configurations:Checked", "architecture:x86_64" }
		targetdir ( "../../bin/checked/" .. _ACTION .. "-" .. mpt_bindirsuffix64 .. "-static/amd64" )
  filter { "kind:not StaticLib", "configurations:CheckedShared", "architecture:x86_64" }
		targetdir ( "../../bin/checked/" .. _ACTION .. "-" .. mpt_bindirsuffix64 .. "-shared/amd64" )
  filter { "kind:not StaticLib", "configurations:Release", "architecture:x86_64" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-" .. mpt_bindirsuffix64 .. "-static/amd64" )
  filter { "kind:not StaticLib", "configurations:ReleaseShared", "architecture:x86_64" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-" .. mpt_bindirsuffix64 .. "-shared/amd64" )
		
  filter { "kind:not StaticLib", "configurations:Debug", "architecture:ARM" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-" .. mpt_bindirsuffix32 .. "-static/arm" )
  filter { "kind:not StaticLib", "configurations:DebugShared", "architecture:ARM" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-" .. mpt_bindirsuffix32 .. "-shared/arm" )
  filter { "kind:not StaticLib", "configurations:Checked", "architecture:ARM" }
		targetdir ( "../../bin/checked/" .. _ACTION .. "-" .. mpt_bindirsuffix32 .. "-static/arm" )
  filter { "kind:not StaticLib", "configurations:CheckedShared", "architecture:ARM" }
		targetdir ( "../../bin/checked/" .. _ACTION .. "-" .. mpt_bindirsuffix32 .. "-shared/arm" )
  filter { "kind:not StaticLib", "configurations:Release", "architecture:ARM" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-" .. mpt_bindirsuffix32 .. "-static/arm" )
  filter { "kind:not StaticLib", "configurations:ReleaseShared", "architecture:ARM" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-" .. mpt_bindirsuffix32 .. "-shared/arm" )
  filter { "kind:not StaticLib", "configurations:Debug", "architecture:ARM64" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-" .. mpt_bindirsuffix64 .. "-static/arm64" )
  filter { "kind:not StaticLib", "configurations:DebugShared", "architecture:ARM64" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-" .. mpt_bindirsuffix64 .. "-shared/arm64" )
  filter { "kind:not StaticLib", "configurations:Checked", "architecture:ARM64" }
		targetdir ( "../../bin/checked/" .. _ACTION .. "-" .. mpt_bindirsuffix64 .. "-static/arm64" )
  filter { "kind:not StaticLib", "configurations:CheckedShared", "architecture:ARM64" }
		targetdir ( "../../bin/checked/" .. _ACTION .. "-" .. mpt_bindirsuffix64 .. "-shared/arm64" )
  filter { "kind:not StaticLib", "configurations:Release", "architecture:ARM64" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-" .. mpt_bindirsuffix64 .. "-static/arm64" )
  filter { "kind:not StaticLib", "configurations:ReleaseShared", "architecture:ARM64" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-" .. mpt_bindirsuffix64 .. "-shared/arm64" )


	filter { "configurations:Debug" }
   defines { "DEBUG" }
   defines { "MPT_BUILD_DEBUG" }
   defines { "MPT_BUILD_MSVC_STATIC" }
   flags { "MultiProcessorCompile" }  -- implies NotIncremental
		symbols "FastLink"
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
		flags { "MultiProcessorCompile" }
		if not _OPTIONS["clang"] then
			flags { "LinkTimeOptimization" }
		end
   staticruntime "On"
	 runtime "Release"
   optimize "Speed"
--		if _OPTIONS["clang"] then
--			floatingpoint "Default"
--		else
			floatingpoint "Fast"
--		end

  filter { "configurations:ReleaseShared" }
   defines { "NDEBUG" }
   defines { "MPT_BUILD_MSVC_SHARED" }
   symbols "On"
		flags { "MultiProcessorCompile" }
		if not _OPTIONS["clang"] then
			flags { "LinkTimeOptimization" }
		end
	 runtime "Release"
   optimize "Speed"
--		if _OPTIONS["clang"] then
--			floatingpoint "Default"
--		else
			floatingpoint "Fast"
--		end


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

  filter {}
	defines { "MPT_BUILD_MSVC" }

  filter {}
   defines { "WIN32", "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_DEPRECATE", "_CRT_SECURE_NO_DEPRECATE", "_CRT_NONSTDC_NO_WARNINGS" }

  filter {}
