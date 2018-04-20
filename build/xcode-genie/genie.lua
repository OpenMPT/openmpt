
newoption {
 trigger     = "group",
 value       = "PROJECTS",
 description = "OpenMPT project group",
 allowed = {
  { "libopenmpt", "libopenmpt" },
 }
}


newoption {
 trigger     = "target",
 value       = "PROJECTS",
 description = "windows target platform",
 allowed = {
  { "macosx"  , "macosx"   },
  { "iphoneos", "iphoneos" },
 }
}


mpt_projectpathname = _ACTION .. "-" .. _OPTIONS["target"]
mpt_bindirsuffix = _OPTIONS["target"]

solution "libopenmpt"
 location ( "../../build/" .. mpt_projectpathname )
 configurations { "Debug", "Release" }

 dofile "../../build/xcode-genie/mpt-libopenmpt.lua"
 dofile "../../build/xcode-genie/ext-mpg123.lua"
 dofile "../../build/xcode-genie/ext-ogg.lua"
 dofile "../../build/xcode-genie/ext-vorbis.lua"
-- dofile "../../build/xcode-genie/ext-zlib.lua"

