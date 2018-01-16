
 project "foo_openmpt"
  uuid "749102d3-b183-420c-a7d7-d8ff343c1a0c"
  language "C++"
  location ( "../../build/" .. mpt_projectpathname )
  vpaths { ["*"] = "../../libopenmpt/" }
  mpt_projectname = "foo_openmpt"
  dofile "../../build/premake/premake-defaults-DLL.lua"
  dofile "../../build/premake/premake-defaults.lua"
  local extincludedirs = {
   "../..",
   "../../include/foobar2000sdk",
  }
	filter { "action:vs*" }
		includedirs ( extincludedirs )
	filter { "not action:vs*" }
		sysincludedirs ( extincludedirs )
	filter {}
  includedirs {
   "../..",
   "$(IntDir)/svn_version",
   "../../build/svn_version",
  }
  files {
   "../../libopenmpt/foo_openmpt.cpp",
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
	buildoptions { "/d2notypeopt" } -- https://hydrogenaud.io/index.php/topic,108411.0.html
  links { "libopenmpt", "zlib", "vorbis", "ogg", "mpg123" }
	links { "delayimp" }
   linkoptions {
    "/DELAYLOAD:openmpt-mpg123.dll",
   }
	links { "pfc", "foobar2000_SDK", "foobar2000_sdk_helpers", "foobar2000_component_client", "../../include/foobar2000sdk/foobar2000/shared/shared.lib" }
  filter {}
  prebuildcommands { "..\\..\\build\\svn_version\\update_svn_version_vs_premake.cmd $(IntDir)" }
  filter { "configurations:Release" }
   flags { "LinkTimeOptimization" }
  filter {}
