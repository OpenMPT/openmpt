
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



if _ACTION == "vs2019" then
	if _OPTIONS["target"] == "windesktop81" then
		mpt_projectpathname = "vs2019win81"
		mpt_bindirsuffix = "win81"
	end
	if _OPTIONS["target"] == "winstore82" then
		mpt_projectpathname = "vs2019uwp"
		mpt_bindirsuffix = "uwp"
	end
end
if _ACTION == "vs2017" then
	if _OPTIONS["target"] == "windesktop81" then
		mpt_projectpathname = "vs2017win81"
		mpt_bindirsuffix = "win81"
	end
	if _OPTIONS["target"] == "winstore82" then
		mpt_projectpathname = "vs2017uwp"
		mpt_bindirsuffix = "uwp"
	end
end
--mpt_projectpathname = _OPTIONS["target"]
--mpt_bindirsuffix = _OPTIONS["target"]



solution "libopenmpt"
	location ( "../../build/" .. mpt_projectpathname )
	configurations { "Debug", "Release", "DebugShared", "ReleaseShared" }
	platforms { "x32", "x64", "ARM" }

	dofile "../../build/genie/mpt-libopenmpt.lua"
	dofile "../../build/genie/ext-mpg123.lua"
	dofile "../../build/genie/ext-ogg.lua"
	dofile "../../build/genie/ext-vorbis.lua"
	dofile "../../build/genie/ext-zlib.lua"

