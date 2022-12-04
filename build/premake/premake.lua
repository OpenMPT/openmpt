
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
 }
}

newoption {
	trigger = "windows-version",
	value = "Windows Version",
	description = "Target Windows Version",
	default = "win81",
	allowed = {
		{ "winxp", "Windows XP" },
		{ "win7", "Windows 7" },
		{ "win81", "Windows 8.1" },
		{ "win10", "Wiondows 10" }
	}
}

newoption {
	trigger = "windows-family",
	value = "Windows Family",
	description = "Target Windows Family",
	default = "desktop",
	allowed = {
		{ "desktop", "Win32" },
		{ "uwp", "UWP" }
	}
}

newoption {
	trigger = "charset",
	value = "Character Set",
	description = "Windows Default Character Set",
	default = "Unicode",
	allowed = {
		{ "Unicode", "Unicode" },
		{ "MBCS", "MBCS" }
	}
}

newoption {
	trigger = "clang",
	description = "ClangCL projects",
}



require('vstudio')



premake.api.register {
	name = "spectremitigations",
	scope = "config",
	kind = "string",
	allowed = {
		"Default",
		"On",
		"Off",
	}
}

function premake.vstudio.vc2010.spectreMitigations(cfg)
	if (cfg.spectremitigations == 'On') then
		if _ACTION >= "vs2017" then
			premake.vstudio.vc2010.element("SpectreMitigation", nil, "Spectre")
		end
	end
end

premake.override(premake.vstudio.vc2010.elements, "configurationProperties", function(base, prj)
	local calls = base(prj)
	table.insertafter(calls, premake.vstudio.vc2010.wholeProgramOptimization, premake.vstudio.vc2010.spectreMitigations)
	return calls
end)



premake.api.register {
	name = "dataexecutionprevention",
	scope = "config",
	kind = "string",
	allowed = {
		"Default",
		"Off",
		"On",
	}
}

function premake.vstudio.vc2010.dataExecutionPrevention(cfg)
	if (cfg.dataexecutionprevention == 'Off') then
		premake.vstudio.vc2010.element("DataExecutionPrevention", nil, 'false')
	end
end

premake.override(premake.vstudio.vc2010.elements, "link", function(base, prj)
	local calls = base(prj)
	table.insertafter(calls, premake.vstudio.vc2010.ignoreDefaultLibraries, premake.vstudio.vc2010.dataExecutionPrevention)
	return calls
end)



premake.api.register {
	name = "preprocessor",
	scope = "config",
	kind = "string",
	allowed = {
		"Default",
		"Standard",
		"Legacy",
	}
}

function premake.vstudio.vc2010.preprocessor(cfg)
	if _ACTION >= "vs2019" then
		if (cfg.preprocessor == 'Standard') then
			premake.vstudio.vc2010.element("UseStandardPreprocessor", nil, "true")
		elseif (cfg.preprocessor == 'Legacy') then
			premake.vstudio.vc2010.element("UseStandardPreprocessor", nil, "false")
		end
	end
end

premake.override(premake.vstudio.vc2010.elements, "clCompile", function(base, prj)
	local calls = base(prj)
	table.insertafter(calls, premake.vstudio.vc2010.externalAngleBrackets, premake.vstudio.vc2010.preprocessor)
	return calls
end)



mpt_projectpathname = _ACTION .. _OPTIONS["windows-version"]
mpt_bindirsuffix = _OPTIONS["windows-version"]

if _OPTIONS["windows-version"] == "win10" then
	allplatforms = { "x86", "x86_64", "arm", "arm64" }
elseif _OPTIONS["windows-version"] == "win81" then
	allplatforms = { "x86", "x86_64" }
elseif _OPTIONS["windows-version"] == "win7" then
	allplatforms = { "x86", "x86_64" }
elseif _OPTIONS["windows-version"] == "winxp" then
	allplatforms = { "x86", "x86_64" }
else
	allplatforms = { "x86", "x86_64" }
end

