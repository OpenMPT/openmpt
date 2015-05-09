
-- premake gets a tiny bit confused if the same project appears in multiple
-- solutions in a single run. premake adds a bogus $projectname path to the
-- intermediate objects directory in that case. work-around using multiple
-- invocations of premake and a custom option to distinguish them.

MPT_PREMAKE_VERSION = ""

if _PREMAKE_VERSION == "4.3" then
 MPT_PREMAKE_VERSION = "4.3"
elseif _PREMAKE_VERSION == "4.4-beta5" then
 MPT_PREMAKE_VERSION = "4.4"
else
 print "Premake 4.3 or 4.4-beta5 required"
 os.exit(1)
end

newoption {
 trigger     = "group",
 value       = "PROJECTS",
 description = "OpenMPT project group",
 allowed = {
  { "libopenmpt-all", "libopenmpt-all" },
  { "libopenmpt_test", "libopenmpt_test" },
  { "libopenmpt", "libopenmpt" },
  { "in_openmpt", "in_openmpt" },
  { "xmp-openmpt", "xmp-openmpt" },
  { "openmpt123", "openmpt123" },
  { "PluginBridge", "PluginBridge" },
  { "OpenMPT", "OpenMPT" },
  { "all-externals", "all-externals" }
 }
}

function replace_in_file (filename, from, to)
	local text
	local infile
	local outfile
	infile = io.open(filename, "r")
	text = infile:read("*all")
	infile:close()
	text = string.gsub(text, from, to)
	outfile = io.open(filename, "w")
	outfile:write(text)
	outfile:close()
end

function postprocess_vs2008_mfc (filename)
if MPT_PREMAKE_VERSION == "4.3" then
	replace_in_file(filename, "UseOfMFC=\"2\"", "UseOfMFC=\"1\"")
end
end

function postprocess_vs2008_main (filename)
	replace_in_file(filename, "\t\t\t\tEntryPointSymbol=\"mainCRTStartup\"\n", "")
end

function postprocess_vs2008_dynamicbase (filename)
	replace_in_file(filename, "\t\t\t\tEnableCOMDATFolding=\"2\"", "\t\t\t\tEnableCOMDATFolding=\"2\"\n\t\t\t\tRandomizedBaseAddress=\"2\"")
end

function postprocess_vs2008_nonxcompat (filename)
	replace_in_file(filename, "\t\t\t<Tool\n\t\t\t\tName=\"VCLinkerTool\"\n", "\t\t\t<Tool\n\t\t\t\tName=\"VCLinkerTool\"\n\t\t\t\t\DataExecutionPrevention=\"1\"\n")
end

function postprocess_vs2008_largeaddress (filename)
	replace_in_file(filename, "\t\t\t<Tool\n\t\t\t\tName=\"VCLinkerTool\"\n", "\t\t\t<Tool\n\t\t\t\tName=\"VCLinkerTool\"\n\t\t\t\t\LargeAddressAware=\"2\"\n")
end

function postprocess_vs2010_mfc (filename)
if MPT_PREMAKE_VERSION == "4.3" then
	replace_in_file(filename, "<UseOfMfc>Dynamic</UseOfMfc>", "<UseOfMfc>Static</UseOfMfc>")
end
end

function postprocess_vs2010_main (filename)
	replace_in_file(filename, "<EntryPointSymbol>mainCRTStartup</EntryPointSymbol>", "")
end

function postprocess_vs2010_dynamicbase (filename)
if MPT_PREMAKE_VERSION == "4.3" then
	replace_in_file(filename, "<EnableCOMDATFolding>true</EnableCOMDATFolding>", "<EnableCOMDATFolding>true</EnableCOMDATFolding>\n\t\t\t<RandomizedBaseAddress>true</RandomizedBaseAddress>")
elseif MPT_PREMAKE_VERSION == "4.4" then
	replace_in_file(filename, "<EnableCOMDATFolding>true</EnableCOMDATFolding>", "<EnableCOMDATFolding>true</EnableCOMDATFolding>\n      <RandomizedBaseAddress>true</RandomizedBaseAddress>")
end
end

function postprocess_vs2010_nonxcompat (filename)
if MPT_PREMAKE_VERSION == "4.3" then
	replace_in_file(filename, "\t\t</Link>\n", "\t\t\t<DataExecutionPrevention>false</DataExecutionPrevention>\n\t\t</Link>\n")
elseif MPT_PREMAKE_VERSION == "4.4" then
	replace_in_file(filename, "    </Link>\n", "      <DataExecutionPrevention>false</DataExecutionPrevention>\n    </Link>\n")
end
end

function postprocess_vs2010_largeaddress (filename)
if MPT_PREMAKE_VERSION == "4.3" then
	replace_in_file(filename, "\t\t</Link>\n", "\t\t\t<LargeAddressAware>true</LargeAddressAware>\n\t\t</Link>\n")
elseif MPT_PREMAKE_VERSION == "4.4" then
	replace_in_file(filename, "    </Link>\n", "      <LargeAddressAware>true</LargeAddressAware>\n    </Link>\n")
