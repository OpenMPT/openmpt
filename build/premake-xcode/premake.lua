

newoption {
 trigger     = "target",
 value       = "PROJECTS",
 description = "Apple target platform",
 allowed = {
  { "macosx", "macosx" },
  { "ios"   , "ios"    },
 }
}


mpt_projectpathname = "xcode" .. "-" .. _OPTIONS["target"]
mpt_bindirsuffix = _OPTIONS["target"]

solution "libopenmpt"
 location ( "../../build/" .. mpt_projectpathname )
 configurations { "Debug", "Release" }

 dofile "../../build/premake-xcode/mpt-libopenmpt.lua"
 dofile "../../build/premake-xcode/ext-mpg123.lua"
 dofile "../../build/premake-xcode/ext-ogg.lua"
 dofile "../../build/premake-xcode/ext-vorbis.lua"
-- dofile "../../build/premake-xcode/ext-zlib.lua"

