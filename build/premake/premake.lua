
-- premake gets a tiny bit confused if the same project appears in multiple
-- solutions in a single run. premake adds a bogus $projectname path to the
-- intermediate objects directory in that case. work-around using multiple
-- invocations of premake and a custom option to distinguish them.

MPT_PREMAKE_VERSION = ""

MPT_PREMAKE_VERSION = "5.0"

newoption {
 trigger     = "group",
 value       = "PROJECTS",
 description = "OpenMPT project group",
 allowed = {
  { "libopenmpt-all", "libopenmpt-all" },
  { "libopenmpt_test", "libopenmpt_test" },
  { "libopenmpt", "libopenmpt" },
  { "libopenmpt-small", "libopenmpt-small" },
  { "in_openmpt", "in_openmpt" },
  { "xmp-openmpt", "xmp-openmpt" },
  { "openmpt123", "openmpt123" },
  { "PluginBridge", "PluginBridge" },
  { "OpenMPT", "OpenMPT" },
  { "all-externals", "all-externals" }
 }
}

newoption {
	trigger = "xp",
	description = "Generate XP targetting projects",
}

newoption {
	trigger = "win10",
	description = "Generate Windows 10 Desktop targetting projects",
}

if _OPTIONS["win10"] then
	allplatforms = { "x86", "x86_64", "arm", "arm64" }
	trkplatforms = { "x86", "x86_64", "arm", "arm64" }
	mpt_projectpathname = _ACTION .. "win10"
	mpt_bindirsuffix = "win10"
	mpt_bindirsuffix32 = "win10"
	mpt_bindirsuffix64 = "win10"
elseif _OPTIONS["xp"] then
	allplatforms = { "x86", "x86_64" }
	trkplatforms = { "x86", "x86_64" }
	mpt_projectpathname = _ACTION .. "winxp"
	mpt_bindirsuffix = "winxp"
	mpt_bindirsuffix32 = "winxp"
	mpt_bindirsuffix64 = "winxp"
else
	allplatforms = { "x86", "x86_64" }
	trkplatforms = { "x86", "x86_64" }
	mpt_projectpathname = _ACTION .. "win7"
	mpt_bindirsuffix = "win7"
	mpt_bindirsuffix32 = "win7"
	mpt_bindirsuffix64 = "win7"
end

function replace_in_file (filename, from, to)
	local text
	local infile
	local outfile
	local oldtext
	local newtext
	infile = io.open(filename, "rb")
	text = infile:read("*all")
	infile:close()
	oldtext = text
	newtext = string.gsub(oldtext, from, to)
	text = newtext
	if newtext == oldtext then
   print("Failed to postprocess '" .. filename .. "': " .. from .. " -> " .. to)
   os.exit(1)
	end
	outfile = io.open(filename, "wb")
	outfile:write(text)
	outfile:close()
end

newaction {
 trigger     = "postprocess",
 description = "OpenMPT postprocess the project files to mitigate premake problems",
 execute     = function ()

 end
}

if _OPTIONS["group"] == "libopenmpt-all" then

solution "libopenmpt-all"
	startproject "libopenmpt"
 location ( "../../build/" .. mpt_projectpathname )
 configurations { "Debug", "Release", "Checked" }
 platforms ( allplatforms )
	dofile "../../build/premake/premake-defaults-solution.lua"

 dofile "../../build/premake/mpt-libopenmpt_test.lua"
 dofile "../../build/premake/mpt-libopenmpt.lua"
 dofile "../../build/premake/mpt-libopenmpt_examples.lua"
 dofile "../../build/premake/mpt-libopenmpt-small.lua"
 dofile "../../build/premake/mpt-libopenmpt_modplug.lua"
 dofile "../../build/premake/mpt-in_openmpt.lua"
 dofile "../../build/premake/mpt-xmp-openmpt.lua"
 dofile "../../build/premake/mpt-openmpt123.lua"
 dofile "../../build/premake/ext-flac.lua"
 dofile "../../build/premake/ext-minimp3.lua"
 dofile "../../build/premake/ext-miniz.lua"
 dofile "../../build/premake/ext-mpg123.lua"
 dofile "../../build/premake/ext-ogg.lua"
 dofile "../../build/premake/ext-portaudio.lua"
 dofile "../../build/premake/ext-portaudiocpp.lua"
 dofile "../../build/premake/ext-stb_vorbis.lua"
 dofile "../../build/premake/ext-vorbis.lua"
 dofile "../../build/premake/ext-zlib.lua"

