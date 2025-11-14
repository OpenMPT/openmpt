/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_ARCH_ARCH_HPP
#define MPT_ARCH_ARCH_HPP


#include "mpt/arch/feature_fence.hpp"
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

template <typename tag>
struct no_flags {
	constexpr no_flags() noexcept = default;
	constexpr no_flags operator~() noexcept {
		return no_flags{};
	}
	constexpr no_flags & operator&=(no_flags) noexcept {
		return *this;
	}
	constexpr no_flags & operator|=(no_flags) noexcept {
		return *this;
	}
	constexpr no_flags & operator^=(no_flags) noexcept {
		return *this;
	}
	friend constexpr no_flags operator&(no_flags, no_flags) noexcept {
		return no_flags{};
	}
	friend constexpr no_flags operator|(no_flags, no_flags) noexcept {
		return no_flags{};
	}
	friend constexpr no_flags operator^(no_flags, no_flags) noexcept {
		return no_flags{};
	}
	friend constexpr bool operator==(no_flags, no_flags) noexcept {
		return true;
	}
	friend constexpr bool operator!=(no_flags, no_flags) noexcept {
		return false;
	}
	constexpr bool supports(no_flags) noexcept {
		return true;
	}
	explicit constexpr operator bool() const noexcept {
		return true;
	}
	constexpr bool operator!() const noexcept {
		return false;
	}
};

struct no_feature_flags_tag {
};

struct no_mode_flags_tag {
};

using feature_flags = mpt::arch::basic_feature_flags<no_flags<no_feature_flags_tag>>;
using mode_flags = mpt::arch::basic_feature_flags<no_flags<no_mode_flags_tag>>;

namespace feature {
inline constexpr feature_flags none = feature_flags{};
} // namespace feature

namespace mode {
inline constexpr mode_flags none = mode_flags{};
} // namespace mode

struct cpu_info {
public:
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static cpu_info query() noexcept {
		return cpu_info{};
	}
private:
	cpu_info() = default;
public:
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE constexpr bool operator[](feature_flags) const noexcept {
		return true;
	}
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE constexpr bool has_features(feature_flags) const noexcept {
		return true;
	}
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE constexpr feature_flags get_features() const noexcept {
		return {};
	}
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE constexpr bool operator[](mode_flags) const noexcept {
		return true;
	}
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE constexpr bool enabled_modes(mode_flags) const noexcept {
		return true;
	}
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE constexpr mode_flags get_modes() const noexcept {
		return {};
	}
};

[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_CONSTEVAL feature_flags assumed_features() noexcept {
	return {};
}

[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_CONSTEVAL mode_flags assumed_modes() noexcept {
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

MPT_ATTR_NOINLINE MPT_DECL_NOINLINE inline const cpu_info & get_cpu_info() {
	static cpu_info info = cpu_info::query();
	return info;
}

namespace detail {

struct info_initializer {
	MPT_ATTR_NOINLINE MPT_DECL_NOINLINE inline info_initializer() noexcept {
		get_cpu_info();
	}
};

#if MPT_COMPILER_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wglobal-constructors"
#endif // MPT_COMPILER_CLANG
inline info_initializer g_info_initializer;
#if MPT_COMPILER_CLANG
#pragma clang diagnostic pop
#endif // MPT_COMPILER_CLANG


} // namespace detail



} // namespace arch



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_ARCH_ARCH_HPP
