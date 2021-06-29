/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_CRYPTO_EXCEPTION_HPP
#define MPT_CRYPTO_EXCEPTION_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/out_of_memory/out_of_memory.hpp"

#include <stdexcept>

#if MPT_OS_WINDOWS
#include <windows.h>  // must be before wincrypt.h for clang-cl
#include <wincrypt.h> // must be before ncrypt.h
#include <ncrypt.h>
#endif // MPT_OS_WINDOWS


namespace mpt {
inline namespace MPT_INLINE_NS {


namespace crypto {



#if MPT_OS_WINDOWS

class exception
	: public std::runtime_error {
private:
	NTSTATUS m_Status;

public:
	exception(NTSTATUS status)
		: std::runtime_error("crypto error")
		, m_Status(status) {
		return;
	}

public:
	NTSTATUS code() const noexcept {
		return m_Status;
	}
};


class security_exception
	: public std::runtime_error {
private:
	SECURITY_STATUS m_Status;

public:
	security_exception(SECURITY_STATUS status)
		: std::runtime_error("crypto error")
		, m_Status(status) {
		return;
	}

public:
	SECURITY_STATUS code() const noexcept {
		return m_Status;
	}
};


inline void CheckNTSTATUS(NTSTATUS status) {
	if (status >= 0) {
		return;
	} else if (static_cast<DWORD>(status) == STATUS_NO_MEMORY) {
		mpt::throw_out_of_memory();
	} else {
		throw exception(status);
	}
}


inline void CheckSECURITY_STATUS(SECURITY_STATUS status) {
	if (status == ERROR_SUCCESS) {
		return;
	} else if (status == NTE_NO_MEMORY) {
		mpt::throw_out_of_memory();
	} else {
		throw security_exception(status);
	}
}

#endif // MPT_OS_WINDOWS



} // namespace crypto


} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_CRYPTO_EXCEPTION_HPP