end

if _OPTIONS["group"] == "libopenmpt_test" then

solution "libopenmpt_test"
	startproject "libopenmpt_test"
 location ( "../../build/" .. mpt_projectpathname )
 configurations { "Debug", "Release", "Checked" }
 platforms ( allplatforms )
	dofile "../../build/premake/premake-defaults-solution.lua"

 dofile "../../build/premake/mpt-libopenmpt_test.lua"
 dofile "../../build/premake/ext-mpg123.lua"
 dofile "../../build/premake/ext-ogg.lua"
 dofile "../../build/premake/ext-vorbis.lua"
 dofile "../../build/premake/ext-zlib.lua"

end

if _OPTIONS["group"] == "in_openmpt" then

solution "in_openmpt"
	startproject "in_openmpt"
 location ( "../../build/" .. mpt_projectpathname )
 configurations { "Debug", "Release", "Checked" }
 platforms { "x86" }
	dofile "../../build/premake/premake-defaults-solution.lua"

 dofile "../../build/premake/mpt-in_openmpt.lua"
 dofile "../../build/premake/mpt-libopenmpt.lua"
 dofile "../../build/premake/ext-mpg123.lua"
 dofile "../../build/premake/ext-ogg.lua"
 dofile "../../build/premake/ext-vorbis.lua"
 dofile "../../build/premake/ext-zlib.lua"

end

if _OPTIONS["group"] == "xmp-openmpt" then

solution "xmp-openmpt"
	startproject "xmp-openmpt"
 location ( "../../build/" .. mpt_projectpathname )
 configurations { "Debug", "Release", "Checked" }
 platforms { "x86" }
	dofile "../../build/premake/premake-defaults-solution.lua"

 dofile "../../build/premake/mpt-xmp-openmpt.lua"
 dofile "../../build/premake/mpt-libopenmpt.lua"
 dofile "../../build/premake/ext-mpg123.lua"
 dofile "../../build/premake/ext-ogg.lua"
 dofile "../../build/premake/ext-pugixml.lua"
 dofile "../../build/premake/ext-vorbis.lua"
 dofile "../../build/premake/ext-zlib.lua"

end

if _OPTIONS["group"] == "libopenmpt-small" then

solution "libopenmpt-small"
	startproject "libopenmpt-small"
 location ( "../../build/" .. mpt_projectpathname )
 configurations { "Debug", "Release", "Checked", "DebugShared", "ReleaseShared", "CheckedShared" }
 platforms ( allplatforms )
	dofile "../../build/premake/premake-defaults-solution.lua"

 dofile "../../build/premake/mpt-libopenmpt-small.lua"
 dofile "../../build/premake/ext-minimp3.lua"
 dofile "../../build/premake/ext-miniz.lua"
 dofile "../../build/premake/ext-stb_vorbis.lua"

end

-- should stay the last libopenmpt solution in order to overwrite the libopenmpt base project with all possible configurations
if _OPTIONS["group"] == "libopenmpt" then

solution "libopenmpt"
	startproject "libopenmpt"
 location ( "../../build/" .. mpt_projectpathname )
 configurations { "Debug", "Release", "Checked", "DebugShared", "ReleaseShared", "CheckedShared" }
 platforms ( allplatforms )
	dofile "../../build/premake/premake-defaults-solution.lua"

 dofile "../../build/premake/mpt-libopenmpt.lua"
 dofile "../../build/premake/mpt-libopenmpt_examples.lua"
 dofile "../../build/premake/mpt-libopenmpt_modplug.lua"
 dofile "../../build/premake/ext-mpg123.lua"
 dofile "../../build/premake/ext-ogg.lua"
 dofile "../../build/premake/ext-portaudio.lua"
 dofile "../../build/premake/ext-portaudiocpp.lua"
 dofile "../../build/premake/ext-vorbis.lua"
 dofile "../../build/premake/ext-zlib.lua"

