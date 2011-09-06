/*
 * Version related stuff.
 *
 */

#ifndef _VERSION_H
#define _VERSION_H

//STRINGIZE makes a string of given argument. If used with #defined value,
//the string is made of the contents of the defined value.
#define HELPER_STRINGIZE(x)			#x
#define STRINGIZE(x)				HELPER_STRINGIZE(x)

//Version definitions. The only thing that needs to be changed when changing version number.
#define VER_MAJORMAJOR				1
#define VER_MAJOR					19
#define VER_MINOR					04
#define VER_MINORMINOR				00

//Creates version number from version parts that appears in version string.
//For example MAKE_VERSION_NUMERIC(1,17,02,28) gives version number of 
//version 1.17.02.28. 
#define MAKE_VERSION_NUMERIC(v0,v1,v2,v3) ((0x##v0 << 24) + (0x##v1<<16) + (0x##v2<<8) + (0x##v3))

//Version string. For example "1.17.02.28"
#define MPT_VERSION_STR				STRINGIZE(VER_MAJORMAJOR)"."STRINGIZE(VER_MAJOR)"."STRINGIZE(VER_MINOR)"."STRINGIZE(VER_MINORMINOR)

//Numerical value of the version.
#define MPT_VERSION_NUMERIC			MAKE_VERSION_NUMERIC(VER_MAJORMAJOR,VER_MAJOR,VER_MINOR,VER_MINORMINOR)

namespace MptVersion
{
	typedef uint32 VersionNum;
	const VersionNum num = MPT_VERSION_NUMERIC;
	const char* const str = MPT_VERSION_STR;

	// Returns numerical version value from given version string.
	static VersionNum ToNum(const char* const s)
	{
		int v1, v2, v3, v4; 
		sscanf(s, "%x.%x.%x.%x", &v1, &v2, &v3, &v4);
		return ((v1 << 24) +  (v2 << 16) + (v3 << 8) + v4);
	};

	// Returns version string from given numerical version value.
	static CString ToStr(const VersionNum v)
	{
		CString strVersion;
		if(v == 0)
			strVersion = "Unknown";
		else
			strVersion.Format("%X.%02X.%02X.%02X", (v>>24)&0xFF, (v>>16)&0xFF, (v>>8)&0xFF, (v)&0xFF);
		return strVersion;
	};

	// Return a version without build number (the last number in the version).
	// The current versioning scheme uses this number only for test builds, and it should be 00 for official builds,
	// So sometimes it might be wanted to do comparisons without the build number.
	static VersionNum RemoveBuildNumber(const VersionNum num)
	{
		return (num & 0xFFFFFF00);
	};

	// Returns true if a given version number is from a test build, false if it's a release build.
	static bool IsTestBuild(const VersionNum num)
	{
		return ((num > MAKE_VERSION_NUMERIC(1,17,02,54) && num < MAKE_VERSION_NUMERIC(1,18,02,00)) || (num > MAKE_VERSION_NUMERIC(1,18,02,00) && RemoveBuildNumber(num) != num));
	}
}; //namespace MptVersion



#endif // _VERSION_H
