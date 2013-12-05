
#pragma once

#define OPENMPT_VERSION_URL ""

#define OPENMPT_VERSION_DATE __DATE__ " " __TIME__

#ifdef BUILD_PACKAGE
#define OPENMPT_VERSION_IS_PACKAGE true
#else
#define OPENMPT_VERSION_IS_PACKAGE false
#endif

#ifndef BUILD_SVNVERSION

#define OPENMPT_VERSION_REVISION 0

#define OPENMPT_VERSION_DIRTY false
#define OPENMPT_VERSION_MIXEDREVISIONS false

#else // BUILD_SVNVERSION

#include <locale>
#include <sstream>
#include <string>

#define OPENMPT_VERSION_REVISION mpt::svnversion::parse_svnversion_to_revision( BUILD_SVNVERSION )

#define OPENMPT_VERSION_DIRTY mpt::svnversion::parse_svnversion_to_modified( BUILD_SVNVERSION )
#define OPENMPT_VERSION_MIXEDREVISIONS mpt::svnversion::parse_svnversion_to_mixed_revisions( BUILD_SVNVERSION )

namespace mpt
{
namespace svnversion
{

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
	int revision = 0;
	std::istringstream s( svnversion );
	s.imbue(std::locale::classic());
	s >> revision;
	return revision;
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

} // namespace svnversion
} // namespace mpt

#endif // !BUILD_SVNVERSION else BUILD_SVNVERSION