end
end

function fixbug_vs2010_pch (filename)
if MPT_PREMAKE_VERSION == "4.3" then
	replace_in_file(filename, "</PrecompiledHeader>\n\t\t</ClCompile>", "</PrecompiledHeader>")
end
end

newaction {
 trigger     = "postprocess",
 description = "OpenMPT postprocess the project files to mitigate premake problems",
 execute     = function ()
  postprocess_vs2008_main("build/vs2008/libopenmpt_test.vcproj")
  postprocess_vs2008_main("build/vs2008/openmpt123.vcproj")
  postprocess_vs2008_main("build/vs2008/libopenmpt_example_c.vcproj")
  postprocess_vs2008_main("build/vs2008/libopenmpt_example_c_mem.vcproj")
  postprocess_vs2008_mfc("build/vs2008/OpenMPT.vcproj")
  postprocess_vs2008_dynamicbase("build/vs2008/OpenMPT.vcproj")
  postprocess_vs2008_nonxcompat("build/vs2008/OpenMPT.vcproj")
  postprocess_vs2008_largeaddress("build/vs2008/OpenMPT.vcproj")
  postprocess_vs2008_dynamicbase("build/vs2008/PluginBridge.vcproj")
  postprocess_vs2008_nonxcompat("build/vs2008/PluginBridge.vcproj")
  postprocess_vs2008_largeaddress("build/vs2008/PluginBridge.vcproj")
  postprocess_vs2010_main("build/vs2010/libopenmpt_test.vcxproj")
  postprocess_vs2010_main("build/vs2010/openmpt123.vcxproj")
  postprocess_vs2010_main("build/vs2010/OpenMPT.vcxproj")
  postprocess_vs2010_main("build/vs2010/libopenmpt_example_c.vcxproj")
  postprocess_vs2010_main("build/vs2010/libopenmpt_example_c_mem.vcxproj")
  postprocess_vs2010_main("build/vs2010/libopenmpt_example_cxx.vcxproj")
  postprocess_vs2010_mfc("build/vs2010/in_openmpt.vcxproj")
  postprocess_vs2010_mfc("build/vs2010/xmp-openmpt.vcxproj")
  postprocess_vs2010_mfc("build/vs2010/OpenMPT.vcxproj")
  postprocess_vs2010_dynamicbase("build/vs2010/OpenMPT.vcxproj")
  postprocess_vs2010_nonxcompat("build/vs2010/OpenMPT.vcxproj")
  postprocess_vs2010_largeaddress("build/vs2010/OpenMPT.vcxproj")
  postprocess_vs2010_dynamicbase("build/vs2010/PluginBridge.vcxproj")
  postprocess_vs2010_nonxcompat("build/vs2010/PluginBridge.vcxproj")
  postprocess_vs2010_largeaddress("build/vs2010/PluginBridge.vcxproj")
  fixbug_vs2010_pch("build/vs2010/OpenMPT.vcxproj")
 end
}

if _OPTIONS["group"] == "libopenmpt-all" then

solution "libopenmpt-all"
 location ( "../build/" .. _ACTION )
 configurations { "Debug", "Release" }
 platforms { "x32", "x64" }

 dofile "../build/premake4-win/mpt-libopenmpt_test.premake4.lua"
 dofile "../build/premake4-win/mpt-libopenmpt.premake4.lua"
 dofile "../build/premake4-win/mpt-libopenmpt_examples.premake4.lua"
 dofile "../build/premake4-win/mpt-libopenmptDLL.premake4.lua"
 dofile "../build/premake4-win/mpt-libopenmpt_modplug.premake4.lua"
 dofile "../build/premake4-win/mpt-in_openmpt.premake4.lua"
 dofile "../build/premake4-win/mpt-xmp-openmpt.premake4.lua"
 dofile "../build/premake4-win/mpt-openmpt123.premake4.lua"
 dofile "../build/premake4-win/ext-flac.premake4.lua"
 dofile "../build/premake4-win/ext-miniz.premake4.lua"
 dofile "../build/premake4-win/ext-portaudio.premake4.lua"

end

if _OPTIONS["group"] == "libopenmpt_test" then

solution "libopenmpt_test"
 location ( "../build/" .. _ACTION )
 configurations { "Debug", "Release" }
 platforms { "x32", "x64" }

 dofile "../build/premake4-win/mpt-libopenmpt_test.premake4.lua"
 dofile "../build/premake4-win/ext-miniz.premake4.lua"

end

if _OPTIONS["group"] == "in_openmpt" then

solution "in_openmpt"
 location ( "../build/" .. _ACTION )
 configurations { "Debug", "Release" }
 platforms { "x32" }

 dofile "../build/premake4-win/mpt-libopenmpt.premake4.lua"
 dofile "../build/premake4-win/mpt-in_openmpt.premake4.lua"
 dofile "../build/premake4-win/ext-miniz.premake4.lua"
 dofile "../build/premake4-win/ext-pugixml.premake4.lua"

