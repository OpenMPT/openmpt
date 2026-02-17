
include_dependency "ext-zlib.lua"

 project "minizip"
  uuid "63AF9025-A6CE-4147-A05D-6E2CCFD3A0D7"
  language "C"
  location ( "%{wks.location}" .. "/ext" )
  mpt_kind "default"
  targetname "openmpt-minizip"
	
	mpt_use_zlib()
	
  includedirs {
		"../../include/zlib/contrib/minizip"
	}
	filter {}
  files {
   "../../include/zlib/contrib/minizip/ioapi.c",
   "../../include/zlib/contrib/minizip/iowin32.c",
   "../../include/zlib/contrib/minizip/mztools.c",
   "../../include/zlib/contrib/minizip/unzip.c",
   "../../include/zlib/contrib/minizip/zip.c",
  }
  files {
   "../../include/zlib/contrib/minizip/crypt.h",
   "../../include/zlib/contrib/minizip/ints.h",
   "../../include/zlib/contrib/minizip/ioapi.h",
   "../../include/zlib/contrib/minizip/iowin32.h",
   "../../include/zlib/contrib/minizip/mztools.h",
   "../../include/zlib/contrib/minizip/skipset.h",
   "../../include/zlib/contrib/minizip/unzip.h",
   "../../include/zlib/contrib/minizip/zip.h",
  }
  filter {}
  filter { "kind:SharedLib" }
   files { "../../build/premake/def/ext-minizip.def" }
	filter {}
	if MPT_COMPILER_MSVC or MPT_COMPILER_CLANGCL then
		buildoptions { "/wd6262" } -- analyze
	end
	filter {}
	if MPT_COMPILER_CLANGCL or MPT_COMPILER_CLANG then
		buildoptions {
			"-Wno-deprecated-non-prototype",
			"-Wno-unused-but-set-variable",
			"-Wno-unused-variable",
		}
	end
	filter {}

function mpt_use_minizip ()
	filter {}
	dependencyincludedirs {
		"../../include/zlib",
	}
	filter {}
	links {
		"minizip",
	}
	filter {}
end
