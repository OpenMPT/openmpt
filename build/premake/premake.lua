
-- premake gets a tiny bit confused if the same project appears in multiple
-- solutions in a single run. premake adds a bogus $projectname path to the
-- intermediate objects directory in that case. work-around using multiple
-- invocations of premake and a custom option to distinguish them.

MPT_PREMAKE_VERSION = ""

if _PREMAKE_VERSION == "5.0.0-alpha7" then
 MPT_PREMAKE_VERSION = "5.0"
else
 print "Premake 5.0.0-alpha7 required"
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
  { "foo_openmpt", "foo_openmpt" },
  { "in_openmpt", "in_openmpt" },
  { "xmp-openmpt", "xmp-openmpt" },
  { "openmpt123", "openmpt123" },
  { "PluginBridge", "PluginBridge" },
  { "OpenMPT-VSTi", "OpenMPT-VSTi" },
  { "OpenMPT", "OpenMPT" },
  { "all-externals", "all-externals" }
 }
}

function replace_in_file (filename, from, to)
	local text
	local infile
	local outfile
	local oldtext
	local newtext
	infile = io.open(filename, "r")
	text = infile:read("*all")
	infile:close()
	oldtext = text
	newtext = string.gsub(oldtext, from, to)
	text = newtext
	if newtext == oldtext then
   print("Failed to postprocess '" .. filename .. "': " .. from .. " -> " .. to)
   os.exit(1)
	end
	outfile = io.open(filename, "w")
	outfile:write(text)
	outfile:close()
end

function postprocess_vs2008_main (filename)
	replace_in_file(filename, "\t\t\t\tEntryPointSymbol=\"mainCRTStartup\"\n", "")
end

function postprocess_vs2008_nonxcompat (filename)
	replace_in_file(filename, "\t\t\t<Tool\n\t\t\t\tName=\"VCLinkerTool\"\n", "\t\t\t<Tool\n\t\t\t\tName=\"VCLinkerTool\"\n\t\t\t\t\DataExecutionPrevention=\"1\"\n")
end

function postprocess_vs2008_largeaddress (filename)
	replace_in_file(filename, "\t\t\t<Tool\n\t\t\t\tName=\"VCLinkerTool\"\n", "\t\t\t<Tool\n\t\t\t\tName=\"VCLinkerTool\"\n\t\t\t\t\LargeAddressAware=\"2\"\n")
end

function postprocess_vs2010_main (filename)
	replace_in_file(filename, "<EntryPointSymbol>mainCRTStartup</EntryPointSymbol>", "")
end

function postprocess_vs2010_nonxcompat (filename)
	replace_in_file(filename, "    </Link>\n", "      <DataExecutionPrevention>false</DataExecutionPrevention>\n    </Link>\n")
end

function postprocess_vs2010_disabledpiaware (filename)
	replace_in_file(filename, "%%%(AdditionalManifestFiles%)</AdditionalManifestFiles>\n", "%%%(AdditionalManifestFiles%)</AdditionalManifestFiles>\n      <EnableDPIAwareness>false</EnableDPIAwareness>\n")
end

