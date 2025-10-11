
MPT_PREMAKE_VERSION = ""

MPT_PREMAKE_VERSION = "5.0"

newoption {
	trigger = "windows-version",
	value = "Windows Version",
	description = "Target Windows Version",
	default = "win81",
	allowed = {
		{ "winxp", "Windows XP" },
		{ "winxpx64", "Windows XP" },
		{ "win7", "Windows 7" },
		{ "win8", "Windows 8" },
		{ "win81", "Windows 8.1" },
		{ "win10", "Windows 10" },
		{ "win11", "Windows 11" }
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
	trigger = "windows-charset",
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



premake.ARM64EC = "ARM64EC"

premake.api.unregister("architecture")

premake.api.register {
	name = "architecture",
	scope = "config",
	kind = "string",
	allowed = {
		"universal",
		premake.X86,
		premake.X86_64,
		premake.ARM,
		premake.ARM64,
		premake.ARM64EC,
	},
	aliases = {
		i386  = premake.X86,
		amd64 = premake.X86_64,
		x32   = premake.X86,
		x64   = premake.X86_64,
	},
}

premake.vstudio.vs200x_architectures =
{
	win32   = "x86",
	x86     = "x86",
	x86_64  = "x64",
	ARM     = "ARM",
	ARM64   = "ARM64",
	ARM64EC = "ARM64EC",
}

function premake.vstudio.vc2010.windowsSDKDesktopARM64ECSupport(cfg)
	if cfg.system == premake.WINDOWS then
		if cfg.architecture == premake.ARM64EC then
			premake.w('<WindowsSDKDesktopARM64Support>true</WindowsSDKDesktopARM64Support>')
		end
	end
end

premake.override(premake.vstudio.vc2010.elements, "configurationProperties", function(base, prj)
	local calls = base(prj)
	table.insertafter(calls, premake.vstudio.vc2010.windowsSDKDesktopARMSupport, premake.vstudio.vc2010.windowsSDKDesktopARM64ECSupport)
	return calls
end)



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



mpt_projectpathname = _ACTION .. _OPTIONS["windows-version"]
mpt_bindirsuffix = _OPTIONS["windows-version"]

if _OPTIONS["windows-version"] == "win11" then
	if _OPTIONS["clang"] then
		allplatforms = { "x86", "x86_64", "arm64" }
	elseif _OPTIONS["windows-family"] == "uwp" then
		allplatforms = { "x86", "x86_64", "arm64" }
	else
		allplatforms = { "x86", "x86_64", "arm64", "arm64ec" }
	end
elseif _OPTIONS["windows-version"] == "win10" then
	allplatforms = { "x86", "x86_64", "arm", "arm64" }
elseif _OPTIONS["windows-version"] == "win81" then
	allplatforms = { "x86", "x86_64", "arm" }
elseif _OPTIONS["windows-version"] == "win8" then
	allplatforms = { "x86", "x86_64", "arm" }
elseif _OPTIONS["windows-version"] == "win7" then
	allplatforms = { "x86", "x86_64" }
elseif _OPTIONS["windows-version"] == "winxpx64" then
	allplatforms = { "x86", "x86_64" }
elseif _OPTIONS["windows-version"] == "winxp" then
	allplatforms = { "x86" }
else
	allplatforms = { "x86_64" }
end

if _OPTIONS["windows-family"] == "uwp" then
	mpt_projectpathname = mpt_projectpathname .. "uwp"
	mpt_bindirsuffix = mpt_bindirsuffix .. "uwp"
end

if _OPTIONS["clang"] then
	mpt_projectpathname = mpt_projectpathname .. "clang"
	mpt_bindirsuffix = mpt_bindirsuffix .. "clang"
end

if _OPTIONS["windows-charset"] == "MBCS" then
	mpt_projectpathname = mpt_projectpathname .. "ansi"
	mpt_bindirsuffix = mpt_bindirsuffix .. "ansi"
end


dofile "../../build/premake/premake-defaults.lua"


charset = "Unicode"
stringmode = "WCHAR"

solution "all"

	startproject "OpenMPT"

	dofile "../../build/premake/sys-mfc.lua"
	dofile "../../build/premake/ext-pthread-win32.lua"
	dofile "../../build/premake/ext-SignalsmithStretch.lua"
	dofile "../../build/premake/ext-UnRAR.lua"
	dofile "../../build/premake/ext-ancient.lua"
	dofile "../../build/premake/ext-asiomodern.lua"
	dofile "../../build/premake/ext-cryptopp.lua"
	dofile "../../build/premake/ext-flac.lua"
	dofile "../../build/premake/ext-lame.lua"
	dofile "../../build/premake/ext-lhasa.lua"
	dofile "../../build/premake/ext-minimp3.lua"
	dofile "../../build/premake/ext-miniz.lua"
	dofile "../../build/premake/ext-zlib.lua"
	dofile "../../build/premake/ext-minizip.lua"
	dofile "../../build/premake/ext-mpg123.lua"
	dofile "../../build/premake/ext-nlohmann-json.lua"
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
	dofile "../../build/premake/ext-stb_vorbis.lua"
	dofile "../../build/premake/ext-vorbis.lua"
	dofile "../../build/premake/ext-winamp.lua"
	dofile "../../build/premake/ext-xmplay.lua"
	dofile "../../build/premake/mpt-libopenmpt.lua"
	dofile "../../build/premake/mpt-libopenmpt-small.lua"
	dofile "../../build/premake/mpt-libopenmpt_test.lua"
if _OPTIONS["windows-family"] ~= "uwp" then
	dofile "../../build/premake/mpt-libopenmpt_examples.lua"
end
	dofile "../../build/premake/mpt-openmpt123.lua"
if _OPTIONS["windows-family"] ~= "uwp" then
	dofile "../../build/premake/mpt-in_openmpt.lua"
	dofile "../../build/premake/mpt-in_openmpt_wa2.lua"
	dofile "../../build/premake/mpt-xmp-openmpt.lua"
end
if _OPTIONS["windows-family"] ~= "uwp" then
	dofile "../../build/premake/mpt-updatesigntool.lua"
	dofile "../../build/premake/mpt-OpenMPT-NativeSupport.lua"
	dofile "../../build/premake/mpt-OpenMPT-WineWrapper.lua"
	dofile "../../build/premake/mpt-PluginBridge.lua"
	dofile "../../build/premake/mpt-OpenMPT.lua"
	charset = "Unicode"
	stringmode = "UTF8"
		dofile "../../build/premake/mpt-OpenMPT.lua"
	charset = "MBCS"
	stringmode = "WCHAR"
		dofile "../../build/premake/mpt-OpenMPT.lua"
end



charset = "Unicode"
stringmode = "WCHAR"



if _OPTIONS["windows-family"] ~= "uwp" then

solution "libopenmpt_test"
	startproject "libopenmpt_test"

 includeexternal "../../build/premake/ext-mpg123.lua"
 includeexternal "../../build/premake/ext-ogg.lua"
 includeexternal "../../build/premake/ext-vorbis.lua"
 includeexternal "../../build/premake/ext-zlib.lua"
 includeexternal "../../build/premake/mpt-libopenmpt_test.lua"

end



if _OPTIONS["windows-family"] ~= "uwp" then

solution "in_openmpt"
	startproject "in_openmpt"

 includeexternal "../../build/premake/sys-mfc.lua"
 includeexternal "../../build/premake/ext-mpg123.lua"
 includeexternal "../../build/premake/ext-ogg.lua"
 includeexternal "../../build/premake/ext-vorbis.lua"
 includeexternal "../../build/premake/ext-winamp.lua"
 includeexternal "../../build/premake/ext-zlib.lua"
 includeexternal "../../build/premake/mpt-libopenmpt.lua"
 includeexternal "../../build/premake/mpt-in_openmpt.lua"
 includeexternal "../../build/premake/mpt-in_openmpt_wa2.lua"

end



if _OPTIONS["windows-family"] ~= "uwp" then

solution "xmp-openmpt"
	startproject "xmp-openmpt"

 includeexternal "../../build/premake/sys-mfc.lua"
 includeexternal "../../build/premake/ext-mpg123.lua"
 includeexternal "../../build/premake/ext-ogg.lua"
 includeexternal "../../build/premake/ext-pugixml.lua"
 includeexternal "../../build/premake/ext-vorbis.lua"
 includeexternal "../../build/premake/ext-xmplay.lua"
 includeexternal "../../build/premake/ext-zlib.lua"
 includeexternal "../../build/premake/mpt-libopenmpt.lua"
 includeexternal "../../build/premake/mpt-xmp-openmpt.lua"

end



solution "libopenmpt-small"
	startproject "libopenmpt-small"

 includeexternal "../../build/premake/ext-minimp3.lua"
 includeexternal "../../build/premake/ext-miniz.lua"
 includeexternal "../../build/premake/ext-stb_vorbis.lua"
 includeexternal "../../build/premake/mpt-libopenmpt-small.lua"



solution "libopenmpt"
	startproject "libopenmpt"

 includeexternal "../../build/premake/ext-mpg123.lua"
 includeexternal "../../build/premake/ext-ogg.lua"
 includeexternal "../../build/premake/ext-vorbis.lua"
 includeexternal "../../build/premake/ext-zlib.lua"
 includeexternal "../../build/premake/mpt-libopenmpt.lua"



if _OPTIONS["windows-family"] ~= "uwp" then

 includeexternal "../../build/premake/ext-mpg123.lua"
 includeexternal "../../build/premake/ext-ogg.lua"
 includeexternal "../../build/premake/ext-portaudio.lua"
 includeexternal "../../build/premake/ext-portaudiocpp.lua"
 includeexternal "../../build/premake/ext-vorbis.lua"
 includeexternal "../../build/premake/ext-zlib.lua"
 includeexternal "../../build/premake/mpt-libopenmpt.lua"
 includeexternal "../../build/premake/mpt-libopenmpt_examples.lua"

end


solution "openmpt123"
	startproject "openmpt123"

if _ACTION < "vs2022" then
 includeexternal "../../build/premake/ext-pthread-win32.lua"
end
 includeexternal "../../build/premake/ext-flac.lua"
 includeexternal "../../build/premake/ext-mpg123.lua"
 includeexternal "../../build/premake/ext-ogg.lua"
 includeexternal "../../build/premake/ext-portaudio.lua"
 includeexternal "../../build/premake/ext-vorbis.lua"
 includeexternal "../../build/premake/ext-zlib.lua"
 includeexternal "../../build/premake/mpt-libopenmpt.lua"
 includeexternal "../../build/premake/mpt-openmpt123.lua"



if _OPTIONS["windows-family"] ~= "uwp" then

solution "PluginBridge"
	startproject "PluginBridge"

 includeexternal "../../build/premake/mpt-PluginBridge.lua"

end



if _OPTIONS["windows-family"] ~= "uwp" then

charset = "Unicode"
stringmode = "UTF8"
solution "OpenMPT-UTF8"
	startproject "OpenMPT-UTF8"

 includeexternal "../../build/premake/sys-mfc.lua"
 includeexternal "../../build/premake/ext-ancient.lua"
 includeexternal "../../build/premake/ext-asiomodern.lua"
if _OPTIONS["windows-version"] == "winxp" or _OPTIONS["windows-version"] == "winxpx64" then
 includeexternal "../../build/premake/ext-cryptopp.lua"
end
if _ACTION < "vs2022" then
 includeexternal "../../build/premake/ext-pthread-win32.lua"
end
 includeexternal "../../build/premake/ext-flac.lua"
 includeexternal "../../build/premake/ext-lame.lua"
 includeexternal "../../build/premake/ext-lhasa.lua"
 includeexternal "../../build/premake/ext-mpg123.lua"
 includeexternal "../../build/premake/ext-nlohmann-json.lua"
 includeexternal "../../build/premake/ext-ogg.lua"
 includeexternal "../../build/premake/ext-opus.lua"
 includeexternal "../../build/premake/ext-opusenc.lua"
 includeexternal "../../build/premake/ext-opusfile.lua"
 includeexternal "../../build/premake/ext-portaudio.lua"
 includeexternal "../../build/premake/ext-r8brain.lua"
 includeexternal "../../build/premake/ext-rtaudio.lua"
 includeexternal "../../build/premake/ext-rtmidi.lua"
 includeexternal "../../build/premake/ext-SignalsmithStretch.lua"
 includeexternal "../../build/premake/ext-UnRAR.lua"
 includeexternal "../../build/premake/ext-vorbis.lua"
 includeexternal "../../build/premake/ext-zlib.lua"
 includeexternal "../../build/premake/ext-minizip.lua"
 includeexternal "../../build/premake/mpt-updatesigntool.lua"
 includeexternal "../../build/premake/mpt-PluginBridge.lua"
 includeexternal "../../build/premake/mpt-OpenMPT-NativeSupport.lua"
 includeexternal "../../build/premake/mpt-OpenMPT-WineWrapper.lua"
 includeexternal "../../build/premake/mpt-OpenMPT.lua"

charset = "MBCS"
stringmode = "WCHAR"
solution "OpenMPT-ANSI"
	startproject "OpenMPT-ANSI"

 includeexternal "../../build/premake/sys-mfc.lua"
 includeexternal "../../build/premake/ext-ancient.lua"
 includeexternal "../../build/premake/ext-asiomodern.lua"
if _OPTIONS["windows-version"] == "winxp" or _OPTIONS["windows-version"] == "winxpx64" then
 includeexternal "../../build/premake/ext-cryptopp.lua"
end
if _ACTION < "vs2022" then
 includeexternal "../../build/premake/ext-pthread-win32.lua"
end
 includeexternal "../../build/premake/ext-flac.lua"
 includeexternal "../../build/premake/ext-lame.lua"
 includeexternal "../../build/premake/ext-lhasa.lua"
 includeexternal "../../build/premake/ext-mpg123.lua"
 includeexternal "../../build/premake/ext-nlohmann-json.lua"
 includeexternal "../../build/premake/ext-ogg.lua"
 includeexternal "../../build/premake/ext-opus.lua"
 includeexternal "../../build/premake/ext-opusenc.lua"
 includeexternal "../../build/premake/ext-opusfile.lua"
 includeexternal "../../build/premake/ext-portaudio.lua"
 includeexternal "../../build/premake/ext-r8brain.lua"
 includeexternal "../../build/premake/ext-rtaudio.lua"
 includeexternal "../../build/premake/ext-rtmidi.lua"
 includeexternal "../../build/premake/ext-SignalsmithStretch.lua"
 includeexternal "../../build/premake/ext-UnRAR.lua"
 includeexternal "../../build/premake/ext-vorbis.lua"
 includeexternal "../../build/premake/ext-zlib.lua"
 includeexternal "../../build/premake/ext-minizip.lua"
 includeexternal "../../build/premake/mpt-updatesigntool.lua"
 includeexternal "../../build/premake/mpt-PluginBridge.lua"
 includeexternal "../../build/premake/mpt-OpenMPT-NativeSupport.lua"
 includeexternal "../../build/premake/mpt-OpenMPT-WineWrapper.lua"
 includeexternal "../../build/premake/mpt-OpenMPT.lua"

charset = "Unicode"
stringmode = "WCHAR"
solution "OpenMPT"
	startproject "OpenMPT"

 includeexternal "../../build/premake/sys-mfc.lua"
 includeexternal "../../build/premake/ext-ancient.lua"
 includeexternal "../../build/premake/ext-asiomodern.lua"
if _OPTIONS["windows-version"] == "winxp" or _OPTIONS["windows-version"] == "winxpx64" then
 includeexternal "../../build/premake/ext-cryptopp.lua"
end
if _ACTION < "vs2022" then
 includeexternal "../../build/premake/ext-pthread-win32.lua"
end
 includeexternal "../../build/premake/ext-flac.lua"
 includeexternal "../../build/premake/ext-lame.lua"
 includeexternal "../../build/premake/ext-lhasa.lua"
 includeexternal "../../build/premake/ext-mpg123.lua"
 includeexternal "../../build/premake/ext-nlohmann-json.lua"
 includeexternal "../../build/premake/ext-ogg.lua"
 includeexternal "../../build/premake/ext-opus.lua"
 includeexternal "../../build/premake/ext-opusenc.lua"
 includeexternal "../../build/premake/ext-opusfile.lua"
 includeexternal "../../build/premake/ext-portaudio.lua"
 includeexternal "../../build/premake/ext-r8brain.lua"
 includeexternal "../../build/premake/ext-rtaudio.lua"
 includeexternal "../../build/premake/ext-rtmidi.lua"
 includeexternal "../../build/premake/ext-SignalsmithStretch.lua"
 includeexternal "../../build/premake/ext-UnRAR.lua"
 includeexternal "../../build/premake/ext-vorbis.lua"
 includeexternal "../../build/premake/ext-zlib.lua"
 includeexternal "../../build/premake/ext-minizip.lua"
 includeexternal "../../build/premake/mpt-updatesigntool.lua"
 includeexternal "../../build/premake/mpt-PluginBridge.lua"
 includeexternal "../../build/premake/mpt-OpenMPT-NativeSupport.lua"
 includeexternal "../../build/premake/mpt-OpenMPT-WineWrapper.lua"
 includeexternal "../../build/premake/mpt-OpenMPT.lua"

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
			if _OPTIONS["windows-version"] == "win10" then
				premake.w('<WindowsTargetPlatformVersion Condition=" \'$(WindowsTargetPlatformVersion)\' == \'\' ">10.0.22621.0</WindowsTargetPlatformVersion>')
				premake.w('<WindowsTargetPlatformMinVersion>10.0.19045.0</WindowsTargetPlatformMinVersion>')
			elseif _OPTIONS["windows-version"] == "win11" then
				premake.w('<WindowsTargetPlatformVersion Condition=" \'$(WindowsTargetPlatformVersion)\' == \'\' ">10.0.26100.0</WindowsTargetPlatformVersion>')
				premake.w('<WindowsTargetPlatformMinVersion>10.0.22631.0</WindowsTargetPlatformMinVersion>')
			end
		elseif _ACTION == 'vs2019' then
			premake.w('<DefaultLanguage>en-US</DefaultLanguage>')
			premake.w('<MinimumVisualStudioVersion>15.0</MinimumVisualStudioVersion>')
			premake.w('<AppContainerApplication>true</AppContainerApplication>')
			premake.w('<ApplicationType>Windows Store</ApplicationType>')
			premake.w('<ApplicationTypeRevision>10.0</ApplicationTypeRevision>')
			premake.w('<WindowsTargetPlatformVersion>10.0.22621.0</WindowsTargetPlatformVersion>')
			premake.w('<WindowsTargetPlatformMinVersion>10.0.19045.0</WindowsTargetPlatformMinVersion>')
		end
	end

	local function mptClCompileUWP(prj)
		premake.w('<CompileAsWinRT>false</CompileAsWinRT>')
	end

	local function mptClCompileUWPC(prj)
		if premake.languages.isc(prj.language) then
			premake.w('<ForcedUsingFiles />')
		end
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

	premake.override(premake.vstudio.vc2010.elements, "clCompile", function(base, prj)
		local calls = base(prj)
		table.insert(calls, mptClCompileUWPC)
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
