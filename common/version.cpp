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

#include <sstream>

#include <cstdlib>

#include "versionNumber.h"
#include "svn_version.h"

namespace MptVersion {

const VersionNum num = MPT_VERSION_NUMERIC;

const char * const str = MPT_VERSION_STR;

static int parse_svnversion_to_revision( std::string svnversion )
{
	if(svnversion.length() == 0)
	{
		return 0;
	}
	if(svnversion.find(":") != std::string::npos)
	{
		svnversion = svnversion.substr(svnversion.find(":") + 1);
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
	return std::strtol(svnversion.c_str(), 0, 10);
}

static bool parse_svnversion_to_mixed_revisions( std::string svnversion )
{
	if(svnversion.length() == 0)
	{
		return false;
	}
	if(svnversion.find(":") != std::string::npos)
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
}

static bool parse_svnversion_to_modified( std::string svnversion )
{
	if(svnversion.length() == 0)
	{
		return false;
	}
	if(svnversion.find("M") != std::string::npos)
	{
		return true;
	}
	return false;
}

std::string GetOpenMPTVersionStr()
{
	return std::string("OpenMPT ") + std::string(MPT_VERSION_STR);
}

VersionNum ToNum(const std::string &s_)
{
	const char *s = s_.c_str();
	unsigned int v1, v2, v3, v4; 
	sscanf(s, "%x.%x.%x.%x", &v1, &v2, &v3, &v4);
	return ((v1 << 24) |  (v2 << 16) | (v3 << 8) | v4);
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
		return mpt::String::Format("%X.%02X", (v >> 24) & 0xFF, (v >> 16) & 0xFF);
	} else
	{
		// Full version info available
		return mpt::String::Format("%X.%02X.%02X.%02X", (v >> 24) & 0xFF, (v >> 16) & 0xFF, (v >> 8) & 0xFF, (v) & 0xFF);
	}
}

VersionNum RemoveBuildNumber(const VersionNum num)
{
	return (num & 0xFFFFFF00);
}

bool IsTestBuild(const VersionNum num)
{
	return ((num > MAKE_VERSION_NUMERIC(1,17,02,54) && num < MAKE_VERSION_NUMERIC(1,18,02,00) && num != MAKE_VERSION_NUMERIC(1,18,00,00)) || (num > MAKE_VERSION_NUMERIC(1,18,02,00) && RemoveBuildNumber(num) != num));
}

bool IsDebugBuild()
{
	#ifdef _DEBUG
		return true;
	#else
		return false;
	#endif
}

std::string GetUrl()
{
	#ifdef OPENMPT_VERSION_URL
		return OPENMPT_VERSION_URL;
	#else
		return "";
	#endif
}

int GetRevision()
{
	#if defined(OPENMPT_VERSION_REVISION)
		return OPENMPT_VERSION_REVISION;
	#elif defined(OPENMPT_VERSION_SVNVERSION)
		return parse_svnversion_to_revision(OPENMPT_VERSION_SVNVERSION);
	#else
		return 0;
	#endif
}

bool IsDirty()
{
	#if defined(OPENMPT_VERSION_DIRTY)
		return OPENMPT_VERSION_DIRTY;
	#elif defined(OPENMPT_VERSION_SVNVERSION)
		return parse_svnversion_to_modified(OPENMPT_VERSION_SVNVERSION);
	#else
		return false;
	#endif
}

bool HasMixedRevisions()
{
	#if defined(OPENMPT_VERSION_MIXEDREVISIONS)
		return OPENMPT_VERSION_MIXEDREVISIONS;
	#elif defined(OPENMPT_VERSION_SVNVERSION)
		return parse_svnversion_to_mixed_revisions(OPENMPT_VERSION_SVNVERSION);
	#else
		return false;
	#endif
}

std::string GetStateString()
{
	std::string retval;
	if(HasMixedRevisions())
	{
		retval += "+mixed";
	}
	if(IsDirty())
	{
		retval += "+dirty";
	}
	return retval;
}

std::string GetBuildDateString()
{
	return OPENMPT_VERSION_DATE;
}

std::string GetBuildFlagsString()
{
	std::string retval;
	if(IsTestBuild())
	{
		retval += " TEST";
	}
	if(IsDebugBuild())
	{
		retval += " DEBUG";
	}
	#ifdef MODPLUG_TRACKER
		#ifdef NO_VST
			retval += " NO_VST";
		#endif
		#ifdef NO_ASIO
			retval += " NO_ASIO";
		#endif
		#ifdef NO_DSOUND
			retval += " NO_DSOUND";
		#endif
	#endif
	return retval;
}

std::string GetRevisionString()
{
	if(GetRevision() == 0)
	{
		return "";
	}
	std::ostringstream str;
	str << "-r" << GetRevision();
	if(HasMixedRevisions())
	{
		str << "!";
	}
	if(IsDirty())
	{
		str << "+";
	}
	return str.str();
}

std::string GetVersionStringExtended()
{
	std::string retval = MPT_VERSION_STR;
	if(IsDebugBuild() || IsTestBuild() || IsDirty() || HasMixedRevisions())
	{
		retval += GetRevisionString();
		retval += GetBuildFlagsString();
	}
	return retval;
}

std::string GetVersionUrlString()
{
	if(GetRevision() == 0)
	{
		return "";
	}
	#if defined(OPENMPT_VERSION_URL)
		std::string url = OPENMPT_VERSION_URL;
	#else
		std::string url = "";
	#endif
	std::string baseurl = "https://svn.code.sf.net/p/modplug/code/";
	if(url.substr(0, baseurl.length()) == baseurl)
	{
		url = url.substr(baseurl.length());
	}
	std::ostringstream str;
	str << url << "@" << GetRevision() << GetStateString();
	return str.str();
}

std::string GetContactString()
{
	return "Contact / Discussion:\n"
		"http://forum.openmpt.org/\n"
		"\n"
		"Updates:\n"
		"http://openmpt.org/download";
}

std::string GetFullCreditsString()
{
	return
#ifdef MODPLUG_TRACKER
		"OpenMPT / ModPlug Tracker\n"
#else
		"libopenmpt (based on OpenMPT / ModPlug Tracker)\n"
#endif
		"Copyright © 2004-2013 Contributors\n"
		"Copyright © 1997-2003 Olivier Lapicque\n"
		"\n"
		"Contributors:\n"
		"Johannes Schultz (2008-2013)\n"
		"Ahti Leppänen (2005-2011, 2013)\n"
		"Joern Heusipp (2012-2013)\n"
		"Robin Fernandes (2004-2007)\n"
		"Sergiy Pylypenko (2007)\n"
		"Eric Chavanon (2004-2005)\n"
		"Trevor Nunes (2004)\n"
		"Olivier Lapicque (1997-2003)\n"
		"\n"
		"Additional patch submitters:\n"
		"coda (http://coda.s3m.us/)\n"
		"kode54 (http://kode54.foobar2000.org/)\n"
		"xaimus (http://xaimus.com/)\n"
		"\n"
		"Thanks to:\n"
		"\n"
		"Konstanty for the XMMS-ModPlug resampling implementation\n"
		"http://modplug-xmms.sourceforge.net/\n"
#ifdef MODPLUG_TRACKER
		"Stephan M. Bernsee for pitch shifting source code\n"
		"http://www.dspdimension.com/\n"
#endif
#ifdef MODPLUG_TRACKER
		"Olli Parviainen for SoundTouch Library (time stretching)\n"
		"http://www.surina.net/soundtouch/\n"
#endif
#ifndef NO_VST
		"Hermann Seib for his example VST Host implementation\n"
		"http://www.hermannseib.com/english/vsthost.htm\n"
#endif
#ifndef NO_MO3
		"Ian Luck for UNMO3\n"
		"http://www.un4seen.com/mo3.html\n"
#endif
		"Ben \"GreaseMonkey\" Russell for IT sample compression code\n"
		"https://github.com/iamgreaser/it2everything/\n"
		"Jean-loup Gailly and Mark Adler for zlib\n"
		"http://zlib.net/\n"
#ifndef NO_PORTAUDIO
		"PortAudio contributors\n"
		"http://www.portaudio.com/\n"
#endif
#ifndef NO_FLAC
		"Josh Coalson for libFLAC\n"
		"http://flac.sourceforge.net/\n"
#endif
#ifndef NO_MP3_SAMPLES
		"The mpg123 project for libmpg123\n"
		"http://mpg123.de/\n"
#endif
		"Storlek for all the IT compatibility hints and testcases\n"
		"as well as the IMF, OKT and ULT loaders\n"
		"http://schismtracker.org/\n"
#ifdef MODPLUG_TRACKER
		"Pel K. Txnder for the scrolling credits control :)\n"
		"http://tinyurl.com/4yze8\n"
#endif
		"\n"
		"The people at ModPlug forums for crucial contribution\n"
		"in the form of ideas, testing and support; thanks\n"
		"particularly to:\n"
		"33, Anboi, BooT-SectoR-ViruZ, Bvanoudtshoorn\n"
		"christofori, Diamond, Ganja, Georg, Goor00, jmkz,\n"
		"KrazyKatz, LPChip, Nofold, Rakib, Sam Zen\n"
		"Skaven, Skilletaudio, Snu, Squirrel Havoc, Waxhead\n"
		"\n"
#ifndef NO_VST
		"\n"
		"VST PlugIn Technology by Steinberg Media Technologies GmbH\n"
#endif
#ifndef NO_ASIO
		"\n"
		"ASIO Technology by Steinberg Media Technologies GmbH\n"
#endif
		"\n"
		;

}

} // namespace MptVersion
