
 project "libopenmpt_test"
  uuid "0A313F63-131E-46A0-931D-23C3A3D488F2"
  language "C++"
  vpaths { ["*"] = "../../" }
  mpt_kind "Console"
	
	mpt_use_mpg123()
	mpt_use_ogg()
	mpt_use_vorbis()
	mpt_use_zlib()
	
	defines {
		"MPT_WITH_MPG123",
		"MPT_WITH_OGG",
		"MPT_WITH_VORBIS",
		"MPT_WITH_VORBISFILE",
		"MPT_WITH_ZLIB",
	}
	
	files {
		"../../libopenmpt/libopenmpt_test/libopenmpt_test.manifest",
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
   "../../test/*.cpp",
   "../../test/*.h",
   "../../unarchiver/archive.h",
   "../../unarchiver/ungzip.cpp",
   "../../unarchiver/ungzip.h",
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
   "../../libopenmpt/libopenmpt_test/libopenmpt_test.cpp",
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
		"../../src/mpt/uuid_namespace/**.cpp",
		"../../src/mpt/uuid_namespace/**.hpp",
		"../../test/mpt_tests_crypto.cpp",
		"../../test/mpt_tests_uuid_namespace.cpp",
		"../../src/openmpt/sounddevice/**.cpp",
		"../../src/openmpt/sounddevice/**.hpp",
		"../../src/openmpt/streamencoder/**.cpp",
		"../../src/openmpt/streamencoder/**.hpp",
	}

	if _OPTIONS["charset"] ~= "Unicode" then
		defines { "MPT_CHECK_WINDOWS_IGNORE_WARNING_NO_UNICODE" }
	end

  warnings "Extra"
  defines { "LIBOPENMPT_BUILD", "LIBOPENMPT_BUILD_TEST" }
	links {
		"ole32.lib",
		"rpcrt4.lib",
	}
  filter {}
  prebuildcommands { "..\\..\\build\\svn_version\\update_svn_version_vs_premake.cmd $(IntDir)" }
