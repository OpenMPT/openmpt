/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_ENDIAN_INT24_HPP
#define MPT_ENDIAN_INT24_HPP



#include "mpt/base/bit.hpp"
#include "mpt/base/macros.hpp"
#include "mpt/base/memory.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/endian/integer.hpp"

#include <array>
#include <limits>
#include <type_traits>

#include <cstddef>



namespace mpt {
inline namespace MPT_INLINE_NS {



struct uint24 {
	std::array<std::byte, 3> bytes;
	uint24() = default;
	template <typename T, typename std::enable_if<std::is_integral<T>::value, bool>::type = true>
	explicit uint24(T other) noexcept {
		using Tunsigned = typename std::make_unsigned<T>::type;
		MPT_MAYBE_CONSTANT_IF (mpt::endian_is_big()) {
			bytes[0] = mpt::byte_cast<std::byte>(static_cast<uint8>((static_cast<Tunsigned>(other) >> 16) & 0xff));
			bytes[1] = mpt::byte_cast<std::byte>(static_cast<uint8>((static_cast<Tunsigned>(other) >> 8) & 0xff));
			bytes[2] = mpt::byte_cast<std::byte>(static_cast<uint8>((static_cast<Tunsigned>(other) >> 0) & 0xff));
		} else {
			bytes[0] = mpt::byte_cast<std::byte>(static_cast<uint8>((static_cast<Tunsigned>(other) >> 0) & 0xff));
			bytes[1] = mpt::byte_cast<std::byte>(static_cast<uint8>((static_cast<Tunsigned>(other) >> 8) & 0xff));
			bytes[2] = mpt::byte_cast<std::byte>(static_cast<uint8>((static_cast<Tunsigned>(other) >> 16) & 0xff));
		}
	}
	operator unsigned int() const noexcept {
		MPT_MAYBE_CONSTANT_IF (mpt::endian_is_big()) {
			return (mpt::byte_cast<uint8>(bytes[0]) * 65536) + (mpt::byte_cast<uint8>(bytes[1]) * 256) + mpt::byte_cast<uint8>(bytes[2]);
		} else {
			return (mpt::byte_cast<uint8>(bytes[2]) * 65536) + (mpt::byte_cast<uint8>(bytes[1]) * 256) + mpt::byte_cast<uint8>(bytes[0]);
		}
	}
	friend bool operator==(uint24 a, uint24 b) noexcept {
		return static_cast<unsigned int>(a) == static_cast<unsigned int>(b);
	}
	friend bool operator!=(uint24 a, uint24 b) noexcept {
		return static_cast<unsigned int>(a) != static_cast<unsigned int>(b);
	}
};

static_assert(sizeof(uint24) == 3);


struct int24 {
	std::array<std::byte, 3> bytes;
	int24() = default;
	template <typename T, typename std::enable_if<std::is_integral<T>::value, bool>::type = true>
	explicit int24(T other) noexcept {
		using Tunsigned = typename std::make_unsigned<T>::type;
		MPT_MAYBE_CONSTANT_IF (mpt::endian_is_big()) {
			bytes[0] = mpt::byte_cast<std::byte>(static_cast<uint8>((static_cast<Tunsigned>(other) >> 16) & 0xff));
			bytes[1] = mpt::byte_cast<std::byte>(static_cast<uint8>((static_cast<Tunsigned>(other) >> 8) & 0xff));
			bytes[2] = mpt::byte_cast<std::byte>(static_cast<uint8>((static_cast<Tunsigned>(other) >> 0) & 0xff));
		} else {
			bytes[0] = mpt::byte_cast<std::byte>(static_cast<uint8>((static_cast<Tunsigned>(other) >> 0) & 0xff));
			bytes[1] = mpt::byte_cast<std::byte>(static_cast<uint8>((static_cast<Tunsigned>(other) >> 8) & 0xff));
			bytes[2] = mpt::byte_cast<std::byte>(static_cast<uint8>((static_cast<Tunsigned>(other) >> 16) & 0xff));
		}
	}
	operator int() const noexcept {
		MPT_MAYBE_CONSTANT_IF (mpt::endian_is_big()) {
			return (static_cast<int8>(mpt::byte_cast<uint8>(bytes[0])) * 65536) + (mpt::byte_cast<uint8>(bytes[1]) * 256) + mpt::byte_cast<uint8>(bytes[2]);
		} else {
			return (static_cast<int8>(mpt::byte_cast<uint8>(bytes[2])) * 65536) + (mpt::byte_cast<uint8>(bytes[1]) * 256) + mpt::byte_cast<uint8>(bytes[0]);
		}
	}
	friend bool operator==(int24 a, int24 b) noexcept {
		return static_cast<int>(a) == static_cast<int>(b);
	}
	friend bool operator!=(int24 a, int24 b) noexcept {
		return static_cast<int>(a) != static_cast<int>(b);
	}
};

static_assert(sizeof(int24) == 3);



template <>
struct packed_int_type<int24> {
	using type = int;
};
template <>
struct packed_int_type<uint24> {
	using type = unsigned int;
};

using int24le = packed<int24, mpt::endian::little>;
using uint24le = packed<uint24, mpt::endian::little>;

using int24be = packed<int24, mpt::endian::big>;
using uint24be = packed<uint24, mpt::endian::big>;

constexpr bool declare_binary_safe(const int24le &) {
	return true;
}
constexpr bool declare_binary_safe(const uint24le &) {
	return true;
}

constexpr bool declare_binary_safe(const int24be &) {
	return true;
}
constexpr bool declare_binary_safe(const uint24be &) {
	return true;
}

static_assert(mpt::check_binary_size<int24le>(3));
static_assert(mpt::check_binary_size<uint24le>(3));

static_assert(mpt::check_binary_size<int24be>(3));
static_assert(mpt::check_binary_size<uint24be>(3));



} // namespace MPT_INLINE_NS
} // namespace mpt



#if !defined(CPPCHECK)
// work-around crash in cppcheck 2.4.1
namespace std {
template <>
class numeric_limits<mpt::uint24> : public std::numeric_limits<mpt::uint32> {
public:
	static constexpr mpt::uint32 min() noexcept {
		return 0;
	}
	static constexpr mpt::uint32 lowest() noexcept {
		return 0;
	}
	static constexpr mpt::uint32 max() noexcept {
		return 0x00ffffff;
	}
};
template <>
class numeric_limits<mpt::int24> : public std::numeric_limits<mpt::int32> {
public:
	static constexpr mpt::int32 min() noexcept {
		return 0 - 0x00800000;
	}
	static constexpr mpt::int32 lowest() noexcept {
		return 0 - 0x00800000;
	}
	static constexpr mpt::int32 max() noexcept {
		return 0 + 0x007fffff;
	}
};
} // namespace std
#endif // !CPPCHECK



#endif // MPT_ENDIAN_INT24_HPP
