/*
 * mptOS.h
 * -------
 * Purpose: Operating system version information.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "mpt/library/library.hpp"
#include "mpt/osinfo/class.hpp"
#include "mpt/osinfo/windows_version.hpp"


OPENMPT_NAMESPACE_BEGIN


namespace mpt
{
namespace OS
{
namespace Windows
{


class Version
	: public mpt::osinfo::windows::Version
{

public:

	static mpt::ustring VersionToString(mpt::OS::Windows::Version::System version);

private:

	Version() noexcept;

public:

	static Version NoWindows() noexcept;

public:

	Version(mpt::osinfo::windows::Version v) noexcept;

public:

	Version(mpt::OS::Windows::Version::System system, mpt::OS::Windows::Version::ServicePack servicePack, mpt::OS::Windows::Version::Build build, mpt::OS::Windows::Version::TypeId type) noexcept;

public:

	static mpt::OS::Windows::Version Current() noexcept;

public:

	mpt::ustring GetName() const;
	mpt::ustring GetNameShort() const;

public:

	static mpt::OS::Windows::Version::System GetMinimumKernelLevel() noexcept;
	static mpt::OS::Windows::Version::System GetMinimumAPILevel() noexcept;

}; // class Version

#if MPT_OS_WINDOWS

enum class Architecture
{
	unknown = -1,

	x86     = 0x0401,
	amd64   = 0x0801,
	arm     = 0x0402,
	arm64   = 0x0802,

	mips    = 0x0403,
	ppc     = 0x0404,
	shx     = 0x0405,

	alpha   = 0x0406,
	alpha64 = 0x0806,

	ia64    = 0x0807,
};

enum class EmulationLevel
{
	Native,
	Virtual,
	Hardware,
	Software,
	NA,
};

int Bitness(Architecture arch) noexcept;

mpt::ustring Name(Architecture arch);

Architecture GetHostArchitecture() noexcept;
Architecture GetProcessArchitecture() noexcept;

EmulationLevel HostCanRun(Architecture host, Architecture process) noexcept;

std::vector<Architecture> GetSupportedProcessArchitectures(Architecture host);

uint64 GetSystemMemorySize();

#endif // MPT_OS_WINDOWS


#if defined(MODPLUG_TRACKER)

void PreventWineDetection();

bool IsOriginal();
bool IsWine();

#endif // MODPLUG_TRACKER

} // namespace Windows
} // namespace OS
} // namespace mpt


namespace mpt
{
namespace OS
{
namespace Wine
{

class Version
	: public mpt::osinfo::windows::wine::version
{
public:
	Version();
	Version(uint8 vmajor, uint8 vminor, uint8 vupdate);
	explicit Version(const mpt::ustring &version);
public:
	mpt::ustring AsString() const;
};

mpt::OS::Wine::Version GetMinimumWineVersion();

class VersionContext
{
protected:
	bool m_IsWine;
	std::string m_RawVersion;
	std::string m_RawBuildID;
	std::string m_RawHostSysName;
	std::string m_RawHostRelease;
	mpt::OS::Wine::Version m_Version;
	mpt::osinfo::osclass m_HostClass;
public:
	VersionContext();
public:
	bool IsWine() const { return m_IsWine; }
	std::string RawVersion() const { return m_RawVersion; }
	std::string RawBuildID() const { return m_RawBuildID; }
	std::string RawHostSysName() const { return m_RawHostSysName; }
	std::string RawHostRelease() const { return m_RawHostRelease; }
	mpt::OS::Wine::Version Version() const { return m_Version; }
	mpt::osinfo::osclass HostClass() const { return m_HostClass; }
};

} // namespace Wine
} // namespace OS
} // namespace mpt


OPENMPT_NAMESPACE_END
