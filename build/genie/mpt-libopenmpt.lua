 
 project "libopenmpt"
  uuid "9C5101EF-3E20-4558-809B-277FDD50E878"
  language "C++"
  location ( "../../build/" .. mpt_projectpathname )
  vpaths { ["*"] = "../../" }
  mpt_projectname = "libopenmpt"
  dofile "../../build/genie/genie-defaults-LIBorDLL.lua"
  dofile "../../build/genie/genie-defaults.lua"
  local extincludedirs = {
   "../../include/mpg123/ports/MSVC++",
   "../../include/mpg123/src/libmpg123",
   "../../include/ogg/include",
   "../../include/vorbis/include",
   "../../include/zlib",
  }
  includedirs ( extincludedirs )
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
	resdefines {
		"MPT_BUILD_VER_FILENAME=\"" .. mpt_projectname .. ".dll\"",
		"MPT_BUILD_VER_FILEDESC=\"" .. mpt_projectname .. "\"",
	}
	configuration "DebugShared or ReleaseShared"
		defines { "LIBOPENMPT_BUILD_DLL" }
		resincludedirs {
			"$(IntDir)/svn_version",
			"../../build/svn_version",
			"$(ProjDir)/../../build/svn_version",
		}
		files {
			"../../libopenmpt/libopenmpt_version.rc",
		}
		resdefines { "MPT_BUILD_VER_DLL" }
	configuration {}

  flags { "Unicode" }
  flags { "ExtraWarnings" }
  defines { "LIBOPENMPT_BUILD" }
  links {
   "mpg123",
   "vorbis",
   "ogg",
   "zlib",
  }
  prebuildcommands { "..\\..\\build\\svn_version\\update_svn_version_vs_premake.cmd $(IntDir)" }
