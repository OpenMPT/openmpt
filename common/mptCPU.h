/*
 * mptCPU.h
 * --------
 * Purpose: CPU feature detection.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "mpt/base/macros.hpp"
#ifdef MPT_ENABLE_ARCH_INTRINSICS
#include "mpt/arch/arch.hpp"
#endif


OPENMPT_NAMESPACE_BEGIN


namespace CPU
{


#ifdef MPT_ENABLE_ARCH_INTRINSICS





#if defined(MODPLUG_TRACKER) && !defined(MPT_BUILD_WINESUPPORT)


namespace detail
{

inline MPT_CONSTINIT mpt::arch::current::feature_flags EnabledFeatures;

} // namespace detail

inline void EnableAvailableFeatures() noexcept
{
	CPU::detail::EnabledFeatures = mpt::arch::get_cpu_info().get_features();
}

// enabled processor features for inline asm and intrinsics
MPT_FORCEINLINE [[nodiscard]] mpt::arch::current::feature_flags GetEnabledFeatures() noexcept
{
	return CPU::detail::EnabledFeatures;
}

struct Info
{
	MPT_FORCEINLINE [[nodiscard]] bool HasFeatureSet(mpt::arch::current::feature_flags features) const noexcept
	{
		return features == (GetEnabledFeatures() & features);
	}
};


#else // !MODPLUG_TRACKER


struct Info
{
private:
	const mpt::arch::feature_flags_cache m_features{mpt::arch::get_cpu_info()};
public:
	MPT_FORCEINLINE [[nodiscard]] bool HasFeatureSet(mpt::arch::current::feature_flags features) const noexcept
	{
		return m_features[features];
	}
};


#endif // MODPLUG_TRACKER





// legacy interface

namespace feature = mpt::arch::current::feature;

MPT_FORCEINLINE [[nodiscard]] bool HasFeatureSet(mpt::arch::current::feature_flags features) noexcept
{
	return CPU::Info{}.HasFeatureSet(features);
}





#endif // MPT_ENABLE_ARCH_INTRINSICS


} // namespace CPU


OPENMPT_NAMESPACE_END
