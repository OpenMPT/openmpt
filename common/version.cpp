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

#include "svn_version.h"

namespace MptVersion {

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
	return OPENMPT_VERSION_URL;
}

int GetRevision()
{
	return OPENMPT_VERSION_REVISION;
}

bool IsDirty()
{
	return OPENMPT_VERSION_DIRTY;
}

bool HasMixedRevisions()
{
	return OPENMPT_VERSION_MIXEDREVISIONS;
}

std::string GetStateString()
{
	std::string retval;
	if(OPENMPT_VERSION_MIXEDREVISIONS)
	{
		retval += "+mixed";
	}
	if(OPENMPT_VERSION_DIRTY)
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
	if(OPENMPT_VERSION_REVISION == 0)
	{
		return "";
	}
	std::ostringstream str;
	str << "-r" << OPENMPT_VERSION_REVISION;
	if(OPENMPT_VERSION_MIXEDREVISIONS)
	{
		str << "!";
	}
	if(OPENMPT_VERSION_DIRTY)
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
	if(OPENMPT_VERSION_REVISION == 0)
	{
		return "";
	}
	std::string url = OPENMPT_VERSION_URL;
	std::string baseurl = "https://svn.code.sf.net/p/modplug/code/";
	if(url.substr(0, baseurl.length()) == baseurl)
	{
		url = url.substr(baseurl.length());
	}
	std::ostringstream str;
	str << url << "@" << OPENMPT_VERSION_REVISION << GetStateString();
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
