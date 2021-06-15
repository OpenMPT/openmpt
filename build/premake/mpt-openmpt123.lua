 
 project "openmpt123"
  uuid "2879F62E-9E2F-4EAB-AE7D-F60C194DD5CB"
  language "C++"
  location ( "../../build/" .. mpt_projectpathname )
  vpaths { ["*"] = "../../" }
  mpt_projectname = "openmpt123"
  dofile "../../build/premake/premake-defaults-EXE.lua"
  dofile "../../build/premake/premake-defaults.lua"
  warnings "Extra"
  local extincludedirs = {
   "../..",
   "../../include/flac/include",
   "../../include/portaudio/include",
  }
	filter { "action:vs*" }
		includedirs ( extincludedirs )
	filter { "not action:vs*" }
		sysincludedirs ( extincludedirs )
	filter {}
  includedirs {
   "../..",
   "../../src",
   "../../openmpt123",
   "$(IntDir)/svn_version",
   "../../build/svn_version",
  }
  files {
   "../../src/mpt/base/*.hpp",
   "../../src/mpt/detect/*.hpp",
   "../../src/mpt/string/*.hpp",
   "../../openmpt123/*.cpp",
   "../../openmpt123/*.hpp",
  }
  defines { }
	
	filter { "action:vs*", "kind:SharedLib or ConsoleApp or WindowedApp" }
		resdefines {
			"MPT_BUILD_VER_FILENAME=\"" .. mpt_projectname .. ".exe\"",
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
  links {
    "libopenmpt",
    "flac",
    "portaudio",
    "ksuser",
    "winmm",
  }
  filter {}
  filter { "action:vs*" }
		linkoptions { "wsetargv.obj" }
  filter {}
  filter { "configurations:*Shared" }
   defines { "LIBOPENMPT_USE_DLL" }
  filter { "not configurations:*Shared" }
  filter {}
  prebuildcommands { "..\\..\\build\\svn_version\\update_svn_version_vs_premake.cmd $(IntDir)" }
