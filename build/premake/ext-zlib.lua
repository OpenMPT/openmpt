  
 project "zlib"
  uuid "1654FB18-FDE6-406F-9D84-BA12BFBD67B2"
  language "C"
  location ( "%{wks.location}" .. "/ext" )
  mpt_kind "default"
  targetname "openmpt-zlib"
  includedirs { "../../include/zlib" }
	filter {}
  files {
   "../../include/zlib/adler32.c",
   "../../include/zlib/compress.c",
   "../../include/zlib/crc32.c",
   "../../include/zlib/deflate.c",
   "../../include/zlib/gzclose.c",
   "../../include/zlib/gzlib.c",
   "../../include/zlib/gzread.c",
   "../../include/zlib/gzwrite.c",
   "../../include/zlib/infback.c",
   "../../include/zlib/inffast.c",
   "../../include/zlib/inflate.c",
   "../../include/zlib/inftrees.c",
   "../../include/zlib/trees.c",
   "../../include/zlib/uncompr.c",
   "../../include/zlib/zutil.c",
  }
  files {
   "../../include/zlib/crc32.h",
   "../../include/zlib/deflate.h",
   "../../include/zlib/gzguts.h",
   "../../include/zlib/inffast.h",
   "../../include/zlib/inffixed.h",
   "../../include/zlib/inflate.h",
   "../../include/zlib/inftrees.h",
   "../../include/zlib/trees.h",
   "../../include/zlib/zconf.h",
   "../../include/zlib/zlib.h",
   "../../include/zlib/zutil.h",
  }
	filter {}
	if MPT_OS_WINDOWS then
		filter {}
		filter { "kind:StaticLib" }
		filter { "kind:SharedLib" }
			defines { "ZLIB_DLL" }
		filter {}
	end
	filter {}
	if MPT_COMPILER_MSVC or MPT_COMPILER_CLANGCL then
		buildoptions { "/wd4267" }
		buildoptions { "/wd6297", "/wd6385" } -- /analyze
	end
	filter {}
	if MPT_COMPILER_CLANGCL or MPT_COMPILER_CLANG then
		buildoptions {
			"-Wno-deprecated-non-prototype",
			"-Wno-tautological-pointer-compare",
			"-Wno-unused-but-set-variable",
			"-Wno-unused-const-variable",
		}
	end
	filter {}

function mpt_use_zlib ()
	filter {}
	dependencyincludedirs {
		"../../include/zlib",
	}
	filter {}
	filter { "configurations:*Shared" }
		defines { "ZLIB_DLL" }
	filter { "not configurations:*Shared" }
	filter {}
	links {
		"zlib",
	}
	filter {}
end
