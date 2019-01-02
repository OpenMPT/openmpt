
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



if _OPTIONS["target"] == "windesktop81" then
	mpt_projectpathname = "vs2017win81"
	mpt_bindirsuffix = "vs2017win81"
end
if _OPTIONS["target"] == "winstore82" then
	mpt_projectpathname = "vs2017uwp"
	mpt_bindirsuffix = "vs2017uwp"
end
--mpt_projectpathname = _OPTIONS["target"]
--mpt_bindirsuffix = _OPTIONS["target"]



function remove_pattern_in_file (filename, pattern)
	local outfile
  oldlines = {}
	outfile = io.open(filename .. ".new", "wb")
  for line in io.lines(filename) do 
		if string.find(line, pattern) then
			outfile:write('    <IntDir>obj\\$(PlatformName)\\$(Configuration)\\$(ProjectName)\\</IntDir>' .. "\r\n")
		else
			outfile:write(line .. "\r\n")
		end
  end
	outfile:close()
	os.remove(filename)
	os.rename(filename .. ".new", filename)
end

function postprocess_vs2017_nointdir (filename)
	remove_pattern_in_file(filename, "<IntDir>")
end

newaction {
 trigger     = "postprocess",
 description = "OpenMPT postprocess the project files to mitigate premake problems",
 execute     = function ()

  postprocess_vs2017_nointdir("build/" .. mpt_projectpathname .. "/libopenmpt.vcxproj")
  postprocess_vs2017_nointdir("build/" .. mpt_projectpathname .. "/ext/mpg123.vcxproj")
  postprocess_vs2017_nointdir("build/" .. mpt_projectpathname .. "/ext/ogg.vcxproj")
  postprocess_vs2017_nointdir("build/" .. mpt_projectpathname .. "/ext/vorbis.vcxproj")
  postprocess_vs2017_nointdir("build/" .. mpt_projectpathname .. "/ext/zlib.vcxproj")

 end
}



solution "libopenmpt"
	location ( "../../build/" .. mpt_projectpathname )
	configurations { "Debug", "Release", "DebugShared", "ReleaseShared" }
	platforms { "x32", "x64", "ARM" }

	dofile "../../build/genie/mpt-libopenmpt.lua"
	dofile "../../build/genie/ext-mpg123.lua"
	dofile "../../build/genie/ext-ogg.lua"
	dofile "../../build/genie/ext-vorbis.lua"
	dofile "../../build/genie/ext-zlib.lua"