end

if _OPTIONS["group"] == "openmpt123" then

solution "openmpt123"
	startproject "openmpt123"
 location ( "../../build/" .. mpt_projectpathname )
 configurations { "Debug", "Release", "Checked", "DebugShared", "ReleaseShared", "CheckedShared" }
 platforms ( allplatforms )
	dofile "../../build/premake/premake-defaults-solution.lua"

 dofile "../../build/premake/mpt-openmpt123.lua"
 dofile "../../build/premake/mpt-libopenmpt.lua"
 dofile "../../build/premake/ext-flac.lua"
 dofile "../../build/premake/ext-mpg123.lua"
 dofile "../../build/premake/ext-ogg.lua"
 dofile "../../build/premake/ext-portaudio.lua"
 dofile "../../build/premake/ext-vorbis.lua"
 dofile "../../build/premake/ext-zlib.lua"

end

if _OPTIONS["group"] == "PluginBridge" then

solution "PluginBridge"
	startproject "PluginBridge"
 location ( "../../build/" .. mpt_projectpathname )
 configurations { "Debug", "Release" }
 platforms ( trkplatforms )
	dofile "../../build/premake/premake-defaults-solution.lua"

 dofile "../../build/premake/mpt-PluginBridge.lua"

end

if _OPTIONS["group"] == "OpenMPT" then

charset = "Unicode"
stringmode = "UTF8"
solution "OpenMPT-UTF8"
	startproject "OpenMPT-UTF8"
 location ( "../../build/" .. mpt_projectpathname )
 configurations { "Debug", "Release", "Checked", "DebugShared", "ReleaseShared", "CheckedShared" }
 platforms ( trkplatforms )
	dofile "../../build/premake/premake-defaults-solution.lua"

 dofile "../../build/premake/mpt-OpenMPT.lua"
 dofile "../../build/premake/mpt-PluginBridge.lua"
 dofile "../../build/premake/ext-flac.lua"
 dofile "../../build/premake/ext-lame.lua"
 dofile "../../build/premake/ext-lhasa.lua"
 dofile "../../build/premake/ext-minizip.lua"
 dofile "../../build/premake/ext-mpg123.lua"
 dofile "../../build/premake/ext-ogg.lua"
 dofile "../../build/premake/ext-opus.lua"
 dofile "../../build/premake/ext-opusenc.lua"
 dofile "../../build/premake/ext-opusfile.lua"
 dofile "../../build/premake/ext-portaudio.lua"
 dofile "../../build/premake/ext-r8brain.lua"
 dofile "../../build/premake/ext-rtaudio.lua"
 dofile "../../build/premake/ext-rtmidi.lua"
 dofile "../../build/premake/ext-smbPitchShift.lua"
 dofile "../../build/premake/ext-soundtouch.lua"
 dofile "../../build/premake/ext-UnRAR.lua"
 dofile "../../build/premake/ext-vorbis.lua"
 dofile "../../build/premake/ext-zlib.lua"

charset = "MBCS"
stringmode = "WCHAR"
solution "OpenMPT-ANSI"
	startproject "OpenMPT-ANSI"
 location ( "../../build/" .. mpt_projectpathname )
 configurations { "Debug", "Release", "Checked", "DebugShared", "ReleaseShared", "CheckedShared" }
 platforms ( trkplatforms )
	dofile "../../build/premake/premake-defaults-solution.lua"

 dofile "../../build/premake/mpt-OpenMPT.lua"
 dofile "../../build/premake/mpt-PluginBridge.lua"
 dofile "../../build/premake/ext-flac.lua"
 dofile "../../build/premake/ext-lame.lua"
 dofile "../../build/premake/ext-lhasa.lua"
 dofile "../../build/premake/ext-minizip.lua"
 dofile "../../build/premake/ext-mpg123.lua"
 dofile "../../build/premake/ext-ogg.lua"
 dofile "../../build/premake/ext-opus.lua"
 dofile "../../build/premake/ext-opusenc.lua"
 dofile "../../build/premake/ext-opusfile.lua"
 dofile "../../build/premake/ext-portaudio.lua"
 dofile "../../build/premake/ext-r8brain.lua"
 dofile "../../build/premake/ext-rtaudio.lua"
 dofile "../../build/premake/ext-rtmidi.lua"
 dofile "../../build/premake/ext-smbPitchShift.lua"
 dofile "../../build/premake/ext-soundtouch.lua"
 dofile "../../build/premake/ext-UnRAR.lua"
 dofile "../../build/premake/ext-vorbis.lua"
 dofile "../../build/premake/ext-zlib.lua"