newaction {
 trigger     = "postprocess",
 description = "OpenMPT postprocess the project files to mitigate premake problems",
 execute     = function ()

  postprocess_vs2008_main("build/vs2008/libopenmpt_test.vcproj")
  postprocess_vs2008_main("build/vs2008/openmpt123.vcproj")
  postprocess_vs2008_main("build/vs2008/libopenmpt_example_c.vcproj")
  postprocess_vs2008_main("build/vs2008/libopenmpt_example_c_mem.vcproj")
  postprocess_vs2008_main("build/vs2008/libopenmpt_example_c_unsafe.vcproj")
  postprocess_vs2008_nonxcompat("build/vs2008/OpenMPT.vcproj")
  postprocess_vs2008_largeaddress("build/vs2008/OpenMPT.vcproj")
  postprocess_vs2008_nonxcompat("build/vs2008/PluginBridge.vcproj")
  postprocess_vs2008_largeaddress("build/vs2008/PluginBridge.vcproj")

  postprocess_vs2010_main("build/vs2010/libopenmpt_test.vcxproj")
  postprocess_vs2010_main("build/vs2010/openmpt123.vcxproj")
  postprocess_vs2010_main("build/vs2010/libopenmpt_example_c.vcxproj")
  postprocess_vs2010_main("build/vs2010/libopenmpt_example_c_mem.vcxproj")
  postprocess_vs2010_main("build/vs2010/libopenmpt_example_c_unsafe.vcxproj")
  postprocess_vs2010_main("build/vs2010/libopenmpt_example_cxx.vcxproj")
  postprocess_vs2010_nonxcompat("build/vs2010/OpenMPT.vcxproj")
  postprocess_vs2010_disabledpiaware("build/vs2010/OpenMPT.vcxproj")
  postprocess_vs2010_nonxcompat("build/vs2010/PluginBridge.vcxproj")
  postprocess_vs2010_disabledpiaware("build/vs2010/PluginBridge.vcxproj")
  
  postprocess_vs2010_main("build/vs2012/libopenmpt_test.vcxproj")
  postprocess_vs2010_main("build/vs2012/openmpt123.vcxproj")
  postprocess_vs2010_main("build/vs2012/libopenmpt_example_c.vcxproj")
  postprocess_vs2010_main("build/vs2012/libopenmpt_example_c_mem.vcxproj")
  postprocess_vs2010_main("build/vs2012/libopenmpt_example_c_unsafe.vcxproj")
  postprocess_vs2010_main("build/vs2012/libopenmpt_example_cxx.vcxproj")
  postprocess_vs2010_nonxcompat("build/vs2012/OpenMPT.vcxproj")
  postprocess_vs2010_disabledpiaware("build/vs2012/OpenMPT.vcxproj")
  postprocess_vs2010_nonxcompat("build/vs2012/PluginBridge.vcxproj")
  postprocess_vs2010_disabledpiaware("build/vs2012/PluginBridge.vcxproj")
	
  postprocess_vs2010_main("build/vs2013/libopenmpt_test.vcxproj")
  postprocess_vs2010_main("build/vs2013/openmpt123.vcxproj")
  postprocess_vs2010_main("build/vs2013/libopenmpt_example_c.vcxproj")
  postprocess_vs2010_main("build/vs2013/libopenmpt_example_c_mem.vcxproj")
  postprocess_vs2010_main("build/vs2013/libopenmpt_example_c_unsafe.vcxproj")
  postprocess_vs2010_main("build/vs2013/libopenmpt_example_cxx.vcxproj")
  postprocess_vs2010_nonxcompat("build/vs2013/OpenMPT.vcxproj")
  postprocess_vs2010_disabledpiaware("build/vs2013/OpenMPT.vcxproj")
  postprocess_vs2010_nonxcompat("build/vs2013/PluginBridge.vcxproj")
  postprocess_vs2010_disabledpiaware("build/vs2013/PluginBridge.vcxproj")

  postprocess_vs2010_main("build/vs2015/libopenmpt_test.vcxproj")
  postprocess_vs2010_main("build/vs2015/openmpt123.vcxproj")
  postprocess_vs2010_main("build/vs2015/libopenmpt_example_c.vcxproj")
  postprocess_vs2010_main("build/vs2015/libopenmpt_example_c_mem.vcxproj")
  postprocess_vs2010_main("build/vs2015/libopenmpt_example_c_unsafe.vcxproj")
  postprocess_vs2010_main("build/vs2015/libopenmpt_example_cxx.vcxproj")
  postprocess_vs2010_nonxcompat("build/vs2015/OpenMPT.vcxproj")
  postprocess_vs2010_disabledpiaware("build/vs2015/OpenMPT.vcxproj")
  postprocess_vs2010_nonxcompat("build/vs2015/PluginBridge.vcxproj")
  postprocess_vs2010_disabledpiaware("build/vs2015/PluginBridge.vcxproj")

 end
}

