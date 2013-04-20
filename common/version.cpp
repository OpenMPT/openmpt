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

} // namespace MptVersion