end

if _OPTIONS["group"] == "xmp-openmpt" then

solution "xmp-openmpt"
 location ( "../build/" .. _ACTION )
 configurations { "Debug", "Release" }
 platforms { "x32" }

 dofile "../build/premake4-win/mpt-libopenmpt.premake4.lua"
 dofile "../build/premake4-win/mpt-xmp-openmpt.premake4.lua"
 dofile "../build/premake4-win/ext-miniz.premake4.lua"
 dofile "../build/premake4-win/ext-pugixml.premake4.lua"

end

-- should stay the last libopenmpt solution in order to overwrite the libopenmpt base project with all possible configurations
if _OPTIONS["group"] == "libopenmpt" then

solution "libopenmpt"
 location ( "../build/" .. _ACTION )
 configurations { "Debug", "Release" }
 platforms { "x32", "x64" }

 dofile "../build/premake4-win/mpt-libopenmpt.premake4.lua"
 dofile "../build/premake4-win/mpt-libopenmpt_examples.premake4.lua"
 dofile "../build/premake4-win/mpt-libopenmptDLL.premake4.lua"
 dofile "../build/premake4-win/mpt-libopenmpt_modplug.premake4.lua"
 dofile "../build/premake4-win/ext-miniz.premake4.lua"
 dofile "../build/premake4-win/ext-miniz-shared.premake4.lua"
 dofile "../build/premake4-win/ext-portaudio.premake4.lua"

end

if _OPTIONS["group"] == "openmpt123" then

solution "openmpt123"
 location ( "../build/" .. _ACTION )
 configurations { "Debug", "Release" }
 platforms { "x32", "x64" }

 dofile "../build/premake4-win/mpt-openmpt123.premake4.lua"
 dofile "../build/premake4-win/mpt-libopenmpt.premake4.lua"
 dofile "../build/premake4-win/ext-flac.premake4.lua"
 dofile "../build/premake4-win/ext-miniz.premake4.lua"
 dofile "../build/premake4-win/ext-ogg.premake4.lua"
 dofile "../build/premake4-win/ext-portaudio.premake4.lua"

end

if _OPTIONS["group"] == "PluginBridge" then

solution "PluginBridge"
 location ( "../build/" .. _ACTION )
 configurations { "Debug", "Release", "ReleaseNoLTCG" }
 platforms { "x32", "x64" }

 dofile "../build/premake4-win/mpt-PluginBridge.premake4.lua"

end


if _OPTIONS["group"] == "OpenMPT" then

solution "OpenMPT"
 location ( "../build/" .. _ACTION )
 configurations { "Debug", "Release", "ReleaseNoLTCG" }
 platforms { "x32", "x64" }

 dofile "../build/premake4-win/mpt-OpenMPT.premake4.lua"
 dofile "../build/premake4-win/mpt-PluginBridge.premake4.lua"
 dofile "../build/premake4-win/ext-flac.premake4.lua"
 dofile "../build/premake4-win/ext-lhasa.premake4.lua"
 dofile "../build/premake4-win/ext-minizip.premake4.lua"
 dofile "../build/premake4-win/ext-ogg.premake4.lua"
 dofile "../build/premake4-win/ext-portaudio.premake4.lua"
 dofile "../build/premake4-win/ext-portmidi.premake4.lua"
 dofile "../build/premake4-win/ext-r8brain.premake4.lua"
 dofile "../build/premake4-win/ext-smbPitchShift.premake4.lua"
 dofile "../build/premake4-win/ext-soundtouch.premake4.lua"
 dofile "../build/premake4-win/ext-UnRAR.premake4.lua"
 dofile "../build/premake4-win/ext-zlib.premake4.lua"

end

-- overwrite all external projects once again with the full matrix of possible build config combinations
if _OPTIONS["group"] == "all-externals" then

solution "all-externals"
 configurations { "Debug", "Release", "ReleaseNoLTCG" }
 platforms { "x32", "x64" }

 dofile "../build/premake4-win/ext-flac.premake4.lua"
 dofile "../build/premake4-win/ext-lhasa.premake4.lua"
 dofile "../build/premake4-win/ext-miniz.premake4.lua"
 dofile "../build/premake4-win/ext-miniz-shared.premake4.lua"
 dofile "../build/premake4-win/ext-minizip.premake4.lua"
 dofile "../build/premake4-win/ext-ogg.premake4.lua"
 dofile "../build/premake4-win/ext-portaudio.premake4.lua"
 dofile "../build/premake4-win/ext-portmidi.premake4.lua"
 dofile "../build/premake4-win/ext-pugixml.premake4.lua"
 dofile "../build/premake4-win/ext-r8brain.premake4.lua"
 dofile "../build/premake4-win/ext-smbPitchShift.premake4.lua"
 dofile "../build/premake4-win/ext-soundtouch.premake4.lua"
 dofile "../build/premake4-win/ext-UnRAR.premake4.lua"
 dofile "../build/premake4-win/ext-zlib.premake4.lua"

end