if _OPTIONS["group"] == "libopenmpt-all" then

solution "libopenmpt-all"
 location ( "../../build/" .. _ACTION )
 configurations { "Debug", "Release" }
 platforms { "x86", "x86_64" }

 dofile "../../build/premake/mpt-libopenmpt_test.lua"
 dofile "../../build/premake/mpt-libopenmpt.lua"
 dofile "../../build/premake/mpt-libopenmpt_examples.lua"
 dofile "../../build/premake/mpt-libopenmptDLL.lua"
 dofile "../../build/premake/mpt-libopenmpt_modplug.lua"
 dofile "../../build/premake/mpt-foo_openmpt.lua"
 dofile "../../build/premake/mpt-in_openmpt.lua"
 dofile "../../build/premake/mpt-xmp-openmpt.lua"
 dofile "../../build/premake/mpt-openmpt123.lua"
 dofile "../../build/premake/ext-flac.lua"
 dofile "../../build/premake/ext-miniz.lua"
 dofile "../../build/premake/ext-portaudio.lua"
 dofile "../../build/premake/ext-portaudiocpp.lua"

end

if _OPTIONS["group"] == "libopenmpt_test" then

solution "libopenmpt_test"
 location ( "../../build/" .. _ACTION )
 configurations { "Debug", "Release" }
 platforms { "x86", "x86_64" }

 dofile "../../build/premake/mpt-libopenmpt_test.lua"
 dofile "../../build/premake/ext-miniz.lua"

end

if _OPTIONS["group"] == "foo_openmpt" then

solution "foo_openmpt"
 location ( "../../build/" .. _ACTION )
 configurations { "Debug", "Release" }
 platforms { "x86" }

 dofile "../../build/premake/mpt-foo_openmpt.lua"
 dofile "../../build/premake/mpt-libopenmpt.lua"
 dofile "../../build/premake/ext-miniz.lua"

end

if _OPTIONS["group"] == "in_openmpt" then

solution "in_openmpt"
 location ( "../../build/" .. _ACTION )
 configurations { "Debug", "Release" }
 platforms { "x86" }

 dofile "../../build/premake/mpt-in_openmpt.lua"
 dofile "../../build/premake/mpt-libopenmpt.lua"
 dofile "../../build/premake/ext-miniz.lua"

end

if _OPTIONS["group"] == "xmp-openmpt" then

solution "xmp-openmpt"
 location ( "../../build/" .. _ACTION )
 configurations { "Debug", "Release" }
 platforms { "x86" }

 dofile "../../build/premake/mpt-xmp-openmpt.lua"
 dofile "../../build/premake/mpt-libopenmpt.lua"
 dofile "../../build/premake/ext-miniz.lua"
 dofile "../../build/premake/ext-pugixml.lua"

end

-- should stay the last libopenmpt solution in order to overwrite the libopenmpt base project with all possible configurations
if _OPTIONS["group"] == "libopenmpt" then

solution "libopenmpt"
 location ( "../../build/" .. _ACTION )
 configurations { "Debug", "Release" }
 platforms { "x86", "x86_64" }

 dofile "../../build/premake/mpt-libopenmpt.lua"
 dofile "../../build/premake/mpt-libopenmpt_examples.lua"
 dofile "../../build/premake/mpt-libopenmptDLL.lua"
 dofile "../../build/premake/mpt-libopenmpt_modplug.lua"
 dofile "../../build/premake/ext-miniz.lua"
 dofile "../../build/premake/ext-miniz-shared.lua"
 dofile "../../build/premake/ext-portaudio.lua"
 dofile "../../build/premake/ext-portaudiocpp.lua"

end

if _OPTIONS["group"] == "openmpt123" then

solution "openmpt123"
 location ( "../../build/" .. _ACTION )
 configurations { "Debug", "Release" }
 platforms { "x86", "x86_64" }

 dofile "../../build/premake/mpt-openmpt123.lua"
 dofile "../../build/premake/mpt-libopenmpt.lua"
 dofile "../../build/premake/ext-flac.lua"
 dofile "../../build/premake/ext-miniz.lua"
 dofile "../../build/premake/ext-ogg.lua"
 dofile "../../build/premake/ext-portaudio.lua"

