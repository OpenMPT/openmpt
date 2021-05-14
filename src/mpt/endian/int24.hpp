/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_ENDIAN_INT24_HPP
#define MPT_ENDIAN_INT24_HPP



#include "mpt/base/bit.hpp"
#include "mpt/base/macros.hpp"
#include "mpt/base/memory.hpp"
#include "mpt/base/namespace.hpp"

#include <array>

#include <cstddef>



namespace mpt {
inline namespace MPT_INLINE_NS {



struct int24 {
	std::array<std::byte, 3> bytes;
	int24() = default;
	explicit int24(int other) noexcept {
		MPT_MAYBE_CONSTANT_IF(mpt::endian_is_big()) {
			bytes[0] = mpt::byte_cast<std::byte>(static_cast<uint8>((static_cast<unsigned int>(other) >> 16) & 0xff));
			bytes[1] = mpt::byte_cast<std::byte>(static_cast<uint8>((static_cast<unsigned int>(other) >> 8) & 0xff));
			bytes[2] = mpt::byte_cast<std::byte>(static_cast<uint8>((static_cast<unsigned int>(other) >> 0) & 0xff));
		}
		else {
			bytes[0] = mpt::byte_cast<std::byte>(static_cast<uint8>((static_cast<unsigned int>(other) >> 0) & 0xff));
			bytes[1] = mpt::byte_cast<std::byte>(static_cast<uint8>((static_cast<unsigned int>(other) >> 8) & 0xff));
			bytes[2] = mpt::byte_cast<std::byte>(static_cast<uint8>((static_cast<unsigned int>(other) >> 16) & 0xff));
		}
	}
	operator int() const noexcept {
		MPT_MAYBE_CONSTANT_IF(mpt::endian_is_big()) {
			return (static_cast<int8>(mpt::byte_cast<uint8>(bytes[0])) * 65536) + (mpt::byte_cast<uint8>(bytes[1]) * 256) + mpt::byte_cast<uint8>(bytes[2]);
		}
		else {
			return (static_cast<int8>(mpt::byte_cast<uint8>(bytes[2])) * 65536) + (mpt::byte_cast<uint8>(bytes[1]) * 256) + mpt::byte_cast<uint8>(bytes[0]);
		}
	}
};

static_assert(sizeof(int24) == 3);



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_ENDIAN_INT24_HPP
