 
 project "libopenmpt"
  uuid "9C5101EF-3E20-4558-809B-277FDD50E878"
  language "C++"
  vpaths { ["*"] = "../../" }
  mpt_kind "default"
	
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
   "../../libopenmpt/*.cpp",
   "../../libopenmpt/*.hpp",
   "../../libopenmpt/*.h",
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
	filter { "action:vs*", "kind:SharedLib or ConsoleApp or WindowedApp" }
		resdefines {
			"MPT_BUILD_VER_FILENAME=\"" .. "libopenmpt" .. ".dll\"",
			"MPT_BUILD_VER_FILEDESC=\"" .. "libopenmpt" .. "\"",
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

function mpt_use_libopenmpt ()
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
		"libopenmpt",
	}
	filter {}
end
