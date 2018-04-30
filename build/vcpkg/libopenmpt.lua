
solution "libopenmpt"
	location ( "../../build/vcpkg" )
	configurations { "Debug", "Release", "DebugStatic", "ReleaseStatic" }
	--platforms ( { "x86", "x86_64", "arm" } )
	platforms ( { "x86", "x86_64" } )

	filter { "platforms:x86" }
		system "Windows"
		architecture "x32"
	filter { "platforms:x86_64" }
		system "Windows"
		architecture "x64"
	filter { "platforms:arm" }
		system "Windows"
		architecture "ARM"
	filter {}

	project "libopenmpt"
	uuid "fa8940b7-a2f7-45ba-8091-0a9167b70087"
	language "C++"
	location ( "../../build/vcpkg" )
	vpaths { ["*"] = "../../" }
	mpt_projectname = "libopenmpt"
	
	
	
	filter { "configurations:Debug" }
		kind "SharedLib"
	filter { "configurations:DebugStatic" }
		kind "StaticLib"
	filter { "configurations:Release" }
		kind "SharedLib"
	filter { "configurations:ReleaseStatic" }
		kind "StaticLib"
	filter {}
	
	objdir ( "../../build/vcpkg/obj/" .. mpt_projectname )

	filter { "not action:vs*", "language:C++" }
		buildoptions { "-std=c++11" }
	filter { "not action:vs*", "language:C" }
		buildoptions { "-std=c99" }
	filter {}

	filter { "configurations:Debug" }
		defines { "DEBUG" }
		defines { "MPT_BUILD_MSVC_SHARED" }
		flags { "MultiProcessorCompile" }
		symbols "On"
		runtime "Debug"
		optimize "Debug"

	filter { "configurations:DebugStatic" }
		defines { "DEBUG" }
		defines { "MPT_BUILD_MSVC_STATIC" }
		flags { "MultiProcessorCompile" }
		staticruntime "On"
		symbols "On"
		runtime "Debug"
		optimize "Debug"

	filter { "configurations:Release" }
		defines { "NDEBUG" }
		defines { "MPT_BUILD_MSVC_SHARED" }
		symbols "On"
		flags { "MultiProcessorCompile" }
		runtime "Release"
		optimize "Speed"
		floatingpoint "Fast"

	filter { "configurations:ReleaseStatic" }
		defines { "NDEBUG" }
		defines { "MPT_BUILD_MSVC_STATIC" }
		symbols "On"
		flags { "MultiProcessorCompile" }
		staticruntime "On"
		runtime "Release"
		optimize "Speed"
		floatingpoint "Fast"

	filter { "architecture:x86" }
		vectorextensions "IA32"
	filter {}	

	defines { "WIN32", "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_DEPRECATE", "_CRT_SECURE_NO_DEPRECATE", "_CRT_NONSTDC_NO_WARNINGS" }
	
	buildoptions { "/permissive-", "/Zc:twoPhase-" } -- '/Zc:twoPhase-' is required because combaseapi.h in Windwos 8.1 SDK relies on it
	defines { "_WIN32_WINNT=0x0601" }


	
	includedirs {
		"../..",
		"../../common",
		"../../soundlib",
		"$(IntDir)/svn_version",
		"../../build/svn_version",
	}
	files {
		"../../common/*.cpp",
		"../../common/*.h",
		"../../soundbase/*.cpp",
		"../../soundbase/*.h",
		"../../soundlib/*.cpp",
		"../../soundlib/*.h",
		"../../soundlib/plugins/*.cpp",
		"../../soundlib/plugins/*.h",
		"../../soundlib/plugins/dmo/*.cpp",
		"../../soundlib/plugins/dmo/*.h",
		"../../sounddsp/*.cpp",
		"../../sounddsp/*.h",
		"../../libopenmpt/libopenmpt.h",
		"../../libopenmpt/libopenmpt.hpp",
		"../../libopenmpt/libopenmpt_config.h",
		"../../libopenmpt/libopenmpt_ext.h",
		"../../libopenmpt/libopenmpt_ext.hpp",
		"../../libopenmpt/libopenmpt_ext_impl.hpp",
		"../../libopenmpt/libopenmpt_impl.hpp",
		"../../libopenmpt/libopenmpt_internal.h",
		"../../libopenmpt/libopenmpt_stream_callbacks_buffer.h",
		"../../libopenmpt/libopenmpt_stream_callbacks_fd.h",
		"../../libopenmpt/libopenmpt_stream_callbacks_file.h",
		"../../libopenmpt/libopenmpt_version.h",
		"../../libopenmpt/libopenmpt_c.cpp",
		"../../libopenmpt/libopenmpt_cxx.cpp",
		"../../libopenmpt/libopenmpt_ext_impl.cpp",
		"../../libopenmpt/libopenmpt_impl.cpp",
	}
	filter { "action:vs*", "kind:SharedLib or ConsoleApp or WindowedApp" }
		resdefines {
			"MPT_BUILD_VER_FILENAME=\"" .. mpt_projectname .. ".dll\"",
			"MPT_BUILD_VER_FILEDESC=\"" .. mpt_projectname .. "\"",
		}
	filter { "action:vs*", "kind:SharedLib or ConsoleApp or WindowedApp" }
		resincludedirs {
			"$(IntDir)/svn_version",
			"../../build/svn_version",
			"$(ProjDir)/../../build/svn_version",
		}
		files {
			"../../libopenmpt/libopenmpt_version.rc",
		}
	filter { "action:vs*", "kind:SharedLib" }
		resdefines { "MPT_BUILD_VER_DLL" }
	filter { "action:vs*", "kind:ConsoleApp or WindowedApp" }
		resdefines { "MPT_BUILD_VER_EXE" }
	filter {}
	characterset "Unicode"
	warnings "Extra"
	defines { "LIBOPENMPT_BUILD" }
	filter { "kind:SharedLib" }
		defines { "LIBOPENMPT_BUILD_DLL" }
	filter { "kind:SharedLib" }
	filter {}
	
	defines { "MPT_BUILD_VCPKG" }
	
	defines {
		"MPT_WITH_MPG123",
		"MPT_WITH_OGG",
		"MPT_WITH_VORBIS",
		"MPT_WITH_VORBISFILE",
		"MPT_WITH_ZLIB",
	}
	
	prebuildcommands { "..\\..\\build\\svn_version\\update_svn_version_vs_premake.cmd $(IntDir)" }
