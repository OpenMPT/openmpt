/*
 * version.cpp
 * -----------
 * Purpose: OpenMPT version handling.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"
#include "version.h"

#include "mptString.h"
#include "mptStringFormat.h"
#include "mptStringParse.h" 

#include "versionNumber.h"
#include "svn_version.h"

OPENMPT_NAMESPACE_BEGIN

namespace MptVersion {

static_assert((MPT_VERSION_NUMERIC & 0xffff) != 0x0000, "Version numbers ending in .00.00 shall never exist again, as they make interpreting the version number ambiguous for file formats which can only store the two major parts of the version number (e.g. IT and S3M).");

const VersionNum num = MPT_VERSION_NUMERIC;

const char * const str = MPT_VERSION_STR;

std::string GetOpenMPTVersionStr()
{
	return std::string("OpenMPT " MPT_VERSION_STR);
}

VersionNum ToNum(const std::string &s)
{
	VersionNum result = 0;
	std::vector<std::string> numbers = mpt::String::Split<std::string>(s, std::string("."));
	for(std::size_t i = 0; i < numbers.size() && i < 4; ++i)
	{
		result |= (mpt::String::Parse::Hex<unsigned int>(numbers[i]) & 0xff) << ((3-i)*8);
	}
	return result;

}

std::string ToStr(const VersionNum v)
{
	if(v == 0)
	{
		// Unknown version
		return "Unknown";
	} else if((v & 0xFFFF) == 0)
	{
		// Only parts of the version number are known (e.g. when reading the version from the IT or S3M file header)
		return mpt::format("%1.%2")(mpt::fmt::HEX((v >> 24) & 0xFF), mpt::fmt::HEX0<2>((v >> 16) & 0xFF));
	} else
	{
		// Full version info available
		return mpt::format("%1.%2.%3.%4")(mpt::fmt::HEX((v >> 24) & 0xFF), mpt::fmt::HEX0<2>((v >> 16) & 0xFF), mpt::fmt::HEX0<2>((v >> 8) & 0xFF), mpt::fmt::HEX0<2>((v) & 0xFF));
	}
}

VersionNum RemoveBuildNumber(const VersionNum num_)
{
	return (num_ & 0xFFFFFF00);
}

bool IsTestBuild(const VersionNum num_)
{
	return (
			// Legacy
			(num_ > MAKE_VERSION_NUMERIC(1,17,02,54) && num_ < MAKE_VERSION_NUMERIC(1,18,02,00) && num_ != MAKE_VERSION_NUMERIC(1,18,00,00))
		||
			// Test builds have non-zero VER_MINORMINOR
			(num_ > MAKE_VERSION_NUMERIC(1,18,02,00) && RemoveBuildNumber(num_) != num_)
		);
}

bool IsDebugBuild()
{
	#ifdef _DEBUG
		return true;
	#else
		return false;
	#endif
}

static std::string GetUrl()
{
	#ifdef OPENMPT_VERSION_URL
		return OPENMPT_VERSION_URL;
	#else
		return "";
	#endif
}

static int GetRevision()
{
	#if defined(OPENMPT_VERSION_REVISION)
		return OPENMPT_VERSION_REVISION;
	#elif defined(OPENMPT_VERSION_SVNVERSION)
		std::string svnversion = OPENMPT_VERSION_SVNVERSION;
		if(svnversion.length() == 0)
		{
			return 0;
		}
		if(svnversion.find(":") != std::string::npos)
		{
			svnversion = svnversion.substr(svnversion.find(":") + 1);
		}
		if(svnversion.find("-") != std::string::npos)
		{
			svnversion = svnversion.substr(svnversion.find("-") + 1);
		}
		if(svnversion.find("M") != std::string::npos)
		{
			svnversion = svnversion.substr(0, svnversion.find("M"));
		}
		if(svnversion.find("S") != std::string::npos)
		{
			svnversion = svnversion.substr(0, svnversion.find("S"));
		}
		if(svnversion.find("P") != std::string::npos)
		{
			svnversion = svnversion.substr(0, svnversion.find("P"));
		}
		return ConvertStrTo<int>(svnversion);
	#else
		#if MPT_COMPILER_MSVC
			#pragma message("SVN revision unknown. Please check your build system.")
		#elif MPT_COMPILER_GCC || MPT_COMPILER_CLANG
			#warning "SVN revision unknown. Please check your build system."
		#else
			// There is no portable way to display a warning.
			// Try to provoke a warning with an unused variable.
			int SVN_revision_unknown__Please_check_your_build_system;
		#endif
		return 0;
	#endif
}

static bool IsDirty()
{
	#if defined(OPENMPT_VERSION_DIRTY)
		return OPENMPT_VERSION_DIRTY != 0;
	#elif defined(OPENMPT_VERSION_SVNVERSION)
		std::string svnversion = OPENMPT_VERSION_SVNVERSION;
		if(svnversion.length() == 0)
		{
			return false;
		}
		if(svnversion.find("M") != std::string::npos)
		{
			return true;
		}
		return false;
	#else
		return false;
	#endif
}

static bool HasMixedRevisions()
{
	#if defined(OPENMPT_VERSION_MIXEDREVISIONS)
		return OPENMPT_VERSION_MIXEDREVISIONS != 0;
	#elif defined(OPENMPT_VERSION_SVNVERSION)
		std::string svnversion = OPENMPT_VERSION_SVNVERSION;
		if(svnversion.length() == 0)
		{
			return false;
		}
		if(svnversion.find(":") != std::string::npos)
		{
			return true;
		}
		if(svnversion.find("-") != std::string::npos)
		{
			return true;
		}
		if(svnversion.find("S") != std::string::npos)
		{
			return true;
		}
		if(svnversion.find("P") != std::string::npos)
		{
			return true;
		}
		return false;
	#else
		return false;
	#endif
}

static bool IsPackage()
{
	#if defined(OPENMPT_VERSION_IS_PACKAGE)
		return OPENMPT_VERSION_IS_PACKAGE != 0;
	#else
		return false;
	#endif
}

static std::string GetSourceDate()
{
	#if defined(OPENMPT_VERSION_DATE)
		return OPENMPT_VERSION_DATE;
	#else
		return "";
	#endif
}

SourceInfo GetSourceInfo()
{
	SourceInfo result;
	result.Url = GetUrl();
	result.Revision = GetRevision();
	result.IsDirty = IsDirty();
	result.HasMixedRevisions = HasMixedRevisions();
	result.IsPackage = IsPackage();
	result.Date = GetSourceDate();
	return result;
}

std::string SourceInfo::GetStateString() const
{
	std::string retval;
	if(IsDirty)
	{
		retval += "+dirty";
	}
	if(HasMixedRevisions)
	{
		retval += "+mixed";
	}
	if(retval.empty())
	{
		retval += "clean";
	}
	if(IsPackage)
	{
		retval += "-pkg";
	}
	return retval;
}

std::string GetBuildDateString()
{
	#ifdef MODPLUG_TRACKER
		#if defined(OPENMPT_BUILD_DATE)
			return OPENMPT_BUILD_DATE;
		#else
			return __DATE__ " " __TIME__ ;
		#endif
	#else // !MODPLUG_TRACKER
		return GetSourceInfo().Date;
	#endif // MODPLUG_TRACKER
}

static std::string GetBuildFlagsString()
{
	std::string retval;
	#ifdef MODPLUG_TRACKER
		if(IsTestBuild())
		{
			retval += " TEST";
		}
	#endif // MODPLUG_TRACKER
	if(IsDebugBuild())
	{
		retval += " DEBUG";
	}
	return retval;
}

std::string GetBuildFeaturesString()
{
	std::string retval;
	#ifdef LIBOPENMPT_BUILD
		#if MPT_COMPILER_GENERIC
			retval += "*C++11";
		#elif MPT_COMPILER_MSVC
			retval += mpt::format("*MSVC-%1.%2")(MPT_COMPILER_MSVC_VERSION / 100, MPT_COMPILER_MSVC_VERSION % 100);
		#elif MPT_COMPILER_GCC
			retval += mpt::format("*GCC-%1.%2.%3")(MPT_COMPILER_GCC_VERSION / 10000, (MPT_COMPILER_GCC_VERSION / 100) % 100, MPT_COMPILER_GCC_VERSION % 100);
		#elif MPT_COMPILER_CLANG
			retval += mpt::format("*Clang-%1.%2.%3")(MPT_COMPILER_CLANG_VERSION / 10000, (MPT_COMPILER_CLANG_VERSION / 100) % 100, MPT_COMPILER_CLANG_VERSION % 100);
		#else
			retval += "*unknown";
		#endif
		#if defined(MPT_CHARSET_WIN32)
			retval += " +WINAPI";
		#endif
		#if defined(MPT_CHARSET_ICONV)
			retval += " +ICONV";
		#endif
		#if defined(MPT_CHARSET_CODECVTUTF8)
			retval += " +CODECVTUTF8";
		#endif
		#if defined(MPT_CHARSET_INTERNAL)
			retval += " +INTERNALCHARSETS";
		#endif
		#if defined(MPT_WITH_ZLIB)
			retval += " +ZLIB";
		#endif
		#if defined(MPT_WITH_MINIZ)
			retval += " +MINIZ";
		#endif
		#if !defined(MPT_WITH_ZLIB) && !defined(MPT_WITH_MINIZ)
			retval += " -INFLATE";
		#endif
		#if defined(MPT_WITH_MPG123)
			retval += " +MPG123";
		#elif defined(MPT_ENABLE_MPG123_DYNBIND)
			retval += " +MPG123-DYNBIND";
		#endif
		#if defined(MPT_WITH_MINIMP3)
			retval += " +MINIMP3";
		#endif
		#if defined(MPT_WITH_MEDIAFOUNDATION)
			retval += " +MF";
		#endif
		#if !defined(MPT_WITH_MPG123) && !defined(MPT_ENABLE_MPG123_DYNBIND) && !defined(MPT_WITH_MINIMP3) && !defined(MPT_WITH_MEDIAFOUNDATION)
			retval += " -MP3";
		#endif
		#if defined(MPT_WITH_OGG) && defined(MPT_WITH_VORBIS) && defined(MPT_WITH_VORBISFILE)
			retval += " +VORBIS";
		#endif
		#if defined(MPT_WITH_STBVORBIS)
			retval += " +STBVORBIS";
		#endif
		#if !(defined(MPT_WITH_OGG) && defined(MPT_WITH_VORBIS) && defined(MPT_WITH_VORBISFILE)) && !defined(MPT_WITH_STB_VORBIS)
			retval += " -VORBIS";
		#endif
		#if defined(MPT_ENABLE_MO3_BUILTIN)
			retval += " +MO3";
		#endif
		#if defined(MPT_WITH_UNMO3)
			retval += " +UNMO3";
		#elif defined(MPT_ENABLE_UNMO3_DYNBIND)
			retval += " +UNMO3-DYNBIND";
		#endif
		#if !defined(MPT_WITH_UNMO3) && !defined(MPT_ENABLE_UNMO3_DYNBIND) && !defined(MPT_ENABLE_MO3_BUILTIN)
			retval += " -MO3";
		#endif
	#endif
	#ifdef MODPLUG_TRACKER
		if(IsForOlderWindows())
		{
			retval += " OLDWIN";
		}
		#ifdef NO_VST
			retval += " NO_VST";
		#endif
		#ifdef NO_DMO
			retval += " NO_DMO";
		#endif
		#ifdef NO_PLUGINS
			retval += " NO_PLUGINS";
		#endif
		#ifndef MPT_WITH_ASIO
			retval += " NO_ASIO";
		#endif
		#ifndef MPT_WITH_DSOUND
			retval += " NO_DSOUND";
		#endif
	#endif
	return retval;
}

std::string GetBuildCompilerString()
{
	std::string retval;
	#if MPT_COMPILER_GENERIC
		retval += "*Generic C++11 Compiler";
	#elif MPT_COMPILER_MSVC
		#if defined(_MSC_FULL_VER) && defined(_MSC_BUILD) && (_MSC_BUILD > 0)
			retval += mpt::format("Microsoft Compiler %1.%2.%3.%4")
				( _MSC_FULL_VER / 10000000
				, mpt::fmt::dec0<2>((_MSC_FULL_VER / 100000) % 100)
				, mpt::fmt::dec0<5>(_MSC_FULL_VER % 100000)
				, mpt::fmt::dec0<2>(_MSC_BUILD)
				);
		#elif defined(_MSC_FULL_VER)
			retval += mpt::format("Microsoft Compiler %1.%2.%3")
				( _MSC_FULL_VER / 10000000
				, mpt::fmt::dec0<2>((_MSC_FULL_VER / 100000) % 100)
				, mpt::fmt::dec0<5>(_MSC_FULL_VER % 100000)
				);
		#else
			retval += mpt::format("Microsoft Compiler %1.%2")(MPT_COMPILER_MSVC_VERSION / 100, MPT_COMPILER_MSVC_VERSION % 100);
		#endif
	#elif MPT_COMPILER_GCC
		retval += mpt::format("GNU Compiler Collection %1.%2.%3")(MPT_COMPILER_GCC_VERSION / 10000, (MPT_COMPILER_GCC_VERSION / 100) % 100, MPT_COMPILER_GCC_VERSION % 100);
	#elif MPT_COMPILER_CLANG
		retval += mpt::format("Clang %1.%2.%3")(MPT_COMPILER_CLANG_VERSION / 10000, (MPT_COMPILER_CLANG_VERSION / 100) % 100, MPT_COMPILER_CLANG_VERSION % 100);
	#else
		retval += "*unknown";
	#endif
	return retval;
}

static std::string GetRevisionString()
{
	std::string result;
	if(GetRevision() == 0)
	{
		return result;
	}
	result = std::string("-r") + mpt::ToString(GetRevision());
	if(HasMixedRevisions())
	{
		result += "!";
	}
	if(IsDirty())
	{
		result += "+";
	}
	if(IsPackage())
	{
		result += "p";
	}
	return result;
}

bool IsForOlderWindows()
{
	#ifdef MODPLUG_TRACKER
		return true
			&& (GetMinimumSSEVersion() <= 0)
			&& (GetMinimumAVXVersion() <= 0)
			&& (mpt::Windows::Version::GetMinimumKernelLevel() < mpt::Windows::Version::WinXP)
			&& (mpt::Windows::Version::GetMinimumAPILevel() < mpt::Windows::Version::WinXP)
			;
	#else
		return false;
	#endif
}

mpt::ustring GetDownloadURL()
{
	#ifdef MODPLUG_TRACKER
		return (MptVersion::IsDebugBuild() || MptVersion::IsTestBuild() || MptVersion::IsDirty() || MptVersion::HasMixedRevisions())
			?
				MPT_USTRING("https://buildbot.openmpt.org/builds/")
			:
				MPT_USTRING("https://openmpt.org/download")
			;
	#else
		return MPT_USTRING("https://lib.openmpt.org/");
	#endif
}

static std::string GetVersionString(bool verbose)
{
	#if !defined(MODPLUG_TRACKER)
		MPT_UNREFERENCED_PARAMETER(verbose);
	#endif
	std::string retval = MPT_VERSION_STR;
	if(IsDebugBuild() || IsTestBuild() || IsDirty() || HasMixedRevisions())
	{
		retval += GetRevisionString();
	}
	#ifdef MODPLUG_TRACKER
		if(verbose)
		{
			retval += mpt::format(" %1 bit")(sizeof(void*) * 8);
		}
	#endif
	#ifndef MODPLUG_TRACKER
		if(verbose)
		{
			const SourceInfo sourceInfo = GetSourceInfo();
			if(!sourceInfo.GetUrlWithRevision().empty())
			{
				retval += mpt::format(" %1")(sourceInfo.GetUrlWithRevision());
			}
			if(!sourceInfo.Date.empty())
			{
				retval += mpt::format(" (%1)")(sourceInfo.Date);
			}
			if(!sourceInfo.GetStateString().empty())
			{
				retval += mpt::format(" %1")(sourceInfo.GetStateString());
			}
		}
	#endif
	if(IsDebugBuild() || IsTestBuild() || IsDirty() || HasMixedRevisions())
	{
		retval += GetBuildFlagsString();
	}
	#ifdef MODPLUG_TRACKER
		if(verbose)
		{
			retval += GetBuildFeaturesString();
		}
	#endif
	return retval;
}

std::string GetVersionStringSimple()
{
	return GetVersionString(false);
}

std::string GetVersionStringExtended()
{
	return GetVersionString(true);
}

std::string SourceInfo::GetUrlWithRevision() const
{
	if(Url.empty() || (Revision == 0))
	{
		return std::string();
	}
	return Url + "@" + mpt::ToString(Revision);
}

mpt::ustring GetContactString()
{
	return MPT_UTF8(
		"Contact / Discussion:\n"
		"https://forum.openmpt.org/\n"
		"\n"
		"Updates:\n"
		"https://openmpt.org/download\n"
		);
}

mpt::ustring GetFullCreditsString()
{
	return mpt::ToUnicode(mpt::CharsetUTF8,
#ifdef MODPLUG_TRACKER
		"OpenMPT / ModPlug Tracker\n"
#else
		"libopenmpt (based on OpenMPT / ModPlug Tracker)\n"
#endif
		"\n"
		"Copyright \xC2\xA9 2004-2016 Contributors\n"
		"Copyright \xC2\xA9 1997-2003 Olivier Lapicque\n"
		"\n"
		"Contributors:\n"
		"Johannes Schultz (2008-2016)\n"
		"J\xC3\xB6rn Heusipp (2012-2016)\n"
		"Ahti Lepp\xC3\xA4nen (2005-2011)\n"
		"Robin Fernandes (2004-2007)\n"
		"Sergiy Pylypenko (2007)\n"
		"Eric Chavanon (2004-2005)\n"
		"Trevor Nunes (2004)\n"
		"Olivier Lapicque (1997-2003)\n"
		"\n"
		"Additional patch submitters:\n"
		"coda (http://coda.s3m.us/)\n"
		"kode54 (https://kode54.net/)\n"
		"xaimus (http://xaimus.com/)\n"
		"\n"
		"Thanks to:\n"
		"\n"
		"Konstanty for the XMMS-ModPlug resampling implementation\n"
		"http://modplug-xmms.sourceforge.net/\n"
		"\n"
#ifdef MODPLUG_TRACKER
		"Stephan M. Bernsee for pitch shifting source code\n"
		"http://www.dspdimension.com/\n"
		"\n"
		"Aleksey Vaneev of Voxengo for r8brain sample rate converter\n"
		"https://github.com/avaneev/r8brain-free-src\n"
		"\n"
		"Olli Parviainen for SoundTouch Library (time stretching)\n"
		"http://www.surina.net/soundtouch/\n"
		"\n"
#endif
#ifndef NO_VST
		"Hermann Seib for his example VST Host implementation\n"
		"http://www.hermannseib.com/english/vsthost.htm\n"
		"\n"
#endif
#if defined(MPT_WITH_UNMO3) || defined(MPT_ENABLE_UNMO3_DYNBIND)
		"Ian Luck for UNMO3\n"
		"http://www.un4seen.com/mo3.html\n"
		"\n"
#endif
#ifdef MPT_ENABLE_MO3_BUILTIN
		"Laurent Cl\xc3\xA9vy for unofficial MO3 documentation and decompression code\n"
		"https://github.com/lclevy/unmo3\n"
		"\n"
#endif
		"Ben \"GreaseMonkey\" Russell for IT sample compression code\n"
		"https://github.com/iamgreaser/it2everything/\n"
		"\n"
#if MPT_COMPILER_MSVC
		"Alexander Chemeris for msinttypes\n"
		"https://github.com/chemeris/msinttypes\n"
		"\n"
#endif
#ifdef MPT_WITH_ZLIB
		"Jean-loup Gailly and Mark Adler for zlib\n"
		"http://zlib.net/\n"
		"\n"
#endif
#ifdef MPT_WITH_MINIZ
		"Rich Geldreich for miniz\n"
		"https://github.com/richgel999/miniz\n"
		"\n"
#endif
#ifdef MPT_WITH_LHASA
		"Simon Howard for lhasa\n"
		"https://fragglet.github.io/lhasa/\n"
		"\n"
#endif
#ifdef MPT_WITH_UNRAR
		"Alexander L. Roshal for UnRAR\n"
		"http://rarlab.com/\n"
		"\n"
#endif
#ifdef MPT_WITH_PORTAUDIO
		"PortAudio contributors\n"
		"http://www.portaudio.com/\n"
		"\n"
#endif
#ifdef MPT_WITH_FLAC
		"Josh Coalson / Xiph.Org Foundation for libFLAC\n"
		"https://xiph.org/flac/\n"
		"\n"
#endif
#if defined(MPT_WITH_MPG123) || defined(MPT_ENABLE_MPG123_DYNBIND)
		"The mpg123 project for libmpg123\n"
		"http://mpg123.de/\n"
		"\n"
#endif
#ifdef MPT_WITH_MINIMP3
		"Fabrice Bellard, FFMPEG contributors\n"
		"and Martin J. Fiedler (KeyJ/kakiarts) for minimp3\n"
		"http://keyj.emphy.de/minimp3/\n"
		"\n"
#endif
#ifdef MPT_WITH_STBVORBIS
		"Sean Barrett for stb_vorbis\n"
		"https://github.com/nothings/stb/\n"
		"\n"
#endif
#ifdef MPT_WITH_OGG
		"Xiph.Org Foundation for libogg\n"
		"https://xiph.org/ogg/\n"
		"\n"
#endif
#if defined(MPT_WITH_VORBIS) || defined(MPT_WITH_LIBVORBISFILE)
		"Xiph.Org Foundation for libvorbis\n"
		"https://xiph.org/vorbis/\n"
		"\n"
#endif
		"Storlek for all the IT compatibility hints and testcases\n"
		"as well as the IMF, OKT and ULT loaders\n"
		"http://schismtracker.org/\n"
		"\n"
#ifdef MODPLUG_TRACKER
		"Alexander Uckun for decimal input field\n"
		"http://www.codeproject.com/Articles/21257/_\n"
		"\n"
		"Nobuyuki for application and file icon\n"
		"https://twitter.com/nobuyukinyuu\n"
		"\n"
#endif
		"The people at ModPlug forums for crucial contribution\n"
		"in the form of ideas, testing and support;\n"
		"thanks particularly to:\n"
		"33, 8bitbubsy, Anboi, BooT-SectoR-ViruZ, Bvanoudtshoorn\n"
		"christofori, cubaxd, Diamond, Ganja, Georg, Goor00,\n"
		"Harbinger, jmkz, KrazyKatz, LPChip, Nofold, Rakib, Sam Zen\n"
		"Skaven, Skilletaudio, Snu, Squirrel Havoc, Waxhead\n"
		"\n"
#ifndef NO_VST
		"VST PlugIn Technology by Steinberg Media Technologies GmbH\n"
		"\n"
#endif
#ifdef MPT_WITH_ASIO
		"ASIO Technology by Steinberg Media Technologies GmbH\n"
		"\n"
#endif
		);
}

mpt::ustring GetLicenseString()
{
	return MPT_UTF8(
		"The OpenMPT code is licensed under the BSD license." "\n"
		"" "\n"
		"Copyright (c) 2004-2016, OpenMPT contributors" "\n"
		"Copyright (c) 1997-2003, Olivier Lapicque" "\n"
		"All rights reserved." "\n"
		"" "\n"
		"Redistribution and use in source and binary forms, with or without" "\n"
		"modification, are permitted provided that the following conditions are met:" "\n"
		"    * Redistributions of source code must retain the above copyright" "\n"
		"      notice, this list of conditions and the following disclaimer." "\n"
		"    * Redistributions in binary form must reproduce the above copyright" "\n"
		"      notice, this list of conditions and the following disclaimer in the" "\n"
		"      documentation and/or other materials provided with the distribution." "\n"
		"    * Neither the name of the OpenMPT project nor the" "\n"
		"      names of its contributors may be used to endorse or promote products" "\n"
		"      derived from this software without specific prior written permission." "\n"
		"" "\n"
		"THIS SOFTWARE IS PROVIDED BY THE CONTRIBUTORS ``AS IS'' AND ANY" "\n"
		"EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED" "\n"
		"WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE" "\n"
		"DISCLAIMED. IN NO EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY" "\n"
		"DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES" "\n"
		"(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;" "\n"
		"LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND" "\n"
		"ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT" "\n"
		"(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS" "\n"
		"SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE." "\n"
		);
}

} // namespace MptVersion

OPENMPT_NAMESPACE_END
