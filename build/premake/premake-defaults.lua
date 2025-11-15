
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
	filter { "platforms:arm64ec" }
		system "Windows"
		architecture "ARM64EC"
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
		elseif mykind == "Console" then
			kind "ConsoleApp"
		else
			-- nothing
		end
		if mykind == "GUI" or mykind == "Console" then
			if MPT_WIN_AT_LEAST(MPT_WIN["11"]) then
				files {
					"../../build/vs/win10.manifest",
				}
			elseif MPT_WIN_AT_LEAST(MPT_WIN["10"]) then
				files {
					"../../build/vs/win10.manifest",
				}
			elseif MPT_WIN_AT_LEAST(MPT_WIN["81"]) then
				files {
					"../../build/vs/win81.manifest",
				}
			elseif MPT_WIN_AT_LEAST(MPT_WIN["8"]) then
				files {
					"../../build/vs/win8.manifest",
				}
			elseif MPT_WIN_AT_LEAST(MPT_WIN["7"]) then
				files {
					"../../build/vs/win7.manifest",
				}
			else
				-- nothing
			end
		end
	end

	filter {}
		objdir ( "../../build/obj/" .. mpt_projectpathname .. "/" .. "%{prj.name}" )
	filter {}

	filter {}
		if MPT_BUILD_MSBUILD and MPT_COMPILER_CLANGCL then
			toolset "clang"
		end
	filter {}

	filter {}
		if MPT_WIN_BEFORE(MPT_WIN["VISTA"]) then
			filter {}
			filter { "action:vs*" }
				toolset "v141_xp"
				buildoptions { "/Zc:threadSafeInit-" }
			filter {}
		end
	filter {}

	filter {}
		characterset ( _OPTIONS["windows-charset"] )
		largeaddressaware ( true )
	filter {}

	filter {}
		cdialect "C17"
	filter { "action:vs*", "language:C++", "action:vs2017" }
		cppdialect "C++17"
	filter { "action:vs*", "language:C++", "action:vs2019" }
		cppdialect "C++17"
	filter { "action:vs*", "language:C++", "not action:vs2017", "not action:vs2019" }
		cppdialect "C++20"
	filter { "action:vs*", "action:vs2017" }
		if MPT_WIN_AT_LEAST(MPT_WIN["10"]) then
			conformancemode "On"
		end
	filter { "action:vs*", "action:vs2017" }
		defines { "MPT_CHECK_CXX_IGNORE_PREPROCESSOR" }
	filter { "action:vs*", "not action:vs2017" }
		usestandardpreprocessor "On"
		conformancemode "On"
	filter {}

	filter {}
	if MPT_BUILD_MSBUILD then
		if MPT_COMPILER_MSVC and MPT_WIN_AT_LEAST(MPT_WIN["7"]) and not MPT_OS_WINDOWS_WINRT then
			spectremitigations "On"
		end
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
	filter { "action:vs*", "architecture:ARM64EC" }
		resdefines { "VER_ARCHNAME=\"arm64ec\"" }
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
  filter { "kind:not StaticLib", "configurations:Debug", "architecture:ARM64EC" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-static/arm64ec" )
  filter { "kind:not StaticLib", "configurations:DebugShared", "architecture:ARM64EC" }
		targetdir ( "../../bin/debug/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-shared/arm64ec" )
  filter { "kind:not StaticLib", "configurations:Checked", "architecture:ARM64EC" }
		targetdir ( "../../bin/checked/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-static/arm64ec" )
  filter { "kind:not StaticLib", "configurations:CheckedShared", "architecture:ARM64EC" }
		targetdir ( "../../bin/checked/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-shared/arm64ec" )
  filter { "kind:not StaticLib", "configurations:Release", "architecture:ARM64EC" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-static/arm64ec" )
  filter { "kind:not StaticLib", "configurations:ReleaseShared", "architecture:ARM64EC" }
		targetdir ( "../../bin/release/" .. _ACTION .. "-" .. mpt_bindirsuffix .. "-shared/arm64ec" )

	filter { "configurations:Debug", "architecture:ARM" }
		editandcontinue "Off"
	filter { "configurations:Debug", "architecture:ARM64" }
		editandcontinue "Off"
	filter { "configurations:Debug", "architecture:ARM64EC" }
		editandcontinue "Off"
	filter { "configurations:DebugShared", "architecture:ARM" }
		editandcontinue "Off"
	filter { "configurations:DebugShared", "architecture:ARM64" }
		editandcontinue "Off"
	filter { "configurations:DebugShared", "architecture:ARM64EC" }
		editandcontinue "Off"

	filter { "configurations:Debug" }
   defines { "DEBUG" }
   defines { "MPT_BUILD_DEBUG" }
	if MPT_MSVC_AT_LEAST(2017) and MPT_MSVC_BEFORE(2026) then
		filter { "configurations:Debug", "architecture:ARM" }
			symbols "On"
		filter { "configurations:Debug", "architecture:ARM64" }
			symbols "On"
		filter { "configurations:Debug", "architecture:ARM64EC" }
			symbols "On"
		filter { "configurations:Debug", "architecture:not ARM", "architecture:not ARM64", "architecture:not ARM64EC" }
			symbols "FastLink"
	else
		filter { "configurations:Debug" }
			symbols "On"
	end
	filter { "configurations:Debug" }
		if not MPT_OS_WINDOWS_WINRT then
			staticruntime "On"
		end
	 runtime "Debug"
   optimize "Debug"

  filter { "configurations:DebugShared" }
   defines { "DEBUG" }
   defines { "MPT_BUILD_DEBUG" }
   symbols "On"
	 runtime "Debug"
   optimize "Debug"

	 
  filter { "configurations:Checked" }
   defines { "DEBUG" }
   defines { "MPT_BUILD_CHECKED" }
   symbols "On"
		if not MPT_OS_WINDOWS_WINRT then
			staticruntime "On"
		end
	 runtime "Release"
   optimize "On"
		if MPT_MSVC_AT_LEAST(2022) then
			buildoptions { "/Gw" }
			buildoptions { "/Zc:checkGwOdr" }
		end
	 omitframepointer "Off"

  filter { "configurations:CheckedShared" }
   defines { "DEBUG" }
   defines { "MPT_BUILD_CHECKED" }
   symbols "On"
	 runtime "Release"
   optimize "On"
		if MPT_MSVC_AT_LEAST(2022) then
			buildoptions { "/Gw" }
			buildoptions { "/Zc:checkGwOdr" }
		end
	 omitframepointer "Off"

	 
  filter { "configurations:Release" }
   defines { "NDEBUG" }
   symbols "On"
		if MPT_COMPILER_MSVC then
			linktimeoptimization "On"
		end
		if MPT_MSVC_AT_LEAST(2022) then
			buildoptions { "/Gw" }
			buildoptions { "/Zc:checkGwOdr" }
		end
		if not MPT_OS_WINDOWS_WINRT then
			staticruntime "On"
		end
	 runtime "Release"
   optimize "Speed"

  filter { "configurations:ReleaseShared" }
   defines { "NDEBUG" }
   symbols "On"
		if MPT_COMPILER_MSVC then
			linktimeoptimization "Default"
			linktimeoptimization2 "Fast"
		end
		if MPT_MSVC_AT_LEAST(2022) then
			buildoptions { "/Gw" }
			buildoptions { "/Zc:checkGwOdr" }
		end
	 runtime "Release"
   optimize "Speed"


	filter {}
		if MPT_BUILD_MSBUILD and MPT_COMPILER_CLANGCL then
			-- work-around
			-- <https://github.com/llvm/llvm-project/issues/56285>
			symbols "Off"
		end

	filter {}
		if MPT_BUILD_MSBUILD and MPT_COMPILER_MSVC then
			flags { "MultiProcessorCompile" }
		end

	if MPT_WIN_BEFORE(MPT_WIN["7"]) then
		filter {}
		filter { "architecture:x86" }
			vectorextensions "IA32"
		filter {}
	else
		filter {}
		filter { "architecture:x86" }
			vectorextensions "SSE2"
		filter {}
	end

	if MPT_WIN_AT_LEAST(MPT_WIN["11_24H2"]) then
		filter {}
		filter { "architecture:x86_64" }
			if MPT_MSVC_AT_LEAST(2022) then
				buildoptions { "/arch:SSE4.2" }
				defines { "MPT_BUILD_MSVC_REQUIRE_SSE42" }
			elseif MPT_COMPILER_CLANGCL then
				-- not supported at the moment for clang-cl
				--vectorextensions "SSE4.2"
			end
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

	if not MPT_OS_WINDOWS_WINRT then
		filter {}
		filter { "action:vs2017" }
			if MPT_WIN_BEFORE(MPT_WIN["VISTA"]) then
				systemversion "7.0"
			elseif MPT_WIN_BEFORE(MPT_WIN["10"]) then
				systemversion "8.1"
			else
				systemversion "10.0.19041.0"
			end
		filter {}
		filter { "action:vs2019" }
			systemversion "10.0.22621.0"
		filter {}
		filter { "action:vs2022" }
			if MPT_WIN_BEFORE(MPT_WIN["10"]) then
				systemversion "10.0.22621.0"
			elseif MPT_WIN_BEFORE(MPT_WIN["11"]) then
				systemversion "10.0.22621.0"
			else
				systemversion "10.0.26100.0"
			end
		filter { "action:vs2026" }
			systemversion "10.0.26100.0"
		filter {}
	end

	filter {}
		defines { "_WIN32_WINNT" .. "=" .. "0x" .. string.format("%04X", MPT_WIN_VERSION >> 16) }
		defines { "NTDDI_VERSION" .. "=" .. "0x" .. string.format("%08X", MPT_WIN_VERSION) }
	filter {}

	filter {}
