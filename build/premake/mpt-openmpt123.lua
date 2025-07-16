 
 project "openmpt123"
  uuid "2879F62E-9E2F-4EAB-AE7D-F60C194DD5CB"
  language "C++"
  vpaths { ["*"] = "../../" }
  mpt_kind "Console"
  warnings "Extra"
	
	mpt_use_libopenmpt()
	
	mpt_use_flac()
	mpt_use_portaudio()
	
	defines {
		"MPT_WITH_FLAC",
		"MPT_WITH_PORTAUDIO",
	}

	if _ACTION < "vs2022" then
		mpt_use_pthread_win32()
		defines { "MPT_WITH_PTHREAD" }
	end

	files {
		"../../openmpt123/openmpt123.manifest",
	}
  includedirs {
   "../..",
   "../../src",
   "../../openmpt123",
   "$(IntDir)/svn_version",
  }
  files {
   "../../src/mpt/base/*.hpp",
   "../../src/mpt/chrono/*.hpp",
   "../../src/mpt/detect/*.hpp",
   "../../src/mpt/exception/*.hpp",
   "../../src/mpt/filemode/*.hpp",
   "../../src/mpt/format/*.hpp",
   "../../src/mpt/io/*.hpp",
   "../../src/mpt/io_file/*.hpp",
   "../../src/mpt/main/*.hpp",
   "../../src/mpt/out_of_memory/*.hpp",
   "../../src/mpt/parse/*.hpp",
   "../../src/mpt/path/*.hpp",
   "../../src/mpt/random/*.hpp",
   "../../src/mpt/string/*.hpp",
   "../../src/mpt/string_transcode/*.hpp",
   "../../src/mpt/terminal/**.cpp",
   "../../src/mpt/terminal/**.hpp",
   "../../openmpt123/*.cpp",
   "../../openmpt123/*.hpp",
  }

	filter {}
	filter { "action:vs*", "kind:SharedLib or ConsoleApp or WindowedApp" }
		resdefines {
			"MPT_BUILD_VER_FILENAME=\"" .. "openmpt123" .. ".exe\"",
			"MPT_BUILD_VER_FILEDESC=\"" .. "openmpt123" .. "\"",
		}
	filter {}
	filter { "action:vs*", "kind:SharedLib or ConsoleApp or WindowedApp" }
		resincludedirs {
			"$(IntDir)/svn_version",
			"../../build/svn_version",
			"$(ProjDir)/../../build/svn_version",
		}
		files {
			"../../libopenmpt/libopenmpt_version.rc",
		}
	filter {}
	filter { "action:vs*", "kind:SharedLib" }
		resdefines { "MPT_BUILD_VER_DLL" }
	filter {}
	filter { "action:vs*", "kind:ConsoleApp or WindowedApp" }
		resdefines { "MPT_BUILD_VER_EXE" }
	filter {}

	if _OPTIONS["charset"] ~= "Unicode" then
		defines { "MPT_CHECK_WINDOWS_IGNORE_WARNING_NO_UNICODE" }
	end

  links {
    "ksuser",
    "winmm",
  }
  
  filter {}
	if _OPTIONS["windows-family"] ~= "uwp" then
		filter { "action:vs*" }
			linkoptions { "wsetargv.obj" }
		filter {}
	end
  prebuildcommands { "..\\..\\build\\svn_version\\update_svn_version_vs_premake.cmd $(IntDir)" }
