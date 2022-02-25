
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
	trigger = "winxp",
	description = "Generate XP targetting projects",
}
newoption {
	trigger = "win7",
	description = "Generate Windows 7 Desktop targetting projects",
}
newoption {
	trigger = "win81",
	description = "Generate Windows 8.1 Desktop targetting projects",
}
newoption {
	trigger = "win10",
	description = "Generate Windows 10 Desktop targetting projects",
}

newoption {
	trigger = "uwp",
	description = "Generate Windows UWP targetting projects",
}

newoption {
	trigger = "clang",
	description = "ClangCL projects",
}

mpt_projectpathname = _ACTION
mpt_bindirsuffix = ""

if _OPTIONS["uwp"] then
	allplatforms = { "x86", "x86_64", "arm", "arm64" }
	mpt_projectpathname = mpt_projectpathname .. "uwp"
	mpt_bindirsuffix = mpt_bindirsuffix .. "uwp"
elseif _OPTIONS["win10"] then
	allplatforms = { "x86", "x86_64", "arm", "arm64" }
	mpt_projectpathname = mpt_projectpathname .. "win10"
	mpt_bindirsuffix = mpt_bindirsuffix .. "win10"
elseif _OPTIONS["win81"] then
	allplatforms = { "x86", "x86_64" }
	mpt_projectpathname = mpt_projectpathname .. "win81"
	mpt_bindirsuffix = mpt_bindirsuffix .. "win81"
elseif _OPTIONS["win7"] then
	allplatforms = { "x86", "x86_64" }
	mpt_projectpathname = mpt_projectpathname .. "win7"
	mpt_bindirsuffix = mpt_bindirsuffix .. "win7"
elseif _OPTIONS["winxp"] then
	allplatforms = { "x86", "x86_64" }
	mpt_projectpathname = mpt_projectpathname .. "winxp"
	mpt_bindirsuffix = mpt_bindirsuffix .. "winxp"
end

if _OPTIONS["clang"] then
	mpt_projectpathname = mpt_projectpathname .. "clang"
	mpt_bindirsuffix = mpt_bindirsuffix .. "clang"
end


dofile "../../build/premake/premake-defaults.lua"


if _OPTIONS["group"] == "libopenmpt_test" then

solution "libopenmpt_test"
	startproject "libopenmpt_test"
 configurations { "Debug", "Release", "Checked", "DebugShared", "ReleaseShared", "CheckedShared" }
 platforms ( allplatforms )

 dofile "../../build/premake/mpt-libopenmpt_test.lua"
 dofile "../../build/premake/ext-mpg123.lua"
 dofile "../../build/premake/ext-ogg.lua"
 dofile "../../build/premake/ext-vorbis.lua"
 dofile "../../build/premake/ext-zlib.lua"

end

if _OPTIONS["group"] == "in_openmpt" then

solution "in_openmpt"
	startproject "in_openmpt"
 configurations { "Debug", "Release", "Checked" }
 platforms { "x86" }

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
 configurations { "Debug", "Release", "Checked" }
 platforms { "x86" }

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
 configurations { "Debug", "Release", "Checked", "DebugShared", "ReleaseShared", "CheckedShared" }
 platforms ( allplatforms )

 dofile "../../build/premake/mpt-libopenmpt-small.lua"
 dofile "../../build/premake/ext-minimp3.lua"
 dofile "../../build/premake/ext-miniz.lua"
 dofile "../../build/premake/ext-stb_vorbis.lua"

end

-- should stay the last libopenmpt solution in order to overwrite the libopenmpt base project with all possible configurations
if _OPTIONS["group"] == "libopenmpt" then

solution "libopenmpt"
	startproject "libopenmpt"
 configurations { "Debug", "Release", "Checked", "DebugShared", "ReleaseShared", "CheckedShared" }
 platforms ( allplatforms )

 dofile "../../build/premake/mpt-libopenmpt.lua"
 if not _OPTIONS["uwp"] then
  dofile "../../build/premake/mpt-libopenmpt_examples.lua"
 end
 dofile "../../build/premake/ext-mpg123.lua"
 dofile "../../build/premake/ext-ogg.lua"
 if not _OPTIONS["uwp"] then
  dofile "../../build/premake/ext-portaudio.lua"
  dofile "../../build/premake/ext-portaudiocpp.lua"
 end
 dofile "../../build/premake/ext-vorbis.lua"
 dofile "../../build/premake/ext-zlib.lua"

end

if _OPTIONS["group"] == "openmpt123" then

solution "openmpt123"
	startproject "openmpt123"
 configurations { "Debug", "Release", "Checked", "DebugShared", "ReleaseShared", "CheckedShared" }
 platforms ( allplatforms )

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
 configurations { "Debug", "Release", "Checked" }
 platforms ( allplatforms )

 dofile "../../build/premake/mpt-PluginBridge.lua"

