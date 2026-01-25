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

#include "mpt/osinfo/class.hpp"
#include "mpt/osinfo/windows_version.hpp"
#include "mpt/osinfo/windows_wine_version.hpp"
#include "mpt/osinfo_wine/wine.hpp"


OPENMPT_NAMESPACE_BEGIN


namespace mpt
{
namespace OS
{
namespace Windows
{

namespace Version
{

	mpt::ustring GetName(mpt::osinfo::windows::Version version);

	mpt::ustring GetCurrentName();

	mpt::osinfo::windows::Version GetMinimumKernelLevel() noexcept;
	mpt::osinfo::windows::Version GetMinimumAPILevel() noexcept;

} // namespace Version

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

using Version = mpt::osinfo::windows::wine::version;

using VersionContext = mpt::osinfo::windows::wine::VersionContext;

mpt::osinfo::windows::wine::version GetMinimumWineVersion();

} // namespace Wine
} // namespace OS
} // namespace mpt


OPENMPT_NAMESPACE_END
