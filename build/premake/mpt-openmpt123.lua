 
 project "openmpt123"
  uuid "2879F62E-9E2F-4EAB-AE7D-F60C194DD5CB"
  language "C++"
  vpaths { ["*"] = "../../" }
  mpt_kind "Console"
  warnings "Extra"
	
	mpt_use_libopenmpt()
	
	mpt_use_flac()
	mpt_use_portaudio()
	
  includedirs {
   "../..",
   "../../src",
   "../../openmpt123",
   "$(IntDir)/svn_version",
  }
  files {
   "../../src/mpt/base/*.hpp",
   "../../src/mpt/detect/*.hpp",
   "../../src/mpt/string/*.hpp",
   "../../openmpt123/*.cpp",
   "../../openmpt123/*.hpp",
  }
	
	filter { "action:vs*", "kind:SharedLib or ConsoleApp or WindowedApp" }
		resdefines {
			"MPT_BUILD_VER_FILENAME=\"" .. "openmpt123" .. ".exe\"",
			"MPT_BUILD_VER_FILEDESC=\"" .. "openmpt123" .. "\"",
		}
	filter { "action:vs*", "kind:SharedLib or ConsoleApp or WindowedApp" }
		resincludedirs {
			"$(IntDir)/svn_version",
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
    "ksuser",
    "winmm",
  }
  defines { "MPT_WITH_FLAC" }
  defines { "MPT_WITH_PORTAUDIO" }
  filter {}
  filter { "configurations:*Shared" }
  filter { "not configurations:*Shared" }
   defines { "FLAC__NO_DLL" }
  filter {}
  
  filter {}
  filter { "action:vs*" }
		linkoptions { "wsetargv.obj" }
  filter {}
  prebuildcommands { "..\\..\\build\\svn_version\\update_svn_version_vs_premake.cmd $(IntDir)" }
