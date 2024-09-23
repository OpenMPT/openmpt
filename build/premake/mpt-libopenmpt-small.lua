 
 project "libopenmpt-small"
  uuid "25560abd-41fc-444c-9e71-f8502bc7ee96"
  language "C++"
  vpaths { ["*"] = "../../" }
  mpt_kind "default"
	
	mpt_use_minimp3()
	mpt_use_miniz()
	mpt_use_stbvorbis()
	
	defines {
		"MPT_WITH_MINIMP3",
		"MPT_WITH_MINIZ",
		"MPT_WITH_STBVORBIS",
	}
	
  includedirs {
   "../..",
   "../../src",
   "../../common",
   "$(IntDir)/svn_version",
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
   "../../libopenmpt/libopenmpt_stream_callbacks_file_mingw.h",
   "../../libopenmpt/libopenmpt_stream_callbacks_file_msvcrt.h",
   "../../libopenmpt/libopenmpt_stream_callbacks_file_posix.h",
   "../../libopenmpt/libopenmpt_stream_callbacks_file_posix_lfs64.h",
   "../../libopenmpt/libopenmpt_version.h",
   "../../libopenmpt/libopenmpt_c.cpp",
   "../../libopenmpt/libopenmpt_cxx.cpp",
   "../../libopenmpt/libopenmpt_ext_impl.cpp",
   "../../libopenmpt/libopenmpt_impl.cpp",
  }
	excludes {
		"../../src/mpt/crypto/**.cpp",
		"../../src/mpt/crypto/**.hpp",
		"../../src/mpt/fs/**.cpp",
		"../../src/mpt/fs/**.hpp",
		"../../src/mpt/json/**.cpp",
		"../../src/mpt/json/**.hpp",
		"../../src/mpt/library/**.cpp",
		"../../src/mpt/library/**.hpp",
		"../../src/mpt/main/**.cpp",
		"../../src/mpt/main/**.hpp",
		"../../src/mpt/test/**.cpp",
		"../../src/mpt/test/**.hpp",
		"../../src/mpt/uuid_namespace/**.cpp",
		"../../src/mpt/uuid_namespace/**.hpp",
		"../../src/openmpt/sounddevice/**.cpp",
		"../../src/openmpt/sounddevice/**.hpp",
		"../../src/openmpt/soundfile_write/**.cpp",
		"../../src/openmpt/soundfile_write/**.hpp",
		"../../src/openmpt/streamencoder/**.cpp",
		"../../src/openmpt/streamencoder/**.hpp",
	}
	filter { "action:vs*" }
		resdefines {
			"MPT_BUILD_VER_SPECIAL_PREFIX=\"+small\"",
		}
	filter { "action:vs*", "kind:SharedLib or ConsoleApp or WindowedApp" }
		resdefines {
			"MPT_BUILD_VER_FILENAME=\"" .. "libopenmpt-small" .. ".dll\"",
			"MPT_BUILD_VER_FILEDESC=\"" .. "libopenmpt-small" .. "\"",
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

	if _OPTIONS["charset"] ~= "Unicode" then
		defines { "MPT_CHECK_WINDOWS_IGNORE_WARNING_NO_UNICODE" }
	end

  warnings "Extra"
  defines { "LIBOPENMPT_BUILD" }
  filter { "kind:SharedLib" }
   defines { "LIBOPENMPT_BUILD_DLL" }
  filter { "kind:SharedLib" }
  filter {}
  prebuildcommands { "..\\..\\build\\svn_version\\update_svn_version_vs_premake.cmd $(IntDir)" }

function mpt_use_libopenmpt_small ()
	filter {}
	filter { "action:vs*" }
		includedirs {
			"../..",
		}
	filter { "not action:vs*" }
		externalincludedirs {
			"../..",
		}
	filter {}
	filter { "configurations:*Shared" }
		defines { "LIBOPENMPT_USE_DLL" }
	filter { "not configurations:*Shared" }
	filter {}
	links {
		"libopenmpt-small",
	}
	filter {}
end
