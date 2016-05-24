/*
 * mptOS.h
 * -------
 * Purpose: Operating system version information.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once


OPENMPT_NAMESPACE_BEGIN


namespace mpt
{
namespace Windows
{

class Version
{

public:

	enum Number
	{

		// Win9x
		Win98    = 0x0410,
		WinME    = 0x0490,

		// WinNT
		WinNT4   = 0x0400,
		Win2000  = 0x0500,
		WinXP    = 0x0501,
		WinVista = 0x0600,
		Win7     = 0x0601,
		Win8     = 0x0602,
		Win81    = 0x0603,
		Win10    = 0x0a00,

		WinNewer = Win10 + 1

	};

	static mpt::ustring VersionToString(uint16 version);

private:

	bool SystemIsWindows;

	bool SystemIsNT;
	uint32 SystemVersion;

private:

	Version();

public:

	static mpt::Windows::Version Current();

public:

	bool IsWindows() const;

	bool IsBefore(mpt::Windows::Version::Number version) const;
	bool IsAtLeast(mpt::Windows::Version::Number version) const;

	bool Is9x() const;
	bool IsNT() const;

	mpt::ustring GetName() const;

public:

	static uint16 GetMinimumKernelLevel();
	static uint16 GetMinimumAPILevel();

}; // class Version

#if defined(MODPLUG_TRACKER)

void PreventWineDetection();

bool IsOriginal();
bool IsWine();

#endif // MODPLUG_TRACKER

} // namespace Windows
} // namespace mpt


#if defined(MODPLUG_TRACKER)

namespace mpt
{

namespace Wine
{

std::string RawGetVersion();
std::string RawGetBuildID();
std::string RawGetHostSysName();
std::string RawGetHostRelease();

class Version
{
private:
	bool valid;
	uint8 vmajor;
	uint8 vminor;
	uint8 vupdate;
public:
	Version();
	Version(uint8 vmajor, uint8 vminor, uint8 vupdate);
	explicit Version(const mpt::ustring &version);
public:
	bool IsValid() const;
	mpt::ustring AsString() const;
private:
	static mpt::Wine::Version FromInteger(uint32 version);
	uint32 AsInteger() const;
public:
	bool IsBefore(mpt::Wine::Version other) const;
	bool IsAtLeast(mpt::Wine::Version other) const;
};

mpt::Wine::Version GetVersion();

bool HostIsLinux();

} // namespace Wine

} // namespace mpt

#endif // MODPLUG_TRACKER


OPENMPT_NAMESPACE_END