end

if _OPTIONS["group"] == "OpenMPT" then

charset = "Unicode"
stringmode = "UTF8"
solution "OpenMPT-UTF8"
	startproject "OpenMPT-UTF8"
 configurations { "Debug", "Release", "Checked", "DebugShared", "ReleaseShared", "CheckedShared" }
 platforms ( allplatforms )

 dofile "../../build/premake/mpt-OpenMPT.lua"
 dofile "../../build/premake/mpt-PluginBridge.lua"
 dofile "../../build/premake/mpt-updatesigntool.lua"
 dofile "../../build/premake/ext-ancient.lua"
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
 configurations { "Debug", "Release", "Checked", "DebugShared", "ReleaseShared", "CheckedShared" }
 platforms ( allplatforms )

 dofile "../../build/premake/mpt-OpenMPT.lua"
 dofile "../../build/premake/mpt-PluginBridge.lua"
 dofile "../../build/premake/mpt-updatesigntool.lua"
 dofile "../../build/premake/ext-ancient.lua"
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
 configurations { "Debug", "Release", "Checked", "DebugShared", "ReleaseShared", "CheckedShared" }
 platforms ( allplatforms )

 dofile "../../build/premake/mpt-OpenMPT.lua"
 dofile "../../build/premake/mpt-PluginBridge.lua"
 dofile "../../build/premake/mpt-updatesigntool.lua"
 dofile "../../build/premake/ext-ancient.lua"
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
 configurations { "Debug", "Release", "Checked", "DebugShared", "ReleaseShared", "CheckedShared" }
 platforms ( allplatforms )

 dofile "../../build/premake/ext-ancient.lua"
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
 dofile "../../build/premake/ext-portaudiocpp.lua"
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



if _OPTIONS["uwp"] then

	require('vstudio')

	local function mptGlobalsUWP(prj)
		if _ACTION == 'vs2022' then
			premake.w('<DefaultLanguage>en-US</DefaultLanguage>')
			premake.w('<MinimumVisualStudioVersion>15.0</MinimumVisualStudioVersion>')
			premake.w('<AppContainerApplication>true</AppContainerApplication>')
			premake.w('<ApplicationType>Windows Store</ApplicationType>')
			premake.w('<ApplicationTypeRevision>10.0</ApplicationTypeRevision>')
			premake.w('<WindowsTargetPlatformVersion Condition=" \'$(WindowsTargetPlatformVersion)\' == \'\' ">10.0.22000.0</WindowsTargetPlatformVersion>')
			premake.w('<WindowsTargetPlatformMinVersion>10.0.17134.0</WindowsTargetPlatformMinVersion>')
		elseif _ACTION == 'vs2019' then
			premake.w('<DefaultLanguage>en-US</DefaultLanguage>')
			premake.w('<MinimumVisualStudioVersion>15.0</MinimumVisualStudioVersion>')
			premake.w('<AppContainerApplication>true</AppContainerApplication>')
			premake.w('<ApplicationType>Windows Store</ApplicationType>')
			premake.w('<ApplicationTypeRevision>10.0</ApplicationTypeRevision>')
			premake.w('<WindowsTargetPlatformVersion>10.0.20348.0</WindowsTargetPlatformVersion>')
			premake.w('<WindowsTargetPlatformMinVersion>10.0.10240.0</WindowsTargetPlatformMinVersion>')
		end
	end

	local function mptClCompileUWP(prj)
		premake.w('<CompileAsWinRT>false</CompileAsWinRT>')
	end
	
	local function mptOutputPropertiesUWP(prj)
		premake.w('<IgnoreImportLibrary>false</IgnoreImportLibrary>')
	end
		
	local function mptProjectReferencesUWP(prj)
		premake.w('<ReferenceOutputAssembly>false</ReferenceOutputAssembly>')
	end

	premake.override(premake.vstudio.vc2010.elements, "globals", function(base, prj)
		local calls = base(prj)
		table.insert(calls, mptGlobalsUWP)
		return calls
	end)

	premake.override(premake.vstudio.vc2010.elements, "clCompile", function(base, prj)
		local calls = base(prj)
		table.insert(calls, mptClCompileUWP)
		return calls
	end)

	premake.override(premake.vstudio.vc2010.elements, "outputProperties", function(base, prj)
		local calls = base(prj)
		table.insert(calls, mptOutputPropertiesUWP)
		return calls
	end)

	premake.override(premake.vstudio.vc2010.elements, "projectReferences", function(base, prj)
		local calls = base(prj)
		table.insert(calls, mptProjectReferencesUWP)
		return calls
	end)

end
