
-- genie gets a tiny bit confused if the same project appears in multiple
-- solutions in a single run. genie adds a bogus $projectname path to the
-- intermediate objects directory in that case. work-around using multiple
-- invocations of genie and a custom option to distinguish them.

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
  { "windesktop81", "windesktop81" },
  { "winphone8"   , "winphone8"  },
  { "winphone81"  , "winphone81" },
  { "winstore81"  , "winstore81" },
  { "winstore82"  , "winstore82" },
 }
}


mpt_projectpathname = _OPTIONS["target"]
mpt_bindirsuffix = _OPTIONS["target"]

solution "libopenmpt"
 location ( "../../build/" .. mpt_projectpathname )
 configurations { "Debug", "Release", "DebugShared", "ReleaseShared" }
	if _OPTIONS["target"] == "windesktop" then
		platforms { "x32", "x64" }
	else
		platforms { "x32", "x64", "ARM" }
	end

 dofile "../../build/genie/mpt-libopenmpt.lua"
 dofile "../../build/genie/ext-ogg.lua"
 dofile "../../build/genie/ext-vorbis.lua"
 dofile "../../build/genie/ext-zlib.lua"

