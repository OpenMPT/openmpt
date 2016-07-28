  
 project "zlib"
  uuid "1654FB18-FDE6-406F-9D84-BA12BFBD67B2"
  language "C"
  location ( "../../build/" .. _ACTION .. "-ext" )
  objdir "../../build/obj/zlib"
  dofile "../../build/premake/premake-defaults-LIBorDLL.lua"
  dofile "../../build/premake/premake-defaults.lua"
  dofile "../../build/premake/premake-defaults-winver.lua"
  targetname "openmpt-zlib"
  includedirs { "../../include/zlib" }
  characterset "MBCS"
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
  filter { "kind:StaticLib" }
  filter { "kind:SharedLib" }
   defines { "ZLIB_DLL" }
  filter {}
