#ifndef SIGNALSMITH_DSP_COMMON_H
#define SIGNALSMITH_DSP_COMMON_H

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

namespace signalsmith {
	/**	@defgroup Common Common
		@brief Definitions and helper classes used by the rest of the library
		
		@{
		@file
	*/

#define SIGNALSMITH_DSP_VERSION_MAJOR 1
#define SIGNALSMITH_DSP_VERSION_MINOR 6
#define SIGNALSMITH_DSP_VERSION_PATCH 0
#define SIGNALSMITH_DSP_VERSION_STRING "1.6.0"

	/** Version compatability check.
	\code{.cpp}
		static_assert(signalsmith::version(1, 4, 1), "version check");
	\endcode
	... or use the equivalent `SIGNALSMITH_DSP_VERSION_CHECK`.
	Major versions are not compatible with each other.  Minor and patch versions are backwards-compatible.
	*/
	constexpr bool versionCheck(int major, int minor, int patch=0) {
		return major == SIGNALSMITH_DSP_VERSION_MAJOR
			&& (SIGNALSMITH_DSP_VERSION_MINOR > minor
				|| (SIGNALSMITH_DSP_VERSION_MINOR == minor && SIGNALSMITH_DSP_VERSION_PATCH >= patch));
	}

/// Check the library version is compatible (semver).
#define SIGNALSMITH_DSP_VERSION_CHECK(major, minor, patch) \
	static_assert(::signalsmith::versionCheck(major, minor, patch), "signalsmith library version is " SIGNALSMITH_DSP_VERSION_STRING);

/** @} */
} // signalsmith::
#else
// If we've already included it, check it's the same version
static_assert(SIGNALSMITH_DSP_VERSION_MAJOR == 1 && SIGNALSMITH_DSP_VERSION_MINOR == 6 && SIGNALSMITH_DSP_VERSION_PATCH == 0, "multiple versions of the Signalsmith DSP library");
#endif // include guard
