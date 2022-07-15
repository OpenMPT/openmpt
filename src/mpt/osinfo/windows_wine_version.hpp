/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_OSINFO_WINDOWS_WINE_VERSION_HPP
#define MPT_OSINFO_WINDOWS_WINE_VERSION_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/integer.hpp"
#include "mpt/base/namespace.hpp"

#if MPT_OS_WINDOWS
#include <windows.h>
#endif // MPT_OS_WINDOWS



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace osinfo {

namespace windows {



namespace wine {

class version {
protected:
	bool valid = false;
	uint8 vmajor = 0;
	uint8 vminor = 0;
	uint8 vupdate = 0;

public:
	version() {
		return;
	}
	version(uint8 vmajor_, uint8 vminor_, uint8 vupdate_)
		: valid(true)
		, vmajor(vmajor_)
		, vminor(vminor_)
		, vupdate(vupdate_) {
		return;
	}

public:
	bool IsValid() const {
		return valid;
	}

private:
	static mpt::osinfo::windows::wine::version FromInteger(uint32 version) {
		mpt::osinfo::windows::wine::version result;
		result.valid = (version <= 0xffffff);
		result.vmajor = static_cast<uint8>(version >> 16);
		result.vminor = static_cast<uint8>(version >> 8);
		result.vupdate = static_cast<uint8>(version >> 0);
		return result;
	}
	uint32 AsInteger() const {
		uint32 version = 0;
		version |= static_cast<uint32>(vmajor) << 16;
		version |= static_cast<uint32>(vminor) << 8;
		version |= static_cast<uint32>(vupdate) << 0;
		return version;
	}

public:
	bool IsBefore(mpt::osinfo::windows::wine::version other) const {
		if (!IsValid()) {
			return false;
		}
		return (AsInteger() < other.AsInteger());
	}
	bool IsAtLeast(mpt::osinfo::windows::wine::version other) const {
		if (!IsValid()) {
			return false;
		}
		return (AsInteger() >= other.AsInteger());
	}
	uint8 GetMajor() const {
		return vmajor;
	}
	uint8 GetMinor() const {
		return vminor;
	}
	uint8 GetUpdate() const {
		return vupdate;
	}
};

} // namespace wine



} // namespace windows

} // namespace osinfo



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_OSINFO_WINDOWS_WINE_VERSION_HPP
