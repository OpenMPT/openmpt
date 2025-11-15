
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
	name = "linktimeoptimization2",
	scope = "config",
	kind = "string",
	allowed = {
		"Default",
		"On",
		"Off",
		"Fast",
	}
}

function premake.vstudio.vc2010.wholeProgramOptimization2(cfg)
	if cfg.linktimeoptimization2 == "On" then
		premake.vstudio.vc2010.element("WholeProgramOptimization", nil, "true")
	elseif cfg.linktimeoptimization2 == "Fast" then
		premake.vstudio.vc2010.element("WholeProgramOptimization", nil, "true")
	elseif cfg.linktimeoptimization2 == "Off" then
		premake.vstudio.vc2010.element("WholeProgramOptimization", nil, "false")
	end
end

function premake.vstudio.vc2010.linkTimeCodeGeneration2(cfg)
	if cfg.linktimeoptimization2 == "On" then
		premake.vstudio.vc2010.element("LinkTimeCodeGeneration", nil, "UseLinkTimeCodeGeneration")
	elseif cfg.linktimeoptimization2 == "Fast" then
		premake.vstudio.vc2010.element("LinkTimeCodeGeneration", nil, "UseFastLinkTimeCodeGeneration")
	end
end

premake.override(premake.vstudio.vc2010.elements, "configurationProperties", function(base, prj)
	local calls = base(prj)
	table.insertafter(calls, premake.vstudio.vc2010.wholeProgramOptimization, premake.vstudio.vc2010.wholeProgramOptimization2)
	return calls
end)

