 
 project "libopenmpt-small"
  uuid "25560abd-41fc-444c-9e71-f8502bc7ee96"
  language "C++"
  location ( "../../build/" .. mpt_projectpathname )
  vpaths { ["*"] = "../../" }
  mpt_projectname = "libopenmpt-small"
  dofile "../../build/premake/premake-defaults-LIBorDLL.lua"
  dofile "../../build/premake/premake-defaults.lua"
  local extincludedirs = {
   "../../include",
  }
	filter { "action:vs*" }
		includedirs ( extincludedirs )
	filter { "not action:vs*" }
		sysincludedirs ( extincludedirs )
	filter {}
  includedirs {
   "../..",
   "../../src",
   "../../common",
   "$(IntDir)/svn_version",
   "../../build/svn_version",
  }
  files {
   "../../src/mpt/**.cpp",
   "../../src/mpt/**.hpp",
   "../../src/openmpt/**.cpp",
   "../../src/openmpt/**.hpp",
   "../../common/*.cpp",
   "../../common/*.h",
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
	excludes {
		"../../src/mpt/crypto/**.cpp",
		"../../src/mpt/crypto/**.hpp",
		"../../src/mpt/json/**.cpp",
		"../../src/mpt/json/**.hpp",
		"../../src/mpt/test/**.cpp",
		"../../src/mpt/test/**.hpp",
		"../../src/mpt/uuid_namespace/**.cpp",
		"../../src/mpt/uuid_namespace/**.hpp",
		"../../src/openmpt/sounddevice/**.cpp",
		"../../src/openmpt/sounddevice/**.hpp",
	}
	filter { "action:vs*" }
		resdefines {
			"MPT_BUILD_VER_SPECIAL_PREFIX=\"+small\"",
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
  defines { "LIBOPENMPT_BUILD", "LIBOPENMPT_BUILD_SMALL" }
  filter { "kind:SharedLib" }
   defines { "LIBOPENMPT_BUILD_DLL" }
  filter { "kind:SharedLib" }
  filter {}
  links {
   "minimp3",
   "miniz",
   "stb_vorbis"
  }
  prebuildcommands { "..\\..\\build\\svn_version\\update_svn_version_vs_premake.cmd $(IntDir)" }