if _OPTIONS["windows-family"] == "uwp" then
	mpt_projectpathname = mpt_projectpathname .. "uwp"
	mpt_bindirsuffix = mpt_bindirsuffix .. "uwp"
end

if _OPTIONS["clang"] then
	mpt_projectpathname = mpt_projectpathname .. "clang"
	mpt_bindirsuffix = mpt_bindirsuffix .. "clang"
end

if _OPTIONS["charset"] == "MBCS" then
	mpt_projectpathname = mpt_projectpathname .. "ansi"
	mpt_bindirsuffix = mpt_bindirsuffix .. "ansi"
end


dofile "../../build/premake/premake-defaults.lua"


if _OPTIONS["group"] == "libopenmpt_test" then

solution "libopenmpt_test"
	startproject "libopenmpt_test"

 dofile "../../build/premake/ext-mpg123.lua"
 dofile "../../build/premake/ext-ogg.lua"
 dofile "../../build/premake/ext-vorbis.lua"
 dofile "../../build/premake/ext-zlib.lua"
 dofile "../../build/premake/mpt-libopenmpt_test.lua"

end

if _OPTIONS["group"] == "in_openmpt" then

solution "in_openmpt"
	startproject "in_openmpt"

 dofile "../../build/premake/sys-mfc.lua"
 dofile "../../build/premake/ext-mpg123.lua"
 dofile "../../build/premake/ext-ogg.lua"
 dofile "../../build/premake/ext-vorbis.lua"
 dofile "../../build/premake/ext-winamp.lua"
 dofile "../../build/premake/ext-zlib.lua"
 dofile "../../build/premake/mpt-libopenmpt.lua"
 dofile "../../build/premake/mpt-in_openmpt.lua"

end

if _OPTIONS["group"] == "xmp-openmpt" then

solution "xmp-openmpt"
	startproject "xmp-openmpt"

 dofile "../../build/premake/sys-mfc.lua"
 dofile "../../build/premake/ext-mpg123.lua"
 dofile "../../build/premake/ext-ogg.lua"
 dofile "../../build/premake/ext-pugixml.lua"
 dofile "../../build/premake/ext-vorbis.lua"
 dofile "../../build/premake/ext-xmplay.lua"
 dofile "../../build/premake/ext-zlib.lua"
 dofile "../../build/premake/mpt-libopenmpt.lua"
 dofile "../../build/premake/mpt-xmp-openmpt.lua"

end

if _OPTIONS["group"] == "libopenmpt-small" then

solution "libopenmpt-small"
	startproject "libopenmpt-small"

 dofile "../../build/premake/ext-minimp3.lua"
 dofile "../../build/premake/ext-miniz.lua"
 dofile "../../build/premake/ext-stb_vorbis.lua"
 dofile "../../build/premake/mpt-libopenmpt-small.lua"

end

-- should stay the last libopenmpt solution in order to overwrite the libopenmpt base project with all possible configurations
if _OPTIONS["group"] == "libopenmpt" then

solution "libopenmpt"
	startproject "libopenmpt"

 dofile "../../build/premake/ext-mpg123.lua"
 dofile "../../build/premake/ext-ogg.lua"
 if _OPTIONS["windows-family"] ~= "uwp" then
  dofile "../../build/premake/ext-portaudio.lua"
  dofile "../../build/premake/ext-portaudiocpp.lua"
 end
 dofile "../../build/premake/ext-vorbis.lua"
 dofile "../../build/premake/ext-zlib.lua"
 dofile "../../build/premake/mpt-libopenmpt.lua"
 if _OPTIONS["windows-family"] ~= "uwp" then
  dofile "../../build/premake/mpt-libopenmpt_examples.lua"
 end

end

if _OPTIONS["group"] == "openmpt123" then

solution "openmpt123"
	startproject "openmpt123"

 dofile "../../build/premake/ext-flac.lua"
 dofile "../../build/premake/ext-mpg123.lua"
 dofile "../../build/premake/ext-ogg.lua"
 dofile "../../build/premake/ext-portaudio.lua"
 dofile "../../build/premake/ext-vorbis.lua"
 dofile "../../build/premake/ext-zlib.lua"
 dofile "../../build/premake/mpt-libopenmpt.lua"
 dofile "../../build/premake/mpt-openmpt123.lua"