end

if _OPTIONS["group"] == "PluginBridge" then

solution "PluginBridge"
 location ( "../../build/" .. _ACTION )
 configurations { "Debug", "Release", "DebugMDd", "ReleaseLTCG" }
 platforms { "x86", "x86_64" }

 dofile "../../build/premake/mpt-PluginBridge.lua"

end

if 1 == 0 then
-- disabled
if _OPTIONS["group"] == "OpenMPT-VSTi" then

solution "OpenMPT-VSTi"
 location ( "../../build/" .. _ACTION )
 configurations { "Debug", "Release" }
 platforms { "x86", "x86_64" }
 
 dofile "../../build/premake/mpt-OpenMPT-VSTi.lua"
 dofile "../../build/premake/ext-flac.lua"
 dofile "../../build/premake/ext-lhasa.lua"
 dofile "../../build/premake/ext-minizip.lua"
 dofile "../../build/premake/ext-ogg.lua"
 dofile "../../build/premake/ext-portaudio.lua"
 dofile "../../build/premake/ext-portmidi.lua"
 dofile "../../build/premake/ext-r8brain.lua"
 dofile "../../build/premake/ext-smbPitchShift.lua"
 dofile "../../build/premake/ext-soundtouch.lua"
 dofile "../../build/premake/ext-stb_vorbis.lua"
 dofile "../../build/premake/ext-UnRAR.lua"
 dofile "../../build/premake/ext-zlib.lua"

end
end

if _OPTIONS["group"] == "OpenMPT" then

solution "OpenMPT"
 location ( "../../build/" .. _ACTION )
 configurations { "Debug", "Release", "DebugMDd", "ReleaseLTCG" }
 platforms { "x86", "x86_64" }
 
 dofile "../../build/premake/mpt-OpenMPT.lua"
 dofile "../../build/premake/mpt-PluginBridge.lua"
 dofile "../../build/premake/ext-flac.lua"
 dofile "../../build/premake/ext-lhasa.lua"
 dofile "../../build/premake/ext-minizip.lua"
 dofile "../../build/premake/ext-ogg.lua"
 dofile "../../build/premake/ext-portaudio.lua"
 dofile "../../build/premake/ext-portmidi.lua"
 dofile "../../build/premake/ext-r8brain.lua"
 dofile "../../build/premake/ext-smbPitchShift.lua"
 dofile "../../build/premake/ext-soundtouch.lua"
 dofile "../../build/premake/ext-stb_vorbis.lua"
 dofile "../../build/premake/ext-UnRAR.lua"
 dofile "../../build/premake/ext-zlib.lua"

end

-- overwrite all external projects once again with the full matrix of possible build config combinations
if _OPTIONS["group"] == "all-externals" then

solution "all-externals"
 location ( "../../build/" .. _ACTION .. "-ext" )
 configurations { "Debug", "Release", "DebugMDd", "ReleaseLTCG" }
 platforms { "x86", "x86_64" }

 dofile "../../build/premake/ext-flac.lua"
 dofile "../../build/premake/ext-lhasa.lua"
 dofile "../../build/premake/ext-miniz.lua"
 dofile "../../build/premake/ext-miniz-shared.lua"
 dofile "../../build/premake/ext-minizip.lua"
 dofile "../../build/premake/ext-ogg.lua"
 dofile "../../build/premake/ext-portaudio.lua"
 dofile "../../build/premake/ext-portmidi.lua"
 dofile "../../build/premake/ext-pugixml.lua"
 dofile "../../build/premake/ext-r8brain.lua"
 dofile "../../build/premake/ext-smbPitchShift.lua"
 dofile "../../build/premake/ext-soundtouch.lua"
 dofile "../../build/premake/ext-stb_vorbis.lua"
 dofile "../../build/premake/ext-UnRAR.lua"
 dofile "../../build/premake/ext-zlib.lua"

end
