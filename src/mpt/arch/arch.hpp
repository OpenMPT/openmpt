/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_ARCH_ARCH_HPP
#define MPT_ARCH_ARCH_HPP


#include "mpt/arch/feature_flags.hpp"
#include "mpt/arch/x86_amd64.hpp"
#include "mpt/base/detect.hpp"
#include "mpt/base/integer.hpp"
#include "mpt/base/macros.hpp"
#include "mpt/base/namespace.hpp"

#include <string>



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace arch {



namespace unknown {

struct no_feature_flags {
	constexpr no_feature_flags() noexcept = default;
	constexpr no_feature_flags operator~() noexcept {
		return no_feature_flags{};
	}
	constexpr no_feature_flags & operator&=(no_feature_flags) noexcept {
		return *this;
	}
	constexpr no_feature_flags & operator|=(no_feature_flags) noexcept {
		return *this;
	}
	constexpr no_feature_flags & operator^=(no_feature_flags) noexcept {
		return *this;
	}
	friend constexpr no_feature_flags operator&(no_feature_flags, no_feature_flags) noexcept {
		return no_feature_flags{};
	}
	friend constexpr no_feature_flags operator|(no_feature_flags, no_feature_flags) noexcept {
		return no_feature_flags{};
	}
	friend constexpr no_feature_flags operator^(no_feature_flags, no_feature_flags) noexcept {
		return no_feature_flags{};
	}
	friend constexpr bool operator==(no_feature_flags, no_feature_flags) noexcept {
		return true;
	}
	friend constexpr bool operator!=(no_feature_flags, no_feature_flags) noexcept {
		return false;
	}
	constexpr bool supports(no_feature_flags) noexcept {
		return true;
	}
	explicit constexpr operator bool() const noexcept {
		return true;
	}
	constexpr bool operator!() const noexcept {
		return false;
	}
};

using feature_flags = mpt::arch::basic_feature_flags<no_feature_flags>;

namespace feature {
inline constexpr feature_flags none = feature_flags{};
} // namespace feature

struct cpu_info {
public:
	cpu_info() = default;
public:
	MPT_CONSTEXPRINLINE bool operator[](feature_flags) const noexcept {
		return true;
	}
	MPT_CONSTEXPRINLINE bool has_features(feature_flags) const noexcept {
		return true;
	}
	MPT_CONSTEXPRINLINE feature_flags get_features() const noexcept {
		return {};
	}
};

[[nodiscard]] MPT_CONSTEVAL feature_flags assumed_features() noexcept {
	return {};
}

} // namespace unknown



#if MPT_ARCH_X86
namespace current = x86;
#elif MPT_ARCH_AMD64
namespace current = amd64;
#else
namespace current = unknown;
#endif

using cpu_info = mpt::arch::current::cpu_info;

inline const cpu_info & get_cpu_info() {
	static cpu_info info;
	return info;
}

namespace detail {

struct info_initializer {
	inline info_initializer() noexcept {
		get_cpu_info();
	}
};

inline info_initializer g_info_initializer;

} // namespace detail

struct feature_flags_cache {
private:
	const mpt::arch::current::feature_flags Features;
public:
	MPT_CONSTEXPRINLINE feature_flags_cache(const mpt::arch::cpu_info & info) noexcept
		: Features(info.get_features()) {
		return;
	}
	[[nodiscard]] MPT_CONSTEXPRINLINE bool operator[](mpt::arch::current::feature_flags query_features) const noexcept {
		return ((Features & query_features) == query_features);
	}
	[[nodiscard]] MPT_CONSTEXPRINLINE bool has_features(mpt::arch::current::feature_flags query_features) const noexcept {
		return ((Features & query_features) == query_features);
	}
	[[nodiscard]] MPT_CONSTEXPRINLINE mpt::arch::current::feature_flags get_features() const noexcept {
		return Features;
	}
};



} // namespace arch



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_ARCH_ARCH_HPP