end

if _OPTIONS["group"] == "PluginBridge" then

solution "PluginBridge"
	startproject "PluginBridge"

 dofile "../../build/premake/mpt-PluginBridge.lua"

end

if _OPTIONS["group"] == "OpenMPT" then

charset = "Unicode"
stringmode = "UTF8"
solution "OpenMPT-UTF8"
	startproject "OpenMPT-UTF8"

 dofile "../../build/premake/sys-mfc.lua"
 dofile "../../build/premake/ext-ancient.lua"
 dofile "../../build/premake/ext-asiomodern.lua"
if _OPTIONS["windows-version"] == "winxp" then
 dofile "../../build/premake/ext-cryptopp.lua"
end
 dofile "../../build/premake/ext-flac.lua"
 dofile "../../build/premake/ext-lame.lua"
 dofile "../../build/premake/ext-lhasa.lua"
 dofile "../../build/premake/ext-mpg123.lua"
 dofile "../../build/premake/ext-nlohmann-json.lua"
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
 dofile "../../build/premake/ext-minizip.lua"
 dofile "../../build/premake/mpt-updatesigntool.lua"
 dofile "../../build/premake/mpt-PluginBridge.lua"
 dofile "../../build/premake/mpt-OpenMPT-NativeSupport.lua"
 dofile "../../build/premake/mpt-OpenMPT-WineWrapper.lua"
 dofile "../../build/premake/mpt-OpenMPT.lua"

charset = "MBCS"
stringmode = "WCHAR"
solution "OpenMPT-ANSI"
	startproject "OpenMPT-ANSI"

 dofile "../../build/premake/sys-mfc.lua"
 dofile "../../build/premake/ext-ancient.lua"
 dofile "../../build/premake/ext-asiomodern.lua"
if _OPTIONS["windows-version"] == "winxp" then
 dofile "../../build/premake/ext-cryptopp.lua"
end
 dofile "../../build/premake/ext-flac.lua"
 dofile "../../build/premake/ext-lame.lua"
 dofile "../../build/premake/ext-lhasa.lua"
 dofile "../../build/premake/ext-mpg123.lua"
 dofile "../../build/premake/ext-nlohmann-json.lua"
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
 dofile "../../build/premake/ext-minizip.lua"
 dofile "../../build/premake/mpt-updatesigntool.lua"
 dofile "../../build/premake/mpt-PluginBridge.lua"
 dofile "../../build/premake/mpt-OpenMPT-NativeSupport.lua"
 dofile "../../build/premake/mpt-OpenMPT-WineWrapper.lua"
 dofile "../../build/premake/mpt-OpenMPT.lua"

charset = "Unicode"
stringmode = "WCHAR"
solution "OpenMPT"
	startproject "OpenMPT"

 dofile "../../build/premake/sys-mfc.lua"
 dofile "../../build/premake/ext-ancient.lua"
 dofile "../../build/premake/ext-asiomodern.lua"
if _OPTIONS["windows-version"] == "winxp" then
 dofile "../../build/premake/ext-cryptopp.lua"
end
 dofile "../../build/premake/ext-flac.lua"
 dofile "../../build/premake/ext-lame.lua"
 dofile "../../build/premake/ext-lhasa.lua"
 dofile "../../build/premake/ext-mpg123.lua"
 dofile "../../build/premake/ext-nlohmann-json.lua"
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
 dofile "../../build/premake/ext-minizip.lua"
 dofile "../../build/premake/mpt-updatesigntool.lua"
 dofile "../../build/premake/mpt-PluginBridge.lua"
 dofile "../../build/premake/mpt-OpenMPT-NativeSupport.lua"
 dofile "../../build/premake/mpt-OpenMPT-WineWrapper.lua"
 dofile "../../build/premake/mpt-OpenMPT.lua"

end



if _OPTIONS["windows-family"] == "uwp" then

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
