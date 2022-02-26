
	filter {}
		location ( "../../build/" .. mpt_projectpathname )

	filter {}
		preferredtoolarchitecture "x86_64"

	configurations { "Debug", "Release", "Checked", "DebugShared", "ReleaseShared", "CheckedShared" }
	platforms ( allplatforms )

	filter { "platforms:x86" }
		system "Windows"
		architecture "x86"
	filter { "platforms:x86_64" }
		system "Windows"
		architecture "x86_64"
	filter { "platforms:arm" }
		system "Windows"
		architecture "ARM"
	filter { "platforms:arm64" }
		system "Windows"
		architecture "ARM64"
	filter {}
	
	function mpt_kind(mykind)
		if mykind == "" then
			-- nothing
		elseif mykind == "default" then
			filter {}
			filter { "configurations:Debug" }
				kind "StaticLib"
			filter { "configurations:DebugShared" }
				kind "SharedLib"
			filter { "configurations:Checked" }
				kind "StaticLib"
			filter { "configurations:CheckedShared" }
				kind "SharedLib"
			filter { "configurations:Release" }
				kind "StaticLib"
			filter { "configurations:ReleaseShared" }
				kind "SharedLib"
			filter {}
		elseif mykind == "shared" then
			kind "SharedLib"
		elseif mykind == "static" then
			kind "StaticLib"
		elseif mykind == "GUI" then
			kind "WindowedApp"
			if _OPTIONS["windows-version"] == "win10" then
				files {
					"../../build/vs/win10.manifest",
				}
			elseif  _OPTIONS["windows-version"] == "win81" then
				files {
					"../../build/vs/win81.manifest",
				}
			elseif  _OPTIONS["windows-version"] == "win7" then
				files {
					"../../build/vs/win7.manifest",
				}
			end
		elseif mykind == "Console" then
			kind "ConsoleApp"
		else
			-- nothing
		end
	end

	filter {}
		objdir ( "../../build/obj/" .. mpt_projectpathname .. "/" .. "%{prj.name}" )
	filter {}

	filter {}
		if _OPTIONS["clang"] then
			toolset "clang"
		end
	filter {}

	filter {}
		if _OPTIONS["windows-version"] == "winxp" then
			if _ACTION == "vs2017" then
				toolset "v141_xp"
			end
			defines { "MPT_BUILD_RETRO" }
			filter { "action:vs*" }
				buildoptions { "/Zc:threadSafeInit-" }
			filter {}
		end
	filter {}

	filter {}

	filter {}
		cdialect "C17"
	filter { "action:vs*", "language:C++", "action:vs2017" }
		cppdialect "C++17"
	filter { "action:vs*", "language:C++", "action:vs2019" }
		cppdialect "C++17"
	filter { "action:vs*", "language:C++", "not action:vs2017", "not action:vs2019" }
		if _OPTIONS["clang"] then
			cppdialect "C++17"
		else
			cppdialect "C++20"
		end
	filter { "action:vs*", "action:vs2017" }
		if _OPTIONS["windows-version"] == "win10" then
			conformancemode "On"
		end
	filter { "action:vs*", "not action:vs2017" }
		conformancemode "On"
	filter { "not action:vs*", "language:C++" }
		buildoptions { "-std=c++17" }
	filter { "not action:vs*", "language:C" }
		buildoptions { "-std=c17" }
	filter {}

	filter {}
	filter { "action:vs*" }
		if not _OPTIONS["clang"] and _OPTIONS["windows-version"] ~= "winxp" and _OPTIONS["windows-family"] ~= "uwp" then
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

  filter { "kind:StaticLib" }
	targetdir ( "../../build/lib/" .. mpt_projectpathname .. "/%{cfg.architecture}/%{cfg.buildcfg}" )
  	
  filter { "kind:not StaticLib", "configurations:Debug", "architecture:x86" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-static/x86" )
  filter { "kind:not StaticLib", "configurations:DebugShared", "architecture:x86" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-shared/x86" )
  filter { "kind:not StaticLib", "configurations:Checked", "architecture:x86" }
		targetdir ( "../../bin/checked/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-static/x86" )
  filter { "kind:not StaticLib", "configurations:CheckedShared", "architecture:x86" }
		targetdir ( "../../bin/checked/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-shared/x86" )
  filter { "kind:not StaticLib", "configurations:Release", "architecture:x86" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-static/x86" )
  filter { "kind:not StaticLib", "configurations:ReleaseShared", "architecture:x86" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-shared/x86" )
  filter { "kind:not StaticLib", "configurations:Debug", "architecture:x86_64" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-static/amd64" )
  filter { "kind:not StaticLib", "configurations:DebugShared", "architecture:x86_64" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-shared/amd64" )
  filter { "kind:not StaticLib", "configurations:Checked", "architecture:x86_64" }
		targetdir ( "../../bin/checked/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-static/amd64" )
  filter { "kind:not StaticLib", "configurations:CheckedShared", "architecture:x86_64" }
		targetdir ( "../../bin/checked/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-shared/amd64" )
  filter { "kind:not StaticLib", "configurations:Release", "architecture:x86_64" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-static/amd64" )
  filter { "kind:not StaticLib", "configurations:ReleaseShared", "architecture:x86_64" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-shared/amd64" )
		
  filter { "kind:not StaticLib", "configurations:Debug", "architecture:ARM" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-static/arm" )
  filter { "kind:not StaticLib", "configurations:DebugShared", "architecture:ARM" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-shared/arm" )
  filter { "kind:not StaticLib", "configurations:Checked", "architecture:ARM" }
		targetdir ( "../../bin/checked/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-static/arm" )
  filter { "kind:not StaticLib", "configurations:CheckedShared", "architecture:ARM" }
		targetdir ( "../../bin/checked/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-shared/arm" )
  filter { "kind:not StaticLib", "configurations:Release", "architecture:ARM" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-static/arm" )
  filter { "kind:not StaticLib", "configurations:ReleaseShared", "architecture:ARM" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-shared/arm" )
  filter { "kind:not StaticLib", "configurations:Debug", "architecture:ARM64" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-static/arm64" )
  filter { "kind:not StaticLib", "configurations:DebugShared", "architecture:ARM64" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-shared/arm64" )
  filter { "kind:not StaticLib", "configurations:Checked", "architecture:ARM64" }
		targetdir ( "../../bin/checked/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-static/arm64" )
  filter { "kind:not StaticLib", "configurations:CheckedShared", "architecture:ARM64" }
		targetdir ( "../../bin/checked/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-shared/arm64" )
  filter { "kind:not StaticLib", "configurations:Release", "architecture:ARM64" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-static/arm64" )
  filter { "kind:not StaticLib", "configurations:ReleaseShared", "architecture:ARM64" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-shared/arm64" )

	filter { "configurations:Debug", "architecture:ARM" }
		editandcontinue "Off"
	filter { "configurations:Debug", "architecture:ARM64" }
		editandcontinue "Off"
	filter { "configurations:DebugShared", "architecture:ARM" }
		editandcontinue "Off"
	filter { "configurations:DebugShared", "architecture:ARM64" }
		editandcontinue "Off"

	filter { "configurations:Debug" }
   defines { "DEBUG" }
   defines { "MPT_BUILD_DEBUG" }
   defines { "MPT_BUILD_MSVC_STATIC" }
	filter { "configurations:Debug", "architecture:ARM" }
		symbols "On"
	filter { "configurations:Debug", "architecture:ARM64" }
		symbols "On"
	filter { "configurations:Debug", "architecture:not ARM", "architecture:not ARM64" }
		symbols "FastLink"
	filter { "configurations:Debug" }
		if _OPTIONS["windows-family"] ~= "uwp" then
			staticruntime "On"
		end
	 runtime "Debug"
   optimize "Debug"

  filter { "configurations:DebugShared" }
   defines { "DEBUG" }
   defines { "MPT_BUILD_DEBUG" }
   defines { "MPT_BUILD_MSVC_SHARED" }
   symbols "On"
	 runtime "Debug"
   optimize "Debug"

	 
  filter { "configurations:Checked" }
   defines { "DEBUG" }
   defines { "MPT_BUILD_MSVC_STATIC" }
   defines { "MPT_BUILD_CHECKED" }
   symbols "On"
		if _OPTIONS["windows-family"] ~= "uwp" then
			staticruntime "On"
		end
	 runtime "Release"
   optimize "On"
	 omitframepointer "Off"

  filter { "configurations:CheckedShared" }
   defines { "DEBUG" }
   defines { "MPT_BUILD_MSVC_SHARED" }
   defines { "MPT_BUILD_CHECKED" }
   symbols "On"
	 runtime "Release"
   optimize "On"
	 omitframepointer "Off"

	 
  filter { "configurations:Release" }
   defines { "NDEBUG" }
   defines { "MPT_BUILD_MSVC_STATIC" }
   symbols "On"
		if not _OPTIONS["clang"] then
			flags { "LinkTimeOptimization" }
		end
		if _OPTIONS["windows-family"] ~= "uwp" then
			staticruntime "On"
		end
	 runtime "Release"
   optimize "Speed"

  filter { "configurations:ReleaseShared" }
   defines { "NDEBUG" }
   defines { "MPT_BUILD_MSVC_SHARED" }
   symbols "On"
		if not _OPTIONS["clang"] then
			flags { "LinkTimeOptimization" }
		end
	 runtime "Release"
   optimize "Speed"


	filter {}
		if not _OPTIONS["clang"] then
			flags { "MultiProcessorCompile" }
		end

	if _OPTIONS["windows-version"] == "winxp" then

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
		defines {
			"WIN32",
			"NOMINMAX",
			"_CRT_NONSTDC_NO_WARNINGS",
			"_CRT_SECURE_NO_WARNINGS",
			"_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1",
			"_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT=1",
		}

  filter {}
  
	if _OPTIONS["windows-version"] ~= "winxp" and _OPTIONS["windows-family"] ~= "uwp" then
		filter {}
		filter { "action:vs2017" }
			systemversion "10.0.17763.0"
		filter {}
		filter { "action:vs2019" }
			systemversion "10.0.20348.0"
		filter {}
		filter { "action:vs2022" }
			systemversion "10.0.20348.0"
		filter {}
	end

	if _OPTIONS["windows-version"] == "win10" then
		filter {}
		defines { "_WIN32_WINNT=0x0A00" }
		filter {}
		filter { "architecture:x86" }
			defines { "NTDDI_VERSION=0x0A000000" }
		filter {}
		filter { "architecture:x86_64" }
			defines { "NTDDI_VERSION=0x0A000000" }
		filter {}
		filter { "architecture:ARM" }
			defines { "NTDDI_VERSION=0x0A000004" } -- Windows 10 1709 Build 16299
		filter {}
		filter { "architecture:ARM64" }
			defines { "NTDDI_VERSION=0x0A000004" } -- Windows 10 1709 Build 16299
		filter {}
	elseif _OPTIONS["windows-version"] == "win81" then
		filter {}
		defines { "_WIN32_WINNT=0x0603" }
		defines { "NTDDI_VERSION=0x06030000" }
	elseif _OPTIONS["windows-version"] == "win7" then
		filter {}
		defines { "_WIN32_WINNT=0x0601" }
		defines { "NTDDI_VERSION=0x06010000" }
	elseif _OPTIONS["windows-version"] == "winxp" then
		filter {}
		systemversion "7.0"
		filter {}
		filter { "architecture:x86" }
			defines { "_WIN32_WINNT=0x0501" }
			defines { "NTDDI_VERSION=0x05010100" } -- Windows XP SP1
		filter { "architecture:x86_64" }
			defines { "_WIN32_WINNT=0x0502" }
			defines { "NTDDI_VERSION=0x05020000" } -- Windows XP x64
		filter {}
	end

  filter {}
