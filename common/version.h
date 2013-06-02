/*
 * version.h
 * ---------
 * Purpose: OpenMPT version handling.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include <string>


//Creates version number from version parts that appears in version string.
//For example MAKE_VERSION_NUMERIC(1,17,02,28) gives version number of 
//version 1.17.02.28. 
#define MAKE_VERSION_NUMERIC_HELPER(prefix,v0,v1,v2,v3) ((prefix##v0 << 24) | (prefix##v1<<16) | (prefix##v2<<8) | (prefix##v3))
#define MAKE_VERSION_NUMERIC(v0,v1,v2,v3) MAKE_VERSION_NUMERIC_HELPER(0x,v0,v1,v2,v3)


namespace MptVersion
{

	typedef uint32 VersionNum;

	extern const VersionNum num; // e.g. 0x01170208
	extern const char * const str; // e.g "1.17.02.08"

	// Return a OpenMPT version string suitable for file format tags 
	std::string GetOpenMPTVersionStr(); // e.g. "OpenMPT 1.17.02.08"

	// Returns numerical version value from given version string.
	VersionNum ToNum(const std::string &s);

	// Returns version string from given numerical version value.
	std::string ToStr(const VersionNum v);

	// Return a version without build number (the last number in the version).
	// The current versioning scheme uses this number only for test builds, and it should be 00 for official builds,
	// So sometimes it might be wanted to do comparisons without the build number.
	VersionNum RemoveBuildNumber(const VersionNum num);

	// Returns true if a given version number is from a test build, false if it's a release build.
	bool IsTestBuild(const VersionNum num = MptVersion::num);

	// Return true if this is a debug build with no optimizations
	bool IsDebugBuild();

	// Return the svn repository url (if built from a svn working copy and tsvn was available during build)
	std::string GetUrl();

	// Return the svn revision (if built from a svn working copy and tsvn was available during build)
	int GetRevision();

	// Return if the svn working copy had local changes during build (if built from a svn working copy and tsvn was available during build)
	bool IsDirty();

	// Return if the svn working copy had files checked out from different revisions and/or branches (if built from a svn working copy and tsvn was available during build)
	bool HasMixedRevisions();

	// Return a string decribing the working copy state (dirty and/or mixed revisions) (if built from a svn working copy and tsvn was available during build)
	std::string GetStateString(); // e.g. "" or "+mixed" or "+mixed+dirty" or "+dirty"

	// Return a string decribing the time of the build process (if built from a svn working copy and tsvn was available during build, otherwise it returns the time version.cpp was last rebuild which could be unreliable as it does not get rebuild every time without tsvn)
	std::string GetBuildDateString();

	// Return a string decribing some of the buidl features and/or flags
	std::string GetBuildFlagsString(); // e.g. " TEST DEBUG NO_VST"

	// Return a string decribing the revision of the svn working copy and if it was dirty (+) or had mixed revisions (!) (if built from a svn working copy and tsvn was available during build)
	std::string GetRevisionString(); // e.g. "-r1234+"

	// Returns MptVersion::str if the build is a clean release build straight from the repository or an extended strin otherwise (if built from a svn working copy and tsvn was available during build)
	std::string GetVersionStringExtended(); // e.g. "1.17.02.08-r1234+ DEBUG"

	// Returns a string combining the repository url and the revision, suitable for checkout if the working copy was clean (if built from a svn working copy and tsvn was available during build)
	std::string GetVersionUrlString(); // e.g. "https://svn.code.sf.net/p/modplug/code/trunk/OpenMPT@1234+dirty"

	// Returns a multi-line string containing the full credits for the code base
	std::string GetFullCreditsString();

	// Returns a multi-line string containing developer contact and community addresses
	std::string GetContactString();

} //namespace MptVersion
