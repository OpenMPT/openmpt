
 project "minizip"
  uuid "63AF9025-A6CE-4147-A05D-6E2CCFD3A0D7"
  language "C"
  location ( "../../build/" .. mpt_projectpathname .. "/ext" )
  mpt_projectname = "minizip"
  dofile "../../build/premake/premake-defaults-LIBorDLL.lua"
  dofile "../../build/premake/premake-defaults.lua"
  dofile "../../build/premake/premake-defaults-winver.lua"
  targetname "openmpt-minizip"
  local extincludedirs = {
		"../../include/zlib",
	}
	filter { "action:vs*" }
		includedirs ( extincludedirs )
	filter { "not action:vs*" }
		sysincludedirs ( extincludedirs )
	filter {}
  includedirs {
		"../../include/zlib/contrib/minizip"
	}
	filter {}
	filter { "action:vs*" }
		characterset "Unicode"
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
   "../../include/zlib/contrib/minizip/ioapi.h",
   "../../include/zlib/contrib/minizip/iowin32.h",
   "../../include/zlib/contrib/minizip/mztools.h",
   "../../include/zlib/contrib/minizip/unzip.h",
   "../../include/zlib/contrib/minizip/zip.h",
  }
  links {
   "zlib"
  }
  filter {}
  filter { "kind:StaticLib" }
  filter { "kind:SharedLib" }
   defines { "ZLIB_DLL" }
  filter {}
  filter { "kind:SharedLib" }
   files { "../../build/premake/def/ext-minizip.def" }
  filter {}
