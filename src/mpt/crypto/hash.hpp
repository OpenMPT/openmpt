/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_CRYPTO_HASH_HPP
#define MPT_CRYPTO_HASH_HPP



#include "mpt/base/array.hpp"
#include "mpt/base/detect.hpp"
#include "mpt/base/memory.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/base/saturate_cast.hpp"
#include "mpt/crypto/exception.hpp"

#include <algorithm>
#include <vector>

#include <cassert>
#include <cstddef>

#if MPT_OS_WINDOWS
#include <windows.h> // must be before wincrypt.h for clang-cl
#include <bcrypt.h>
#endif // MPT_OS_WINDOWS


namespace mpt {
inline namespace MPT_INLINE_NS {


namespace crypto {



#if MPT_OS_WINDOWS


namespace hash {


struct hash_traits_md5 {
	static constexpr std::size_t output_bits = 128;
	static constexpr std::size_t output_bytes = output_bits / 8;
	static constexpr const wchar_t * bcrypt_name = BCRYPT_MD5_ALGORITHM;
};

struct hash_traits_sha1 {
	static constexpr std::size_t output_bits = 160;
	static constexpr std::size_t output_bytes = output_bits / 8;
	static constexpr const wchar_t * bcrypt_name = BCRYPT_SHA1_ALGORITHM;
};

struct hash_traits_sha256 {
	static constexpr std::size_t output_bits = 256;
	static constexpr std::size_t output_bytes = output_bits / 8;
	static constexpr const wchar_t * bcrypt_name = BCRYPT_SHA256_ALGORITHM;
};

struct hash_traits_sha512 {
	static constexpr std::size_t output_bits = 512;
	static constexpr std::size_t output_bytes = output_bits / 8;
	static constexpr const wchar_t * bcrypt_name = BCRYPT_SHA512_ALGORITHM;
};

template <typename Traits>
class hash_impl {

public:
	using traits = Traits;

	using result_type = std::array<std::byte, traits::output_bytes>;

private:
	BCRYPT_ALG_HANDLE hAlg = NULL;
	std::vector<BYTE> hashState;
	std::vector<BYTE> hashResult;
	BCRYPT_HASH_HANDLE hHash = NULL;

private:
	void init() {
		CheckNTSTATUS(BCryptOpenAlgorithmProvider(&hAlg, traits::bcrypt_name, NULL, 0));
		if (!hAlg) {
			throw exception(0);
		}
		DWORD hashStateSize = 0;
		DWORD hashStateSizeSize = 0;
		CheckNTSTATUS(BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&hashStateSize, sizeof(DWORD), &hashStateSizeSize, 0));
		if (hashStateSizeSize != sizeof(DWORD)) {
			throw exception(0);
		}
		if (hashStateSize <= 0) {
			throw exception(0);
		}
		hashState.resize(hashStateSize);
		DWORD hashResultSize = 0;
		DWORD hashResultSizeSize = 0;
		CheckNTSTATUS(BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, (PBYTE)&hashResultSize, sizeof(DWORD), &hashResultSizeSize, 0));
		if (hashResultSizeSize != sizeof(DWORD)) {
			throw exception(0);
		}
		if (hashResultSize <= 0) {
			throw exception(0);
		}
		if (hashResultSize != mpt::extent<result_type>()) {
			throw exception(0);
		}
		hashResult.resize(hashResultSize);
		CheckNTSTATUS(BCryptCreateHash(hAlg, &hHash, hashState.data(), hashStateSize, NULL, 0, 0));
		if (!hHash) {
			throw exception(0);
		}
	}

	void cleanup() {
		if (hHash) {
			BCryptDestroyHash(hHash);
			hHash = NULL;
		}
		hashResult.resize(0);
		hashResult.shrink_to_fit();
		hashState.resize(0);
		hashState.shrink_to_fit();
		if (hAlg) {
			BCryptCloseAlgorithmProvider(hAlg, 0);
			hAlg = NULL;
		}
	}

public:
	hash_impl() {
		try {
			init();
		} catch (...) {
			cleanup();
			throw;
		}
	}
	hash_impl(const hash_impl &) = delete;
	hash_impl & operator=(const hash_impl &) = delete;
	~hash_impl() {
		cleanup();
	}

public:
	hash_impl & process(mpt::const_byte_span data) {
		CheckNTSTATUS(BCryptHashData(hHash, const_cast<UCHAR *>(mpt::byte_cast<const UCHAR *>(data.data())), mpt::saturate_cast<ULONG>(data.size()), 0));
		return *this;
	}

	result_type result() {
		result_type res = mpt::init_array<std::byte, traits::output_bytes>(std::byte{0});
		CheckNTSTATUS(BCryptFinishHash(hHash, hashResult.data(), mpt::saturate_cast<ULONG>(hashResult.size()), 0));
		assert(hashResult.size() == mpt::extent<result_type>());
		std::transform(hashResult.begin(), hashResult.end(), res.begin(), [](BYTE b) { return mpt::as_byte(b); });
		return res;
	}
};

using MD5 = hash_impl<hash_traits_md5>;
using SHA1 = hash_impl<hash_traits_sha1>;
using SHA256 = hash_impl<hash_traits_sha256>;
using SHA512 = hash_impl<hash_traits_sha512>;


} // namespace hash


#endif // MPT_OS_WINDOWS



} // namespace crypto


} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_CRYPTO_HASH_HPP