premake.override(premake.vstudio.vc2010.elements, "link", function(base, prj)
	local calls = base(prj)
	table.insertafter(calls, premake.vstudio.vc2010.linkTimeCodeGeneration, premake.vstudio.vc2010.linkTimeCodeGeneration2)
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



function MPT_WIN_MAKE_VERSION(major, minor, sp, build)
	return ((major << 24) + (minor << 16) + (sp << 8) + (build << 0))
end

MPT_WIN = {

	["WIN32S"]   = MPT_WIN_MAKE_VERSION(0x03, 0x00, 0x00, 0x00),

	["WIN95"]    = MPT_WIN_MAKE_VERSION(0x04, 0x00, 0x00, 0x00),
	["WIN98"]    = MPT_WIN_MAKE_VERSION(0x04, 0x10, 0x00, 0x00),
	["WINME"]    = MPT_WIN_MAKE_VERSION(0x04, 0x90, 0x00, 0x00),

	["NT3"]      = MPT_WIN_MAKE_VERSION(0x03, 0x00, 0x00, 0x00),
	["NT4"]      = MPT_WIN_MAKE_VERSION(0x04, 0x00, 0x00, 0x00),
	["2000"]     = MPT_WIN_MAKE_VERSION(0x05, 0x00, 0x00, 0x00),
	["2000SP1"]  = MPT_WIN_MAKE_VERSION(0x05, 0x00, 0x01, 0x00),
	["2000SP2"]  = MPT_WIN_MAKE_VERSION(0x05, 0x00, 0x02, 0x00),
	["2000SP3"]  = MPT_WIN_MAKE_VERSION(0x05, 0x00, 0x03, 0x00),
	["2000SP4"]  = MPT_WIN_MAKE_VERSION(0x05, 0x00, 0x04, 0x00),
	["XP"]       = MPT_WIN_MAKE_VERSION(0x05, 0x01, 0x00, 0x00),
	["XPSP1"]    = MPT_WIN_MAKE_VERSION(0x05, 0x01, 0x01, 0x00),
	["XPSP2"]    = MPT_WIN_MAKE_VERSION(0x05, 0x01, 0x02, 0x00),
	["XPSP3"]    = MPT_WIN_MAKE_VERSION(0x05, 0x01, 0x03, 0x00),
	["XPSP4"]    = MPT_WIN_MAKE_VERSION(0x05, 0x01, 0x04, 0x00), -- unused
	["XP64"]     = MPT_WIN_MAKE_VERSION(0x05, 0x02, 0x00, 0x00), -- unused
	["XP64SP1"]  = MPT_WIN_MAKE_VERSION(0x05, 0x02, 0x01, 0x00),
	["XP64SP2"]  = MPT_WIN_MAKE_VERSION(0x05, 0x02, 0x02, 0x00),
	["XP64SP3"]  = MPT_WIN_MAKE_VERSION(0x05, 0x02, 0x03, 0x00), -- unused
	["XP64SP4"]  = MPT_WIN_MAKE_VERSION(0x05, 0x02, 0x04, 0x00), -- unused
	["VISTA"]    = MPT_WIN_MAKE_VERSION(0x06, 0x00, 0x00, 0x00),
	["VISTASP1"] = MPT_WIN_MAKE_VERSION(0x06, 0x00, 0x01, 0x00),
	["VISTASP2"] = MPT_WIN_MAKE_VERSION(0x06, 0x00, 0x02, 0x00),
	["VISTASP3"] = MPT_WIN_MAKE_VERSION(0x06, 0x00, 0x03, 0x00), -- unused
	["VISTASP4"] = MPT_WIN_MAKE_VERSION(0x06, 0x00, 0x04, 0x00), -- unused
	["7"]        = MPT_WIN_MAKE_VERSION(0x06, 0x01, 0x00, 0x00),
	["8"]        = MPT_WIN_MAKE_VERSION(0x06, 0x02, 0x00, 0x00),
	["81"]       = MPT_WIN_MAKE_VERSION(0x06, 0x03, 0x00, 0x00),

	["10_PRE"]   = MPT_WIN_MAKE_VERSION(0x06, 0x04, 0x00, 0x00),
	["10"]       = MPT_WIN_MAKE_VERSION(0x0a, 0x00, 0x00, 0x00), -- NTDDI_WIN10      1507
	["10_1511"]  = MPT_WIN_MAKE_VERSION(0x0a, 0x00, 0x00, 0x01), -- NTDDI_WIN10_TH2  1511
	["10_1607"]  = MPT_WIN_MAKE_VERSION(0x0a, 0x00, 0x00, 0x02), -- NTDDI_WIN10_RS1  1607
	["10_1703"]  = MPT_WIN_MAKE_VERSION(0x0a, 0x00, 0x00, 0x03), -- NTDDI_WIN10_RS2  1703
	["10_1709"]  = MPT_WIN_MAKE_VERSION(0x0a, 0x00, 0x00, 0x04), -- NTDDI_WIN10_RS3  1709
	["10_1803"]  = MPT_WIN_MAKE_VERSION(0x0a, 0x00, 0x00, 0x05), -- NTDDI_WIN10_RS4  1803
	["10_1809"]  = MPT_WIN_MAKE_VERSION(0x0a, 0x00, 0x00, 0x06), -- NTDDI_WIN10_RS5  1809
	["10_1903"]  = MPT_WIN_MAKE_VERSION(0x0a, 0x00, 0x00, 0x07), -- NTDDI_WIN10_19H1 1903/19H1
	--["10_1909"]                                                                    1909/19H2
	["10_2004"]  = MPT_WIN_MAKE_VERSION(0x0a, 0x00, 0x00, 0x08), -- NTDDI_WIN10_VB   2004/20H1
	--["10_20H2"]                                                                    20H2
	--["10_21H1"]                                                                    21H1
	--["10_21H2"]                                                                    21H2
	--["10_22H2"]                                                                    22H2

	["11"]       = MPT_WIN_MAKE_VERSION(0x0a, 0x00, 0x00, 0x0b), -- NTDDI_WIN10_CO   21H2
	["11_22H2"]  = MPT_WIN_MAKE_VERSION(0x0a, 0x00, 0x00, 0x0c), -- NTDDI_WIN10_NI   22H2
	--["11_23H2"]                                                                    23H2
	["11_24H2"]  = MPT_WIN_MAKE_VERSION(0x0a, 0x00, 0x00, 0x10), -- NTDDI_WIN11_GE   24H2
	--["11_25H2"]                                                                    25H2

}



MPT_BUILD_MSBUILD = false
if string.sub(_ACTION, 1, 2) == "vs" then
	MPT_BUILD_MSBUILD = true
end



MPT_COMPILER_CLANG = false
MPT_COMPILER_CLANGCL = false
MPT_COMPILER_GCC = false
MPT_COMPILER_MSVC = false
if MPT_BUILD_MSBUILD then
	if _OPTIONS["clang"] then
		MPT_COMPILER_CLANGCL = true
		MPT_COMPILER_CLANGCL_VERSION = tonumber(string.sub(_ACTION, 3))
	else
		MPT_COMPILER_MSVC = true
		MPT_COMPILER_MSVC_VERSION = tonumber(string.sub(_ACTION, 3))
	end
end
if MPT_COMPILER_CLANGCL then
	function MPT_CLANGCL_BEFORE(v)
		return (MPT_COMPILER_CLANGCL_VERSION < v)
	end
	function MPT_CLANGCL_AT_LEAST(v)
		return (MPT_COMPILER_CLANGCL_VERSION >= v)
	end
else
	function MPT_CLANGCL_BEFORE(v)
		return false
	end
	function MPT_CLANGCL_AT_LEAST(v)
		return false
	end
end
if MPT_COMPILER_MSVC then
	function MPT_MSVC_BEFORE(v)
		return (MPT_COMPILER_MSVC_VERSION < v)
	end
	function MPT_MSVC_AT_LEAST(v)
		return (MPT_COMPILER_MSVC_VERSION >= v)
	end
else
	function MPT_MSVC_BEFORE(v)
		return false
	end
	function MPT_MSVC_AT_LEAST(v)
		return false
	end
end



if _TARGET_OS == "windows" then

	MPT_OS_WINDOWS = true

	if _OPTIONS["windows-family"] == "uwp" then
		MPT_OS_WINDOWS_WINRT = true
		MPT_OS_WINDOWS_WINNT = true
		MPT_OS_WINDOWS_WIN9X = false
		MPT_OS_WINDOWS_WIN32 = false
	else
		MPT_OS_WINDOWS_WINRT = false
		MPT_OS_WINDOWS_WINNT = true
		MPT_OS_WINDOWS_WIN9X = false
		MPT_OS_WINDOWS_WIN32 = false
	end

	function MPT_WINRT_AT_LEAST(v)
		return MPT_OS_WINDOWS_WINRT and MPT_OS_WINDOWS_WINNT and (MPT_WIN_VERSION >= v)
	end
	function MPT_WINRT_BEFORE(v)
		return MPT_OS_WINDOWS_WINRT and MPT_OS_WINDOWS_WINNT and (MPT_WIN_VERSION < v)
	end
	function MPT_WINNT_AT_LEAST(v)
		return MPT_OS_WINDOWS_WINNT and (MPT_WIN_VERSION >= v)
	end
	function MPT_WINNT_BEFORE(v)
		return MPT_OS_WINDOWS_WINNT and (MPT_WIN_VERSION < v)
	end
	function MPT_WIN9X_AT_LEAST(v)
		return (MPT_OS_WINDOWS_WINNT or MPT_OS_WINDOWS_WIN9X) and (MPT_WIN_VERSION >= v)
	end
	function MPT_WIN9X_BEFORE(v)
		return (MPT_OS_WINDOWS_WINNT or MPT_OS_WINDOWS_WIN9X) and (MPT_WIN_VERSION < v)
	end
	function MPT_WIN32_AT_LEAST(v)
		return (MPT_OS_WINDOWS_WINNT or MPT_OS_WINDOWS_WIN9X or MPT_OS_WINDOWS_WIN32) and (MPT_WIN_VERSION >= v)
	end
	function MPT_WIN32_BEFORE(v)
		return (MPT_OS_WINDOWS_WINNT or MPT_OS_WINDOWS_WIN9X or MPT_OS_WINDOWS_WIN32) and (MPT_WIN_VERSION < v)
	end

	if MPT_OS_WINDOWS_WINRT then
		MPT_WIN_AT_LEAST = MPT_WINRT_AT_LEAST
		MPT_WIN_BEFORE = MPT_WINRT_BEFORE
	elseif MPT_OS_WINDOWS_WINNT then
		MPT_WIN_AT_LEAST = MPT_WINNT_AT_LEAST
		MPT_WIN_BEFORE = MPT_WINNT_BEFORE
	elseif MPT_OS_WINDOWS_WIN9X then
		MPT_WIN_AT_LEAST = MPT_WIN9X_AT_LEAST
		MPT_WIN_BEFORE = MPT_WIN9X_BEFORE
	elseif MPT_OS_WINDOWS_WIN32 then
		MPT_WIN_AT_LEAST = MPT_WIN32_AT_LEAST
		MPT_WIN_BEFORE = MPT_WIN32_BEFORE
	else
		function MPT_WIN_AT_LEAST(v)
			return false
		end
		function MPT_WIN_BEFORE(v)
			return true
		end
	end

else

	MPT_OS_WINDOWS = false

	MPT_OS_WINDOWS_WINRT = false
	MPT_OS_WINDOWS_WINNT = false
	MPT_OS_WINDOWS_WIN9X = false
	MPT_OS_WINDOWS_WIN32 = false

	function MPT_WINRT_AT_LEAST(v)
		return false
	end
	function MPT_WINRT_BEFORE(v)
		return false
	end
	function MPT_WINNT_AT_LEAST(v)
		return false
	end
	function MPT_WINNT_BEFORE(v)
		return false
	end
	function MPT_WIN9X_AT_LEAST(v)
		return false
	end
	function MPT_WIN9X_BEFORE(v)
		return false
	end
	function MPT_WIN32_AT_LEAST(v)
		return false
	end
	function MPT_WIN32_BEFORE(v)
		return false
	end

	function MPT_WIN_AT_LEAST(v)
		return false
	end
	function MPT_WIN_BEFORE(v)
		return false
	end

end





function MPT_WIN_PLATFORMS(v)
	if MPT_WIN_AT_LEAST(MPT_WIN["11"]) then
		if MPT_MSVC_AT_LEAST(2019) then
			return { "x86", "x86_64", "arm64", "arm64ec" }
		else
			return { "x86", "x86_64", "arm64" }
		end
	elseif MPT_WIN_AT_LEAST(MPT_WIN["10_1709"]) then
		if MPT_MSVC_BEFORE(2026) then
			return { "x86", "x86_64", "arm", "arm64" }
		else
			return { "x86", "x86_64", "arm64" }
		end
	elseif MPT_WIN_AT_LEAST(MPT_WIN["8"]) then
		if MPT_MSVC_BEFORE(2026) then
			return { "x86", "x86_64", "arm" }
		else
			return { "x86", "x86_64" }
		end
	elseif MPT_WIN_AT_LEAST(MPT_WIN["XP64"]) then
		return { "x86", "x86_64" }
	elseif MPT_WIN_AT_LEAST(MPT_WIN["XP"]) then
		return { "x86" }
	else
		return { "x86_64" }
	end
end

mpt_projectpathname = _ACTION .. _OPTIONS["windows-version"]
mpt_bindirsuffix = _OPTIONS["windows-version"]

if _OPTIONS["windows-version"] == "win11" then
	MPT_WIN_VERSION = MPT_WIN["11_24H2"]
elseif _OPTIONS["windows-version"] == "win10" then
	MPT_WIN_VERSION = MPT_WIN["10_2004"]
elseif _OPTIONS["windows-version"] == "win81" then
	MPT_WIN_VERSION = MPT_WIN["81"]
elseif _OPTIONS["windows-version"] == "win8" then
	MPT_WIN_VERSION = MPT_WIN["8"]
elseif _OPTIONS["windows-version"] == "win7" then
	MPT_WIN_VERSION = MPT_WIN["7"]
elseif _OPTIONS["windows-version"] == "winxpx64" then
	MPT_WIN_VERSION = MPT_WIN["XP64SP2"]
elseif _OPTIONS["windows-version"] == "winxp" then
	MPT_WIN_VERSION = MPT_WIN["XPSP3"]
end

allplatforms = MPT_WIN_PLATFORMS(MPT_WIN_VERSION)
if _OPTIONS["clang"] then
	local clangplatforms = {}
	for i, platform in ipairs(allplatforms) do
		if platform ~= "arm64ec" then
			table.insert(clangplatforms, platform)
		end
	end
	allplatforms = clangplatforms
end
if MPT_OS_WINDOWS_WINRT then
	local uwpplatforms = {}
	for i, platform in ipairs(allplatforms) do
		if platform ~= "arm64ec" then
			table.insert(uwpplatforms, platform)
		end
	end
	allplatforms = uwpplatforms
end

if MPT_OS_WINDOWS_WINRT then
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



if MPT_BUILD_MSBUILD then
	dependencyincludedirs = includedirs
else
	dependencyincludedirs = externalincludedirs
end



dofile "../../build/premake/premake-defaults.lua"



include_dependency = include

solution "all"

	startproject "OpenMPT"

	include "../../build/premake/sys-dmo.lua"
	include "../../build/premake/sys-mfc.lua"
	include "../../build/premake/ext-pthread-win32.lua"
	include "../../build/premake/ext-SignalsmithStretch.lua"
	include "../../build/premake/ext-UnRAR.lua"
	include "../../build/premake/ext-ancient.lua"
	include "../../build/premake/ext-asiomodern.lua"
	include "../../build/premake/ext-cryptopp.lua"
	include "../../build/premake/ext-ogg.lua"
	include "../../build/premake/ext-flac.lua"
	include "../../build/premake/ext-lame.lua"
	include "../../build/premake/ext-lhasa.lua"
	include "../../build/premake/ext-minimp3.lua"
	include "../../build/premake/ext-miniz.lua"
	include "../../build/premake/ext-zlib.lua"
	include "../../build/premake/ext-minizip.lua"
	include "../../build/premake/ext-mpg123.lua"
	include "../../build/premake/ext-nlohmann-json.lua"
	include "../../build/premake/ext-opus.lua"
	include "../../build/premake/ext-opusenc.lua"
	include "../../build/premake/ext-opusfile.lua"
	include "../../build/premake/ext-portaudio.lua"
	include "../../build/premake/ext-portaudiocpp.lua"
	include "../../build/premake/ext-pugixml.lua"
	include "../../build/premake/ext-r8brain.lua"
	include "../../build/premake/ext-rtaudio.lua"
	include "../../build/premake/ext-rtmidi.lua"
	include "../../build/premake/ext-stb_vorbis.lua"
	include "../../build/premake/ext-vorbis.lua"
	include "../../build/premake/ext-vst.lua"
	include "../../build/premake/ext-winamp.lua"
	include "../../build/premake/ext-xmplay.lua"
	include "../../build/premake/mpt-libopenmpt.lua"
	include "../../build/premake/mpt-libopenmpt-small.lua"
	include "../../build/premake/mpt-libopenmpt_test.lua"
if not MPT_OS_WINDOWS_WINRT then
	include "../../build/premake/mpt-libopenmpt_examples.lua"
end
	include "../../build/premake/mpt-openmpt123.lua"
if not MPT_OS_WINDOWS_WINRT then
	include "../../build/premake/mpt-in_openmpt.lua"
	include "../../build/premake/mpt-in_openmpt_wa2.lua"
	include "../../build/premake/mpt-xmp-openmpt.lua"
end
if not MPT_OS_WINDOWS_WINRT then
	include "../../build/premake/mpt-updatesigntool.lua"
	include "../../build/premake/mpt-OpenMPT-NativeSupport.lua"
	include "../../build/premake/mpt-OpenMPT-WineWrapper.lua"
	include "../../build/premake/mpt-PluginBridge.lua"
	include "../../build/premake/mpt-OpenMPT.lua"
	include "../../build/premake/mpt-OpenMPT-UTF8.lua"
	include "../../build/premake/mpt-OpenMPT-ANSI.lua"
end



include_dependency = includeexternal



if not MPT_OS_WINDOWS_WINRT then

solution "libopenmpt_test"
	startproject "libopenmpt_test"

 includeexternal "../../build/premake/mpt-libopenmpt_test.lua"

end



if not MPT_OS_WINDOWS_WINRT then

solution "in_openmpt"
	startproject "in_openmpt"

 includeexternal "../../build/premake/mpt-in_openmpt.lua"
 includeexternal "../../build/premake/mpt-in_openmpt_wa2.lua"

end



if not MPT_OS_WINDOWS_WINRT then

solution "xmp-openmpt"
	startproject "xmp-openmpt"

 includeexternal "../../build/premake/mpt-xmp-openmpt.lua"

end



solution "libopenmpt-small"
	startproject "libopenmpt-small"

 includeexternal "../../build/premake/mpt-libopenmpt-small.lua"



solution "libopenmpt"
	startproject "libopenmpt"

 includeexternal "../../build/premake/mpt-libopenmpt.lua"
if not MPT_OS_WINDOWS_WINRT then
 includeexternal "../../build/premake/mpt-libopenmpt_examples.lua"
end


solution "openmpt123"
	startproject "openmpt123"

 includeexternal "../../build/premake/mpt-openmpt123.lua"



if not MPT_OS_WINDOWS_WINRT then

solution "PluginBridge"
	startproject "PluginBridge"

 includeexternal "../../build/premake/mpt-PluginBridge.lua"

end



if not MPT_OS_WINDOWS_WINRT then

solution "OpenMPT-UTF8"
	startproject "OpenMPT-UTF8"

 includeexternal "../../build/premake/mpt-updatesigntool.lua"
 includeexternal "../../build/premake/mpt-PluginBridge.lua"
 includeexternal "../../build/premake/mpt-OpenMPT-NativeSupport.lua"
 includeexternal "../../build/premake/mpt-OpenMPT-WineWrapper.lua"
 includeexternal "../../build/premake/mpt-OpenMPT-UTF8.lua"

solution "OpenMPT-ANSI"
	startproject "OpenMPT-ANSI"

 includeexternal "../../build/premake/mpt-updatesigntool.lua"
 includeexternal "../../build/premake/mpt-PluginBridge.lua"
 includeexternal "../../build/premake/mpt-OpenMPT-NativeSupport.lua"
 includeexternal "../../build/premake/mpt-OpenMPT-WineWrapper.lua"
 includeexternal "../../build/premake/mpt-OpenMPT-ANSI.lua"

solution "OpenMPT"
	startproject "OpenMPT"

 includeexternal "../../build/premake/mpt-updatesigntool.lua"
 includeexternal "../../build/premake/mpt-PluginBridge.lua"
 includeexternal "../../build/premake/mpt-OpenMPT-NativeSupport.lua"
 includeexternal "../../build/premake/mpt-OpenMPT-WineWrapper.lua"
 includeexternal "../../build/premake/mpt-OpenMPT.lua"

end



if MPT_OS_WINDOWS_WINRT then

	require('vstudio')

	local function mptGlobalsUWP(prj)
		if _ACTION == 'vs2026' then
			premake.w('<DefaultLanguage>en-US</DefaultLanguage>')
			premake.w('<MinimumVisualStudioVersion>15.0</MinimumVisualStudioVersion>')
			premake.w('<AppContainerApplication>true</AppContainerApplication>')
			premake.w('<ApplicationType>Windows Store</ApplicationType>')
			premake.w('<ApplicationTypeRevision>10.0</ApplicationTypeRevision>')
			premake.w('<WindowsTargetPlatformVersion Condition=" \'$(WindowsTargetPlatformVersion)\' == \'\' ">10.0.26100.0</WindowsTargetPlatformVersion>')
			if MPT_WIN_AT_LEAST(MPT_WIN["11"]) then
				premake.w('<WindowsTargetPlatformMinVersion>10.0.22631.0</WindowsTargetPlatformMinVersion>')
			elseif MPT_WIN_AT_LEAST(MPT_WIN["10"]) then
				premake.w('<WindowsTargetPlatformMinVersion>10.0.19045.0</WindowsTargetPlatformMinVersion>')
			end
		elseif _ACTION == 'vs2022' then
			premake.w('<DefaultLanguage>en-US</DefaultLanguage>')
			premake.w('<MinimumVisualStudioVersion>15.0</MinimumVisualStudioVersion>')
			premake.w('<AppContainerApplication>true</AppContainerApplication>')
			premake.w('<ApplicationType>Windows Store</ApplicationType>')
			premake.w('<ApplicationTypeRevision>10.0</ApplicationTypeRevision>')
			if MPT_WIN_AT_LEAST(MPT_WIN["11"]) then
				premake.w('<WindowsTargetPlatformVersion Condition=" \'$(WindowsTargetPlatformVersion)\' == \'\' ">10.0.26100.0</WindowsTargetPlatformVersion>')
				premake.w('<WindowsTargetPlatformMinVersion>10.0.22631.0</WindowsTargetPlatformMinVersion>')
			elseif MPT_WIN_AT_LEAST(MPT_WIN["10"]) then
				premake.w('<WindowsTargetPlatformVersion Condition=" \'$(WindowsTargetPlatformVersion)\' == \'\' ">10.0.22621.0</WindowsTargetPlatformVersion>')
				premake.w('<WindowsTargetPlatformMinVersion>10.0.19045.0</WindowsTargetPlatformMinVersion>')
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