charset = "Unicode"
stringmode = "WCHAR"
solution "OpenMPT"
	startproject "OpenMPT"
 location ( "../../build/" .. mpt_projectpathname )
 configurations { "Debug", "Release", "Checked", "DebugShared", "ReleaseShared", "CheckedShared" }
 platforms ( trkplatforms )
	dofile "../../build/premake/premake-defaults-solution.lua"

 dofile "../../build/premake/mpt-OpenMPT.lua"
 dofile "../../build/premake/mpt-PluginBridge.lua"
 dofile "../../build/premake/ext-flac.lua"
 dofile "../../build/premake/ext-lame.lua"
 dofile "../../build/premake/ext-lhasa.lua"
 dofile "../../build/premake/ext-minizip.lua"
 dofile "../../build/premake/ext-mpg123.lua"
 dofile "../../build/premake/ext-ogg.lua"
 dofile "../../build/premake/ext-opus.lua"
 dofile "../../build/premake/ext-opusenc.lua"
 dofile "../../build/premake/ext-opusfile.lua"
 dofile "../../build/premake/ext-portaudio.lua"
 dofile "../../build/premake/ext-r8brain.lua"
 dofile "../../build/premake/ext-rtaudio.lua"
 dofile "../../build/premake/ext-rtmidi.lua"
 dofile "../../build/premake/ext-smbPitchShift.lua"
 dofile "../../build/premake/ext-soundtouch.lua"
 dofile "../../build/premake/ext-UnRAR.lua"
 dofile "../../build/premake/ext-vorbis.lua"
 dofile "../../build/premake/ext-zlib.lua"

end

-- overwrite all external projects once again with the full matrix of possible build config combinations
if _OPTIONS["group"] == "all-externals" then

solution "all-externals"
 location ( "../../build/" .. mpt_projectpathname .. "/ext" )
 configurations { "Debug", "Release", "Checked", "DebugShared", "ReleaseShared", "CheckedShared" }
 platforms ( allplatforms )
	dofile "../../build/premake/premake-defaults-solution.lua"

 dofile "../../build/premake/ext-flac.lua"
 dofile "../../build/premake/ext-lame.lua"
 dofile "../../build/premake/ext-lhasa.lua"
 dofile "../../build/premake/ext-minimp3.lua"
 dofile "../../build/premake/ext-miniz.lua"
 dofile "../../build/premake/ext-minizip.lua"
 dofile "../../build/premake/ext-mpg123.lua"
 dofile "../../build/premake/ext-ogg.lua"
 dofile "../../build/premake/ext-opus.lua"
 dofile "../../build/premake/ext-opusenc.lua"
 dofile "../../build/premake/ext-opusfile.lua"
 dofile "../../build/premake/ext-portaudio.lua"
 dofile "../../build/premake/ext-pugixml.lua"
 dofile "../../build/premake/ext-r8brain.lua"
 dofile "../../build/premake/ext-rtaudio.lua"
 dofile "../../build/premake/ext-rtmidi.lua"
 dofile "../../build/premake/ext-smbPitchShift.lua"
 dofile "../../build/premake/ext-soundtouch.lua"
 dofile "../../build/premake/ext-stb_vorbis.lua"
 dofile "../../build/premake/ext-UnRAR.lua"
 dofile "../../build/premake/ext-vorbis.lua"
 dofile "../../build/premake/ext-zlib.lua"

end
