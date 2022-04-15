/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_ARCH_X86_AMD64_HPP
#define MPT_ARCH_X86_AMD64_HPP


#include "mpt/arch/feature_flags.hpp"
#include "mpt/base/detect.hpp"
#include "mpt/base/integer.hpp"
#include "mpt/base/macros.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/osinfo/windows_version.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <string_view>

#include <cstddef>

#if MPT_OS_DJGPP
#include <dpmi.h>
#include <pc.h>
#endif

#if MPT_OS_WINDOWS
#include <windows.h>
#endif

#if MPT_ARCH_X86 || MPT_ARCH_AMD64
#if MPT_COMPILER_MSVC
#include <intrin.h>
#elif MPT_COMPILER_GCC || MPT_COMPILER_CLANG
#include <cpuid.h>
#include <x86intrin.h>
#endif
#endif

// skip assumed features for amd64
#define MPT_ARCH_X86_AMD64_FAST_DETECT 0



// clang-format off

#if MPT_ARCH_X86 || MPT_ARCH_AMD64

#if MPT_COMPILER_MSVC

#if defined(_M_X64)
	#define MPT_ARCH_X86_I386
	#define MPT_ARCH_X86_FPU
	#define MPT_ARCH_X86_FSIN
	#define MPT_ARCH_X86_I486
	#define MPT_ARCH_X86_CPUID
	#define MPT_ARCH_X86_TSC
	#define MPT_ARCH_X86_CX8
	#define MPT_ARCH_X86_CMOV
	#define MPT_ARCH_X86_MMX
	#define MPT_ARCH_X86_3DNOWPREFETCH
	#define MPT_ARCH_X86_SSE
	#define MPT_ARCH_X86_SSE2
#elif defined(_M_IX86) && defined(_M_IX86_FP)
	#if (_M_IX86_FP >= 2)
		#define MPT_ARCH_X86_I386
		#define MPT_ARCH_X86_FPU
		#define MPT_ARCH_X86_FSIN
		#define MPT_ARCH_X86_I486
		#define MPT_ARCH_X86_CPUID
		#define MPT_ARCH_X86_TSC
		#define MPT_ARCH_X86_CX8
		#define MPT_ARCH_X86_CMOV
		#define MPT_ARCH_X86_MMX
		#define MPT_ARCH_X86_3DNOWPREFETCH
		#define MPT_ARCH_X86_SSE
		#define MPT_ARCH_X86_SSE2
	#elif (_M_IX86_FP == 1)
		#define MPT_ARCH_X86_I386
		#define MPT_ARCH_X86_FPU
		#define MPT_ARCH_X86_FSIN
		#define MPT_ARCH_X86_I486
		#define MPT_ARCH_X86_CPUID
		#define MPT_ARCH_X86_TSC
		#define MPT_ARCH_X86_CX8
		#define MPT_ARCH_X86_CMOV
		#define MPT_ARCH_X86_MMX
		#define MPT_ARCH_X86_3DNOWPREFETCH
		#define MPT_ARCH_X86_SSE
	#elif MPT_MSVC_AT_LEAST(2008, 0)
		#define MPT_ARCH_X86_I386
		#define MPT_ARCH_X86_FPU
		#define MPT_ARCH_X86_FSIN
		#define MPT_ARCH_X86_I486
		#define MPT_ARCH_X86_CPUID
		#define MPT_ARCH_X86_TSC
		#define MPT_ARCH_X86_CX8
	#elif MPT_MSVC_AT_LEAST(2005, 0)
		#define MPT_ARCH_X86_I386
		#define MPT_ARCH_X86_FPU
		#define MPT_ARCH_X86_FSIN
		#define MPT_ARCH_X86_I486
	#elif MPT_MSVC_AT_LEAST(1998, 0)
		#define MPT_ARCH_X86_I386
		#define MPT_ARCH_X86_FPU
		#define MPT_ARCH_X86_FSIN
	#else
		#define MPT_ARCH_X86_I386
	#endif
#endif
#if defined(__AVX__)
	#define MPT_ARCH_X86_AVX
#endif
#if defined(__AVX2__)
	#define MPT_ARCH_X86_AVX2
	#define MPT_ARCH_X86_FMA
	#define MPT_ARCH_X86_BMI1
#endif

#elif MPT_COMPILER_GCC || MPT_COMPILER_CLANG

#define MPT_ARCH_X86_I386
#if !defined(_SOFT_FLOAT)
	#define MPT_ARCH_X86_FPU
	// GCC does not provide a macro for FSIN. Deduce it from 486 later.
#endif
#if defined(__i486__)
	// GCC does not consistently provide i486, deduce it later from cpuid.
	#define MPT_ARCH_X86_I486
#endif
#ifdef __GCC_HAVE_SYNC_COMPARE_AND_SWAP_8
	// GCC does not provide TSC or CPUID.
	// Imply it by CX8.
	#define MPT_ARCH_X86_CX8
	#define MPT_ARCH_X86_TSC
	#define MPT_ARCH_X86_CPUID
#endif
#if defined(__i686__) || defined(__athlon__)
	// GCC is broken here and does not set __i686__ for various non-Intel and even modern Intel CPUs
	// Imply __i686__ by __SSE__ as a work-around.
	#define MPT_ARCH_X86_CMOV
#endif
#if defined(MPT_ARCH_X86_CPUID)
	#ifndef MPT_ARCH_X86_I486
	#define MPT_ARCH_X86_I486
	#endif
#endif
#if defined(MPT_ARCH_X86_I486) && defined(MPT_ARCH_X86_FPU)
	#define MPT_ARCH_X86_FSIN
#endif
#ifdef __MMX__
	#define MPT_ARCH_X86_MMX
#endif
#ifdef __3dNOW__
	#define MPT_ARCH_X86_MMXEXT
	#define MPT_ARCH_X86_3DNOW
#endif
#ifdef __3dNOW_A__
	#define MPT_ARCH_X86_3DNOWEXT
#endif
#ifdef __SSE__
	#define MPT_ARCH_X86_3DNOWPREFETCH
	#define MPT_ARCH_X86_SSE
	#ifndef MPT_ARCH_X86_CMOV
	#define MPT_ARCH_X86_CMOV
	#endif
#endif
#ifdef __SSE2__
	#define MPT_ARCH_X86_SSE2
#endif
#ifdef __SSE3__
	#define MPT_ARCH_X86_SSE3
#endif
#ifdef __SSSE3__
	#define MPT_ARCH_X86_SSSE3
#endif
#ifdef __SSE4_1__
	#define MPT_ARCH_X86_SSE4_1
#endif
#ifdef __SSE4_2__
	#define MPT_ARCH_X86_SSE4_2
#endif
#ifdef __AVX__
	#define MPT_ARCH_X86_AVX
#endif
#ifdef __AVX2__
	#define MPT_ARCH_X86_AVX2
#endif
#ifdef __GCC_HAVE_SYNC_COMPARE_AND_SWAP_16
	#define MPT_ARCH_X86_CX16
#endif
#ifdef __LAHF_SAHF__
	#define MPT_ARCH_X86_LAHF
#endif
#ifdef __POPCNT__
	#define MPT_ARCH_X86_POPCNT
#endif
#ifdef __BMI__
	#define MPT_ARCH_X86_BMI1
#endif
#ifdef __BMI2__
	#define MPT_ARCH_X86_BMI2
#endif
#ifdef __F16C__
	#define MPT_ARCH_X86_F16C
#endif
#ifdef __FMA__
	#define MPT_ARCH_X86_FMA
#endif
#ifdef __LZCNT__
	#define MPT_ARCH_X86_LZCNT
#endif
#ifdef __MOVBE__
	#define MPT_ARCH_X86_MOVBE
#endif

#endif // MPT_COMPILER

#endif // MPT_ARCH

// clang-format on



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace arch {



namespace x86 {



using feature_flags = mpt::arch::basic_feature_flags<uint32>;


// clang-format off

namespace feature {
inline constexpr feature_flags none           = feature_flags{};
inline constexpr feature_flags i386           = feature_flags{ 0x0000'0001 };
inline constexpr feature_flags fpu            = feature_flags{ 0x0000'0002 };
inline constexpr feature_flags fsin           = feature_flags{ 0x0000'0004 };
inline constexpr feature_flags i486           = feature_flags{ 0x0000'0008 };  // XADD, BSWAP, CMPXCHG
inline constexpr feature_flags cpuid          = feature_flags{ 0x0000'0010 };
inline constexpr feature_flags tsc            = feature_flags{ 0x0000'0020 };
inline constexpr feature_flags cx8            = feature_flags{ 0x0000'0040 };
inline constexpr feature_flags cmov           = feature_flags{ 0x0000'0080 };
inline constexpr feature_flags mmx            = feature_flags{ 0x0000'0100 };
inline constexpr feature_flags mmxext         = feature_flags{ 0x0000'0200 };
inline constexpr feature_flags x3dnow         = feature_flags{ 0x0000'0400 };
inline constexpr feature_flags x3dnowext      = feature_flags{ 0x0000'0800 };
inline constexpr feature_flags x3dnowprefetch = feature_flags{ 0x0000'1000 };
inline constexpr feature_flags sse            = feature_flags{ 0x0000'2000 };
inline constexpr feature_flags sse2           = feature_flags{ 0x0000'4000 };
inline constexpr feature_flags sse3           = feature_flags{ 0x0000'8000 };
inline constexpr feature_flags ssse3          = feature_flags{ 0x0001'0000 };
inline constexpr feature_flags sse4_1         = feature_flags{ 0x0002'0000 };
inline constexpr feature_flags sse4_2         = feature_flags{ 0x0004'0000 };
inline constexpr feature_flags avx            = feature_flags{ 0x0008'0000 };
inline constexpr feature_flags avx2           = feature_flags{ 0x0010'0000 };
inline constexpr feature_flags cx16           = feature_flags{ 0x0020'0000 };
inline constexpr feature_flags lahf           = feature_flags{ 0x0040'0000 };
inline constexpr feature_flags popcnt         = feature_flags{ 0x0080'0000 };
inline constexpr feature_flags bmi1           = feature_flags{ 0x0100'0000 };
inline constexpr feature_flags bmi2           = feature_flags{ 0x0200'0000 };
inline constexpr feature_flags f16c           = feature_flags{ 0x0400'0000 };
inline constexpr feature_flags fma            = feature_flags{ 0x0800'0000 };
inline constexpr feature_flags lzcnt          = feature_flags{ 0x1000'0000 };
inline constexpr feature_flags movbe          = feature_flags{ 0x2000'0000 };
} // namespace feature

namespace featureset {
inline constexpr feature_flags i386      = feature::i386;
inline constexpr feature_flags i486SX    = featureset::i386      | feature::i486;
inline constexpr feature_flags i486DX    = featureset::i486SX    | feature::fpu | feature::fsin;
inline constexpr feature_flags i586      = featureset::i486DX    | feature::cpuid | feature::tsc | feature::cx8;
inline constexpr feature_flags i586_mmx  = featureset::i586      | feature::mmx;
inline constexpr feature_flags i686      = featureset::i586      | feature::cmov;
inline constexpr feature_flags i686_mmx  = featureset::i686      | feature::mmx;
inline constexpr feature_flags i686_sse  = featureset::i686_mmx  | feature::sse | feature::x3dnowprefetch;
inline constexpr feature_flags i686_sse2 = featureset::i686_sse  | feature::sse2;
inline constexpr feature_flags i786      = featureset::i686_sse2;
inline constexpr feature_flags amd64     = featureset::i686_sse2;
inline constexpr feature_flags amd64_v2  = featureset::amd64     | feature::cx16 | feature::lahf | feature::popcnt | feature::sse3 | feature::ssse3 | feature::sse4_1 | feature::sse4_2;
inline constexpr feature_flags amd64_v3  = featureset::amd64_v2  | feature::avx | feature::avx2 | feature::bmi1 | feature::bmi2 | feature::f16c | feature::fma | feature::lzcnt | feature::movbe;
inline constexpr feature_flags msvc_x86_1998   = featureset::i386 | feature::fpu | feature::fsin;
inline constexpr feature_flags msvc_x86_2005   = featureset::i486DX;
inline constexpr feature_flags msvc_x86_2008   = featureset::i586;
inline constexpr feature_flags msvc_x86_sse    = featureset::i686_sse;
inline constexpr feature_flags msvc_x86_sse2   = featureset::i686_sse2;
inline constexpr feature_flags msvc_x86_avx    = featureset::i686_sse2 | feature::avx;
inline constexpr feature_flags msvc_x86_avx2   = featureset::i686_sse2 | feature::avx | feature::avx2 | feature::fma | feature::bmi1;
inline constexpr feature_flags msvc_amd64      = featureset::amd64;
inline constexpr feature_flags msvc_amd64_avx  = featureset::amd64 | feature::avx;
inline constexpr feature_flags msvc_amd64_avx2 = featureset::amd64 | feature::avx | feature::avx2 | feature::fma | feature::bmi1;
} // namespace featureset

// clang-format on


enum class vendor : uint8 {
	unknown = 0,
	AMD,
	Centaur,
	Cyrix,
	Intel,
	Transmeta,
	NSC,
	NexGen,
	Rise,
	SiS,
	UMC,
	VIA,
	DMnP,
	Zhaoxin,
	Hygon,
	Elbrus,
	MiSTer,
	bhyve,
	KVM,
	QEMU,
	HyperV,
	Parallels,
	VMWare,
	Xen,
	ACRN,
	QNX,
}; // enum class vendor



// clang-format off
MPT_CONSTEVAL [[nodiscard]] feature_flags assumed_features() noexcept {
	feature_flags flags{};
#if MPT_ARCH_X86 || MPT_ARCH_AMD64
	#ifdef MPT_ARCH_X86_I386
		flags |= feature::i386;
	#endif
	#ifdef MPT_ARCH_X86_FPU
		flags |= feature::fpu;
	#endif
	#ifdef MPT_ARCH_X86_FSIN
		flags |= feature::fsin;
	#endif
	#ifdef MPT_ARCH_X86_CPUID
		flags |= feature::cpuid;
	#endif
	#ifdef MPT_ARCH_X86_TSC
		flags |= feature::tsc;
	#endif
	#ifdef MPT_ARCH_X86_CX8
		flags |= feature::cx8;
	#endif
	#ifdef MPT_ARCH_X86_CMOV
		flags |= feature::cmov;
	#endif
	#ifdef MPT_ARCH_X86_MMX
		flags |= feature::mmx;
	#endif
	#ifdef MPT_ARCH_X86_MMXEXT
		flags |= feature::mmxext;
	#endif
	#ifdef MPT_ARCH_X86_3DNOW
		flags |= feature::x3dnow;
	#endif
	#ifdef MPT_ARCH_X86_3DNOWEXT
		flags |= feature::x3dnowext;
	#endif
	#ifdef MPT_ARCH_X86_3DNOWPREFETCH
		flags |= feature::x3dnowprefetch;
	#endif
	#ifdef MPT_ARCH_X86_SSE
		flags |= feature::sse;
	#endif
	#ifdef MPT_ARCH_X86_SSE2
		flags |= feature::sse2;
	#endif
	#ifdef MPT_ARCH_X86_SSE3
		flags |= feature::sse3;
	#endif
	#ifdef MPT_ARCH_X86_SSSE3
		flags |= feature::ssse3;
	#endif
	#ifdef MPT_ARCH_X86_SSE4_1
		flags |= feature::sse4_1;
	#endif
	#ifdef MPT_ARCH_X86_SSE4_2
		flags |= feature::sse4_2;
	#endif
	#ifdef MPT_ARCH_X86_AVX
		flags |= feature::avx;
	#endif
	#ifdef MPT_ARCH_X86_AVX2
		flags |= feature::avx2;
	#endif
	#ifdef MPT_ARCH_X86_CX16
		flags |= feature::cx16;
	#endif
	#ifdef MPT_ARCH_X86_LAHF
		flags |= feature::lahf;
	#endif
	#ifdef MPT_ARCH_X86_POPCNT
		flags |= feature::popcnt;
	#endif
	#ifdef MPT_ARCH_X86_BMI1
		flags |= feature::bmi1;
	#endif
	#ifdef MPT_ARCH_X86_BMI2
		flags |= feature::bmi2;
	#endif
	#ifdef MPT_ARCH_X86_F16C
		flags |= feature::f16c;
	#endif
	#ifdef MPT_ARCH_X86_FMA
		flags |= feature::fma;
	#endif
	#ifdef MPT_ARCH_X86_LZCNT
		flags |= feature::lzcnt;
	#endif
	#ifdef MPT_ARCH_X86_MOVBE
		flags |= feature::movbe;
	#endif
#endif // MPT_ARCH_X86 || MPT_ARCH_AMD64
	return flags;
}
// clang-format on



template <std::size_t N>
struct fixed_string {
	std::array<char, N> m_data = {};
	constexpr [[nodiscard]] const char & operator[](std::size_t i) const noexcept {
		return m_data[i];
	}
	constexpr [[nodiscard]] char & operator[](std::size_t i) noexcept {
		return m_data[i];
	}
	constexpr [[nodiscard]] std::size_t size() const noexcept {
		return m_data.size();
	}
	constexpr [[nodiscard]] const char * data() const noexcept {
		return m_data.data();
	}
	constexpr [[nodiscard]] char * data() noexcept {
		return m_data.data();
	}
	constexpr [[nodiscard]] const char * begin() const noexcept {
		return m_data.data();
	}
	constexpr [[nodiscard]] char * begin() noexcept {
		return m_data.data();
	}
	constexpr [[nodiscard]] const char * end() const noexcept {
		return m_data.data() + m_data.size();
	}
	constexpr [[nodiscard]] char * end() noexcept {
		return m_data.data() + m_data.size();
	}
	constexpr [[nodiscard]] operator std::string_view() const noexcept {
		return std::string_view(m_data.begin(), std::find(m_data.begin(), m_data.end(), '\0'));
	}
	template <std::size_t M>
	friend MPT_CONSTEXPR20_FUN [[nodiscard]] auto operator+(fixed_string<N> a, fixed_string<M> b) -> fixed_string<N + M> {
		fixed_string<N + M> result;
		std::copy(a.begin(), a.end(), result.data() + 0);
		std::copy(b.begin(), b.end(), result.data() + N);
		return result;
	}
	constexpr [[nodiscard]] explicit operator std::string() const noexcept {
		return std::string(m_data.begin(), std::find(m_data.begin(), m_data.end(), '\0'));
	}
};


struct cpu_info {

private:

	feature_flags Features{};
	uint32 CPUID = 0;
	vendor Vendor = vendor::unknown;
	uint16 Family = 0;
	uint8 Model = 0;
	uint8 Stepping = 0;
	fixed_string<12> VendorID;
	fixed_string<48> BrandID;
	bool Virtualized = false;
	fixed_string<12> HypervisorVendor;
	fixed_string<4> HypervisorInterface;
#if !MPT_ARCH_AMD64
	bool LongMode = false;
#endif // !MPT_ARCH_AMD64

public:

	MPT_CONSTEXPRINLINE [[nodiscard]] bool operator[](feature_flags query_features) const noexcept {
		return ((Features & query_features) == query_features);
	}

	MPT_CONSTEXPRINLINE [[nodiscard]] bool has_features(feature_flags query_features) const noexcept {
		return ((Features & query_features) == query_features);
	}

	MPT_CONSTEXPRINLINE [[nodiscard]] feature_flags get_features() const noexcept {
		return Features;
	}

	MPT_CONSTEXPRINLINE [[nodiscard]] uint32 get_cpuid() const noexcept {
		return CPUID;
	}

	MPT_CONSTEXPRINLINE [[nodiscard]] vendor get_vendor() const noexcept {
		return Vendor;
	}

	MPT_CONSTEXPRINLINE [[nodiscard]] uint16 get_family() const noexcept {
		return Family;
	}

	MPT_CONSTEXPRINLINE [[nodiscard]] uint8 get_model() const noexcept {
		return Model;
	}

	MPT_CONSTEXPRINLINE [[nodiscard]] uint8 get_stepping() const noexcept {
		return Stepping;
	}

	inline [[nodiscard]] std::string get_vendor_string() const {
		return std::string(VendorID);
	}

	inline [[nodiscard]] std::string get_brand_string() const {
		return std::string(BrandID);
	}

	MPT_CONSTEXPRINLINE [[nodiscard]] bool is_virtual() const noexcept {
		return Virtualized;
	}

	MPT_CONSTEXPRINLINE [[nodiscard]] bool can_long_mode() const noexcept {
#if !MPT_ARCH_AMD64
		return LongMode;
#else // MPT_ARCH_AMD64
		return true;
#endif // !MPT_ARCH_AMD64
	}

private:

#if MPT_ARCH_X86 || MPT_ARCH_AMD64

	struct cpuid_result {

		uint32 a = 0;
		uint32 b = 0;
		uint32 c = 0;
		uint32 d = 0;

		MPT_CONSTEXPR20_FUN [[nodiscard]] fixed_string<4> as_text4() const noexcept {
			fixed_string<4> result;
			result[0+0] = (a >> 0) & 0xff;
			result[0+1] = (a >> 8) & 0xff;
			result[0+2] = (a >>16) & 0xff;
			result[0+3] = (a >>24) & 0xff;
			return result;
		}

		MPT_CONSTEXPR20_FUN [[nodiscard]] fixed_string<12> as_text12bcd() const noexcept {
			fixed_string<12> result;
			result[0+0] = (b >> 0) & 0xff;
			result[0+1] = (b >> 8) & 0xff;
			result[0+2] = (b >>16) & 0xff;
			result[0+3] = (b >>24) & 0xff;
			result[4+0] = (c >> 0) & 0xff;
			result[4+1] = (c >> 8) & 0xff;
			result[4+2] = (c >>16) & 0xff;
			result[4+3] = (c >>24) & 0xff;
			result[8+0] = (d >> 0) & 0xff;
			result[8+1] = (d >> 8) & 0xff;
			result[8+2] = (d >>16) & 0xff;
			result[8+3] = (d >>24) & 0xff;
			return result;
		}

		MPT_CONSTEXPR20_FUN [[nodiscard]] fixed_string<12> as_text12bdc() const noexcept {
			fixed_string<12> result;
			result[0+0] = (b >> 0) & 0xff;
			result[0+1] = (b >> 8) & 0xff;
			result[0+2] = (b >>16) & 0xff;
			result[0+3] = (b >>24) & 0xff;
			result[4+0] = (d >> 0) & 0xff;
			result[4+1] = (d >> 8) & 0xff;
			result[4+2] = (d >>16) & 0xff;
			result[4+3] = (d >>24) & 0xff;
			result[8+0] = (c >> 0) & 0xff;
			result[8+1] = (c >> 8) & 0xff;
			result[8+2] = (c >>16) & 0xff;
			result[8+3] = (c >>24) & 0xff;
			return result;
		}

		MPT_CONSTEXPR20_FUN [[nodiscard]] fixed_string<16> as_text16() const noexcept {
			fixed_string<16> result;
			result[0+0] = (a >> 0) & 0xff;
			result[0+1] = (a >> 8) & 0xff;
			result[0+2] = (a >>16) & 0xff;
			result[0+3] = (a >>24) & 0xff;
			result[4+0] = (b >> 0) & 0xff;
			result[4+1] = (b >> 8) & 0xff;
			result[4+2] = (b >>16) & 0xff;
			result[4+3] = (b >>24) & 0xff;
			result[8+0] = (d >> 0) & 0xff;
			result[8+1] = (d >> 8) & 0xff;
			result[8+2] = (d >>16) & 0xff;
			result[8+3] = (d >>24) & 0xff;
			result[12+0] = (c >> 0) & 0xff;
			result[12+1] = (c >> 8) & 0xff;
			result[12+2] = (c >>16) & 0xff;
			result[12+3] = (c >>24) & 0xff;
			return result;
		}

	};

#if MPT_COMPILER_MSVC || MPT_COMPILER_GCC || MPT_COMPILER_CLANG

	static [[nodiscard]] cpuid_result cpuid(uint32 function) noexcept {

#if MPT_COMPILER_MSVC

		cpuid_result result;
		int CPUInfo[4]{};
		__cpuid(CPUInfo, function);
		result.a = CPUInfo[0];
		result.b = CPUInfo[1];
		result.c = CPUInfo[2];
		result.d = CPUInfo[3];
		return result;

#elif MPT_COMPILER_GCC || MPT_COMPILER_CLANG

		cpuid_result result;
		unsigned int regeax{};
		unsigned int regebx{};
		unsigned int regecx{};
		unsigned int regedx{};
		__cpuid(function, regeax, regebx, regecx, regedx);
		result.a = regeax;
		result.b = regebx;
		result.c = regecx;
		result.d = regedx;
		return result;

#elif 0

		cpuid_result result;
		unsigned int a{};
		unsigned int b{};
		unsigned int c{};
		unsigned int d{};
		__asm__ __volatile__ ("cpuid \n\t
			: "=a" (a), "=b" (b), "=c" (c), "=d" (d)
			: "0" (function));
		result.a = a;
		result.b = b;
		result.c = c;
		result.d = d;
		return result;

#else // MPT_COMPILER

		return cpuid_result result{};

#endif // MPT_COMPILER

	}

	static [[nodiscard]] cpuid_result cpuidex(uint32 function_a, uint32 function_c) noexcept {

#if MPT_COMPILER_MSVC

		cpuid_result result;
		int CPUInfo[4]{};
		__cpuidex(CPUInfo, function_a, function_c);
		result.a = CPUInfo[0];
		result.b = CPUInfo[1];
		result.c = CPUInfo[2];
		result.d = CPUInfo[3];
		return result;

#elif MPT_COMPILER_GCC || MPT_COMPILER_CLANG

		cpuid_result result;
		unsigned int regeax{};
		unsigned int regebx{};
		unsigned int regecx{};
		unsigned int regedx{};
		__cpuid_count(function_a, function_c, regeax, regebx, regecx, regedx);
		result.a = regeax;
		result.b = regebx;
		result.c = regecx;
		result.d = regedx;
		return result;

#elif 0

		cpuid_result result;
		unsigned int a{};
		unsigned int b{};
		unsigned int c{};
		unsigned int d{};
		__asm__ __volatile__ ("cpuid \n\t
			: "=a" (a), "=b" (b), "=c" (c), "=d" (d)
			: "0" (function_a), "2" (function_c));
		result.a = a;
		result.b = b;
		result.c = c;
		result.d = d;
		return result;

#else // MPT_COMPILER

		return cpuid_result result{};

#endif // MPT_COMPILER

	}

#endif // MPT_COMPILER_MSVC || MPT_COMPILER_GCC || MPT_COMPILER_CLANG

private:

#if MPT_ARCH_X86 || !MPT_ARCH_X86_AMD64_FAST_DETECT

#if MPT_OS_DJGPP

	static [[nodiscard]] uint8 detect_cpu_level() noexcept {
	
#if 0
		uint8 result = 0;
		__dpmi_version_ret dpmi_version{};
		if (__dpmi_get_version(&dpmi_version) == 0) {
			result = dpmi_version.cpu;
		}
		return result;
#else
		uint8 result = 0;
		__dpmi_regs regs{};
		regs.x.ax = 0x0400;
		if (__dpmi_int(0x31, &regs) == 0) {
			unsigned int cf = (regs.x.flags >> 0) & 1u;
			if (cf == 0) {
				result = regs.h.cl;
			}
		}
		return result;
#endif

	}

#endif // MPT_OS_DJGPP

	static [[nodiscard]] uint8 detect_fpu_level() noexcept {

#if MPT_OS_DJGPP

#if 1
		uint8 result = 0;
		int coprocessor_status = __dpmi_get_coprocessor_status();
		if (coprocessor_status == -1) {
			// error = __dpmi_error
			return 0;
		}
		result = (static_cast<unsigned int>(coprocessor_status) & 0x00f0u) >> 4;
		return result;
#else
		uint8 result = 0;
		__dpmi_regs regs{};
		regs.x.ax = 0x0e00;
		if (__dpmi_int(0x31, &regs) == 0) {
			unsigned int cf = (regs.x.flags >> 0) & 1u;
			if (cf == 0) {
				result = (regs.x.ax & 0x00f0u) >> 4;
			}
		}
		return result;
#endif

#elif MPT_OS_WINDOWS

		uint8 result = 0;
#if defined(_WIN32_WINNT)
#if (_WIN32_WINNT >= _WIN32_WINNT_NT4)
		if (mpt::osinfo::windows::Version::Current().IsAtLeast(mpt::osinfo::windows::Version::Win2000)) {
			if (IsProcessorFeaturePresent(PF_FLOATING_POINT_EMULATED) == 0) {
				result = 3;
			}
		} else {
			// logic is inverted on NT4
			if (IsProcessorFeaturePresent(PF_FLOATING_POINT_EMULATED) != 0) {
				result = 3;
			}
		}
#else
		if ((assumed_features() & feature::fpu) && (assumed_features() & feature::fsin)) {
			result = 3;
		} else if (assumed_features() & feature::fpu) {
			result = 2;
		} else {
			result = 0;
		}
#endif
#else
		if ((assumed_features() & feature::fpu) && (assumed_features() & feature::fsin)) {
			result = 3;
		} else if (assumed_features() & feature::fpu) {
			result = 2;
		} else {
			result = 0;
		}
#endif
		return result;

#elif MPT_COMPILER_MSVC && MPT_MODE_KERNEL

		const std::size_t cr0 = __readcr0();
		if (!(cr0 & (1 << 2))) {  // EM
			result = 2;
			if (cr0 & (1 << 4)) {  // ET
				result = 3;
			}
		}
		return result;

#else

		if ((assumed_features() & feature::fpu) && (assumed_features() & feature::fsin)) {
			result = 3;
		} else if (assumed_features() & feature::fpu) {
			result = 2;
		} else {
			result = 0;
		}
		return result;

#endif

	}

#if MPT_COMPILER_MSVC || MPT_COMPILER_GCC || MPT_COMPILER_CLANG

	static [[nodiscard]] bool can_toggle_eflags(std::size_t mask) noexcept {
		std::size_t eflags_old = __readeflags();
		std::size_t eflags_flipped = eflags_old ^ mask;
		__writeeflags(eflags_flipped);
		std::size_t eflags_testchanged = __readeflags();
		__writeeflags(eflags_old);
		return ((eflags_testchanged ^ eflags_old) & mask) == mask;
	}

#endif

	static [[nodiscard]] bool can_toggle_eflags_ac() noexcept {
#if MPT_COMPILER_MSVC || MPT_COMPILER_GCC || MPT_COMPILER_CLANG
		return can_toggle_eflags(0x0004'0000);
#else // MPT_COMPILER
		return (assumed_features() & feature::i486) != 0;
#endif // MPT_COMPILER
	}

	static [[nodiscard]] bool can_toggle_eflags_id() noexcept {
#if MPT_COMPILER_MSVC || MPT_COMPILER_GCC || MPT_COMPILER_CLANG
		return can_toggle_eflags(0x0020'0000);
#else // MPT_COMPILER
		return (assumed_features() & feature::tsc) != 0;
#endif // MPT_COMPILER
	}

	static [[nodiscard]] bool detect_nexgen() noexcept {

#if	MPT_ARCH_X86 && MPT_COMPILER_MSVC

		uint8 result = 0;
		_asm {
			mov eax, 0x5555
			xor edx, edx
			mov ecx, 2
			clc
			div ecx
			jz  found
			jmp done
			found:
				mov result, 1
				jmp done
			done:
		}
		return (result != 0);

#elif MPT_ARCH_X86 && (MPT_COMPILER_GCC || MPT_COMPILER_CLANG)

		unsigned int result = 0;
		__asm__ __volatile(
			"movl $0x5555, %eax\n"
			"xorl %edx, %edx\n"
			"movl $2, %ecx\n"
			"clc\n"
			"divl %ecx\n"
			"jz found\n"
			"movl $0, %eax\n"
			"jmp done\n"
			"found:\n"
			"movl $0, %eax\n"
			"jmp done\n"
			"done:\n"
			: "=a" (result)
			:
			: "%eax", "%ebx", "%ecx"
		);
		return (result != 0);

#else

		// assume false
		return false;

#endif

	}

	static [[nodiscard]] bool detect_cyrix() noexcept {

#if	MPT_ARCH_X86 && MPT_COMPILER_MSVC

		uint8 result = 0;
		_asm {
			xor  ax, ax
			sahf
			mov  ax, 5
			mov  bx, 2
			div  bl
			lahf
			cmp  ah, 2
			jne  not_cyrix
				mov  result, 1
			not_cyrix:
		}
		return (result != 0);

#elif MPT_ARCH_X86 && (MPT_COMPILER_GCC || MPT_COMPILER_CLANG)

		unsigned int result = 0;
		__asm__ __volatile(
			"xor %ax, %ax\n"
			"shaf\n"
			"movw $5, %ax\n"
			"movw $2, %bx\n"
			"divb %bl\n"
			"lahf\n"
			"cmpw $2, %ah\n"
			"jne not_cyrix\n"
			"movl $1, %eax\n"
			"jmp done\n"
			"not_cyrix:\n"
			"movl $0, eax\n
			"jmp done\n"
			"done:\n"
			: "=a" (result)
			:
			: "%eax", "%ebx"
		);
		return (result != 0);

#else

		// assume false
		return false;

#endif

	}

	static [[nodiscard]] uint16 read_cyrix_id() noexcept {

#if MPT_OS_DJGPP

		uint16 result = 0;
		outportb(0x22, 0x00);
		result |= static_cast<uint16>(inportb(0x23)) << 8;
		outportb(0x22, 0x01);
		result |= static_cast<uint16>(inportb(0x23)) << 0;
		outportb(0x22, 0x00);
		inportb(0x23);
		return result;

#elif	MPT_COMPILER_MSVC && MPT_MODE_KERNEL

		uint16 result = 0;
		__outbyte(0x22, 0x00);
		result |= static_cast<uint16>(__inbyte(0x23)) << 8;
		__outbyte(0x22, 0x01);
		result |= static_cast<uint16>(__inbyte(0x23)) << 0;
		__outbyte(0x22, 0x00);
		__inbyte(0x23);
		return result;

#else

		return 0x00'00;

#endif

	}

#endif // MPT_ARCH_X86

#endif // MPT_ARCH_X86 || MPT_ARCH_AMD64

public:

	cpu_info() noexcept {

#if MPT_ARCH_X86 || MPT_ARCH_AMD64

#if MPT_COMPILER_MSVC || MPT_COMPILER_GCC || MPT_COMPILER_CLANG

#if MPT_ARCH_X86 || !MPT_ARCH_X86_AMD64_FAST_DETECT

		Features |= featureset::i386;

		if (can_toggle_eflags_ac()) {
			Features |= feature::i486;
		}
		if (can_toggle_eflags_id()) {
			Features |= feature::cpuid;
		}
		if (!Features.supports(feature::cpuid)) {
			// without cpuid
			if (!Features.supports(feature::i486)) {
				// 386
				const uint8 fpu_level = detect_fpu_level();
				if (fpu_level >= 2) {
					Features |= feature::fpu;
				}
				if (fpu_level >= 3) {
					Features |= feature::fsin;
				}
				if (detect_nexgen()) {
					Vendor = vendor::NexGen;
				} else {
					Vendor = vendor::unknown;
				}
			} else {
				// 486+
				const uint8 fpu_level = detect_fpu_level();
				if (fpu_level >= 2) {
					Features |= feature::fpu;
				}
				if (fpu_level >= 3) {
					Features |= feature::fsin;
				}
				if (detect_cyrix()) {
					Vendor = vendor::Cyrix;
					uint16 id = read_cyrix_id();
					if ((0x00'00 <= id) && (id <= 0x07'00)) {
						// Cx486SLC / Cx486DLC
						Family = 3;
						Model = static_cast<uint8>((id & 0xff'00) >> 8);
						Stepping = static_cast<uint8>(id & 0x00'ff);
					} else if ((0x10'00 <= id) && (id <= 0x13'00)) {
						// Cx486S
						Family = 4;
						Model = static_cast<uint8>((id & 0xff'00) >> 8);
						Stepping = static_cast<uint8>(id & 0x00'ff);
					} else if ((0x1a'00 <= id) && (id <= 0x1f'00)) {
						// Cx486DX
						Family = 4;
						Model = static_cast<uint8>((id & 0xff'00) >> 8);
						Stepping = static_cast<uint8>(id & 0x00'ff);
					} else if ((0x28'00 <= id) && (id <= 0x2e'00)) {
						// Cx5x86
						Family = 4;
						Model = static_cast<uint8>((id & 0xff'00) >> 8);
						Stepping = static_cast<uint8>(id & 0x00'ff);
					} else if ((0x40'00 <= id) && (id <= 0x4f'00)) {
						// MediaGx / MediaGXm
						Family = 4;
						Model = 4;
						Stepping = static_cast<uint8>(id & 0x00'ff);
					} else if ((0x30'00 <= id) && (id <= 0x34'00)) {
						// Cx6x86 / Cx6x86L
						Family = 5;
						Model = 2;
						Stepping = static_cast<uint8>(id & 0x00'ff);
						if ((id & 0x00'ff) > 0x21) {
							// Cx6x86L
							Features |= feature::cx8;
						}
					} else if ((0x50'00 <= id) && (id <= 0x5f'00)) {
						// Cx6x86MX
						Family = 6;
						Model = 0;
						Stepping = static_cast<uint8>(id & 0x00'ff);
						Features |= feature::cx8;
						Features |= feature::tsc;
						Features |= feature::mmx;
						Features |= feature::cmov;
					}
				} else {
					Vendor = vendor::unknown;
				}
			}
		}

#elif MPT_ARCH_AMD64

		Features |= featureset::amd64;

#endif // MPT_ARCH
		
		if (Features.supports(feature::cpuid)) {
			// with cpuid
			cpuid_result VendorString = cpuid(0x0000'0000u);
			VendorID = VendorString.as_text12bdc();
			if (VendorID == std::string_view("            ")) {
				Vendor = vendor::unknown;
			} else if (VendorID == std::string_view("AMDisbetter!")) {
				Vendor = vendor::AMD;
			} else if (VendorID == std::string_view("AuthenticAMD")) {
				Vendor = vendor::AMD;
			} else if (VendorID == std::string_view("CentaurHauls")) {
				Vendor = vendor::Centaur;
			} else if (VendorID == std::string_view("CyrixInstead")) {
				Vendor = vendor::Cyrix;
			} else if (VendorID == std::string_view("GenuineIntel")) {
				Vendor = vendor::Intel;
			} else if (VendorID == std::string_view("TransmetaCPU")) {
				Vendor = vendor::Transmeta;
			} else if (VendorID == std::string_view("GenuineTMx86")) {
				Vendor = vendor::Transmeta;
			} else if (VendorID == std::string_view("Geode by NSC")) {
				Vendor = vendor::NSC;
			} else if (VendorID == std::string_view("NexGenDriven")) {
				Vendor = vendor::NexGen;
			} else if (VendorID == std::string_view("RiseRiseRise")) {
				Vendor = vendor::Rise;
			} else if (VendorID == std::string_view("SiS SiS SiS ")) {
				Vendor = vendor::SiS;
			} else if (VendorID == std::string_view("UMC UMC UMC ")) {
				Vendor = vendor::UMC;
			} else if (VendorID == std::string_view("VIA VIA VIA ")) {
				Vendor = vendor::VIA;
			} else if (VendorID == std::string_view("Vortex86 SoC")) {
				Vendor = vendor::DMnP;
			} else if (VendorID == std::string_view("  Shanghai  ")) {
				Vendor = vendor::Zhaoxin;
			} else if (VendorID == std::string_view("HygonGenuine")) {
				Vendor = vendor::Hygon;
			} else if (VendorID == std::string_view("E2K MACHINE")) {
				Vendor = vendor::Elbrus;
			} else if (VendorID == std::string_view("MiSTer AO486")) {
				Vendor = vendor::MiSTer;
			} else if (VendorID == std::string_view("bhyve bhyve ")) {
				Vendor = vendor::bhyve;
			} else if (VendorID == std::string_view(" KVMKVMKVM  ")) {
				Vendor = vendor::KVM;
			} else if (VendorID == std::string_view("TCGTCGTCGTCG")) {
				Vendor = vendor::QEMU;
			} else if (VendorID == std::string_view("Microsoft Hv")) {
				Vendor = vendor::HyperV;
			} else if (VendorID == std::string_view(" lrpepyh  vr")) {
				Vendor = vendor::Parallels;
			} else if (VendorID == std::string_view("VMwareVMware")) {
				Vendor = vendor::VMWare;
			} else if (VendorID == std::string_view("XenVMMXenVMM")) {
				Vendor = vendor::Xen;
			} else if (VendorID == std::string_view("ACRNACRNACRN")) {
				Vendor = vendor::ACRN;
			} else if (VendorID == std::string_view(" QNXQVMBSQG ")) {
				Vendor = vendor::QNX;
			}
			// Cyrix 6x86 and 6x86MX do not specify the value returned in eax.
			// They both support 0x00000001u however.
			if ((VendorString.a >= 0x0000'0001u) || (Vendor == vendor::Cyrix)) {
				cpuid_result StandardFeatureFlags = cpuid(0x0000'0001u);
				CPUID = StandardFeatureFlags.a;
				uint32 BaseStepping = (StandardFeatureFlags.a >>  0) & 0x0f;
				uint32 BaseModel    = (StandardFeatureFlags.a >>  4) & 0x0f;
				uint32 BaseFamily   = (StandardFeatureFlags.a >>  8) & 0x0f;
				uint32 ExtModel     = (StandardFeatureFlags.a >> 16) & 0x0f;
				uint32 ExtFamily    = (StandardFeatureFlags.a >> 20) & 0xff;
				if (BaseFamily == 0xf) {
					Family = static_cast<uint16>(ExtFamily + BaseFamily);
				} else {
					Family = static_cast<uint16>(BaseFamily);
				}
				if ((Vendor == vendor::AMD) && (BaseFamily == 0xf)) {
					Model = static_cast<uint8>((ExtModel << 4) | (BaseModel << 0));
				} else if ((Vendor == vendor::Centaur) && (BaseFamily >= 0x6)) {
					// Newer Zhaoxin CPUs use extended family also with BaseFamily=7.
					Model = static_cast<uint8>((ExtModel << 4) | (BaseModel << 0));
				} else if ((BaseFamily == 0x6) || (BaseFamily == 0xf)) {
					Model = static_cast<uint8>((ExtModel << 4) | (BaseModel << 0));
				} else {
					Model = static_cast<uint8>(BaseModel);
				}
				Stepping = static_cast<uint8>(BaseStepping);
				Features |= (StandardFeatureFlags.d & (1 <<  0)) ? (feature::fpu | feature::fsin) : feature::none;
				Features |= (StandardFeatureFlags.d & (1 <<  4)) ? (feature::tsc) : feature::none;
				Features |= (StandardFeatureFlags.d & (1 <<  8)) ? (feature::cx8) : feature::none;
				Features |= (StandardFeatureFlags.d & (1 << 15)) ? (feature::cmov) : feature::none;
				Features |= (StandardFeatureFlags.d & (1 << 23)) ? (feature::mmx) : feature::none;
				Features |= (StandardFeatureFlags.d & (1 << 25)) ? (feature::sse | feature::x3dnowprefetch) : feature::none;
				Features |= (StandardFeatureFlags.d & (1 << 26)) ? (feature::sse2) : feature::none;
				Features |= (StandardFeatureFlags.c & (1 <<  0)) ? (feature::sse3) : feature::none;
				Features |= (StandardFeatureFlags.c & (1 <<  9)) ? (feature::ssse3) : feature::none;
				Features |= (StandardFeatureFlags.c & (1 << 12)) ? (feature::fma) : feature::none;
				Features |= (StandardFeatureFlags.c & (1 << 13)) ? (feature::cx16) : feature::none;
				Features |= (StandardFeatureFlags.c & (1 << 19)) ? (feature::sse4_1) : feature::none;
				Features |= (StandardFeatureFlags.c & (1 << 20)) ? (feature::sse4_2) : feature::none;
				Features |= (StandardFeatureFlags.c & (1 << 22)) ? (feature::movbe) : feature::none;
				Features |= (StandardFeatureFlags.c & (1 << 23)) ? (feature::popcnt) : feature::none;
				Features |= (StandardFeatureFlags.c & (1 << 28)) ? (feature::avx) : feature::none;
				Features |= (StandardFeatureFlags.c & (1 << 29)) ? (feature::f16c) : feature::none;
				if (StandardFeatureFlags.c & (1 << 31)) {
					Virtualized = true;
				}
			}
			if (VendorString.a >= 0x0000'0007u) {
				cpuid_result ExtendedFeatures = cpuidex(0x0000'0007u, 0x0000'0000u);
				Features |= (ExtendedFeatures.b & (1 <<  3)) ? (feature::bmi1) : feature::none;
				Features |= (ExtendedFeatures.b & (1 <<  5)) ? (feature::avx2) : feature::none;
				Features |= (ExtendedFeatures.b & (1 <<  8)) ? (feature::bmi2) : feature::none;
			}
			// 3DNow! manual recommends to just execute 0x8000'0000u.
			// It is totally unknown how earlier CPUs from other vendors
			// would behave.
			// Thus we only execute 0x80000000u on other vendors CPUs for the earliest
			// that we found it documented for and that actually supports 3DNow!.
			bool ecpuid = false;
			bool x3dnowknown = false;
			if (Vendor == vendor::Intel) {
				ecpuid = true;
			} else if (Vendor == vendor::AMD) {
				if ((Family > 5) || ((Family == 5) && (Model >= 8))) {
					// >= K6-2 (K6 = Family 5, K6-2 = Model 8)
					// Not sure if earlier AMD CPUs support 0x80000000u.
					// AMD 5k86 and AMD K5 manuals do not mention it.
					ecpuid = true;
					x3dnowknown = true;
				}
			} else if (Vendor == vendor::Centaur) {
				// Centaur (IDT WinChip or VIA C3)
				if (Family == 5) {
					// IDT
					if (Model >= 8) {
						// >= WinChip 2
						ecpuid = true;
						x3dnowknown = true;
					}
				} else if (Family >= 6) {
					// VIA
					if ((Family >= 7) || ((Family == 6) && (Model >= 7))) {
						// >= C3 Samuel 2
						ecpuid = true;
						x3dnowknown = true;
					}
				}
			} else if (Vendor == vendor::Cyrix) {
				// Cyrix
				// 6x86    : 5.2.x
				// 6x86L   : 5.2.x
				// MediaGX : 4.4.x
				// 6x86MX  : 6.0.x
				// MII     : 6.0.x
				// MediaGXm: 5.4.x
				// well, doh ...
				if ((Family == 5) && (Model >= 4)) {
					// Cyrix MediaGXm
					ecpuid = true;
					x3dnowknown = true;
				}
			} else if (Vendor == vendor::NSC) {
				// National Semiconductor
				if ((Family > 5) || ((Family == 5) && (Model >= 5))) {
					// >= Geode GX2
					ecpuid = true;
					x3dnowknown = true;
				}
			} else {
				// Intel specification allows 0x8000'0000u on earlier CPUs,
				// thus we execute it on unknown vendors.
				ecpuid = true;
			}
			if (ecpuid) {
				cpuid_result ExtendedVendorString = cpuid(0x8000'0000u);
				if ((ExtendedVendorString.a & 0xffff'0000u) == 0x8000'0000u) {
					if (ExtendedVendorString.a >= 0x8000'0001u) {
						cpuid_result ExtendedFeatureFlags = cpuid(0x8000'0001u);
#if !MPT_ARCH_AMD64
						if (ExtendedFeatureFlags.d & (1 << 29)) {
							LongMode = true;
						}
#endif // !MPT_ARCH_AMD64
						Features |= (ExtendedFeatureFlags.c & (1 <<  0)) ? (feature::lahf) : feature::none;
						Features |= (ExtendedFeatureFlags.c & (1 <<  5)) ? (feature::lzcnt) : feature::none;
						if (x3dnowknown) {
							Features |= (ExtendedFeatureFlags.d & (1 << 31)) ? (feature::x3dnow) : feature::none;
						}
						if (Vendor == vendor::AMD) {
							Features |= (ExtendedFeatureFlags.d & (1 << 22)) ? (feature::mmxext) : feature::none;
							Features |= (ExtendedFeatureFlags.d & (1 << 30)) ? (feature::x3dnowext) : feature::none;
							Features |= (ExtendedFeatureFlags.c & (1 <<  5)) ? (feature::popcnt) : feature::none;
							Features |= (ExtendedFeatureFlags.c & (1 <<  8)) ? (feature::x3dnowprefetch) : feature::none;
						}
					}
					if (ExtendedVendorString.a >= 0x8000'0004u) {
						BrandID = cpuid(0x8000'0002u).as_text16() + cpuid(0x8000'0003u).as_text16() + cpuid(0x8000'0004u).as_text16();
					}
				}
			}
			if (Virtualized) {
				cpuid_result HypervisorVendorID = cpuid(0x4000'0000u);
				if (HypervisorVendorID.a >= 0x4000'0000u) {
					HypervisorVendor = HypervisorVendorID.as_text12bcd();
					if (HypervisorVendor == std::string_view("Microsoft Hv")) {
						if (HypervisorVendorID.a >= 0x4000'0001u) {
							cpuid_result HypervisorInterfaceID = cpuid(0x4000'0001u);
							HypervisorInterface = HypervisorInterfaceID.as_text4();
						}
					} else if (HypervisorVendor == std::string_view("KVMKVMKVM")) {
						// nothing
					}
				}
			}
		}

#elif MPT_OS_WINDOWS

#if MPT_ARCH_X86 || !MPT_ARCH_X86_AMD64_FAST_DETECT

		SYSTEM_INFO si = {};
		GetSystemInfo(&si);
		switch (si.wProcessorArchitecture) {
			case PROCESSOR_ARCHITECTURE_INTEL:
			case PROCESSOR_ARCHITECTURE_AMD64:
			case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
			case PROCESSOR_ARCHITECTURE_IA32_ON_ARM64:
				switch (si.dwProcessorType) {
					case PROCESSOR_INTEL_386:
						Family = si.wProcessorLevel;
						if (((si.wProcessorRevision & 0xff00) >> 8) == 0xff) {
							Model = ((si.wProcessorRevision & 0x00f0) >> 4) - 0xa;
							Stepping = ((si.wProcessorRevision & 0x000f) >> 0);
						} else {
							Model = ((si.wProcessorRevision & 0xff00) >> 8) + static_cast<unsigned char>('A');
							Stepping = ((si.wProcessorRevision & 0x00ff) >> 0);
						}
						Model = (si.wProcessorRevision & 0xff00) >> 8;
						Stepping = (si.wProcessorRevision & 0x00ff) >> 0;
						Features |= featureset::i386;
						break;
					case PROCESSOR_INTEL_486:
						Family = si.wProcessorLevel;
						if (((si.wProcessorRevision & 0xff00) >> 8) == 0xff) {
							Model = ((si.wProcessorRevision & 0x00f0) >> 4) - 0xa;
							Stepping = ((si.wProcessorRevision & 0x000f) >> 0);
						} else {
							Model = ((si.wProcessorRevision & 0xff00) >> 8) + static_cast<unsigned char>('A');
							Stepping = ((si.wProcessorRevision & 0x00ff) >> 0);
						}
						Model = (si.wProcessorRevision & 0xff00) >> 8;
						Stepping = (si.wProcessorRevision & 0x00ff) >> 0;
						Features |= featureset::i486SX;
						break;
					case PROCESSOR_INTEL_PENTIUM:
						Family = si.wProcessorLevel;
						Model = (si.wProcessorRevision & 0xff00) >> 8;
						Stepping = (si.wProcessorRevision & 0x00ff) >> 0;
						// rely on IsProcessorFeaturePresent() for > 486 features
						// Features |= featureset::i586;
						Features |= featureset::i486SX;
						break;
				}
				break;
		}
		Features |= featureset::i386;
		const uint8 fpu_level = detect_fpu_level();
		if (fpu_level >= 2) {
			Features |= feature::fpu;
		}
		if (fpu_level >= 3) {
			Features |= feature::fsin;
		}

#elif MPT_ARCH_AMD64

		Features |= featureset::amd64;

#endif // MPT_ARCH

		Features |= (IsProcessorFeaturePresent(PF_RDTSC_INSTRUCTION_AVAILABLE) != 0)   ? (feature::tsc | feature::i486) : feature::none;
		Features |= (IsProcessorFeaturePresent(PF_COMPARE_EXCHANGE_DOUBLE) != 0)       ? (feature::cx8 | feature::i486) : feature::none;
		Features |= (IsProcessorFeaturePresent(PF_MMX_INSTRUCTIONS_AVAILABLE) != 0)    ? (feature::mmx | feature::i486) : feature::none;
		Features |= (IsProcessorFeaturePresent(PF_3DNOW_INSTRUCTIONS_AVAILABLE) != 0)  ? (feature::x3dnow) : feature::none;
		Features |= (IsProcessorFeaturePresent(PF_XMMI_INSTRUCTIONS_AVAILABLE) != 0)   ? (feature::sse | feature::x3dnowprefetch | feature::cmov) : feature::none;
		Features |= (IsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE) != 0) ? (feature::sse2) : feature::none;
		Features |= (IsProcessorFeaturePresent(PF_SSE3_INSTRUCTIONS_AVAILABLE) != 0)   ? (feature::sse3) : feature::none;
		Features |= (IsProcessorFeaturePresent(PF_SSSE3_INSTRUCTIONS_AVAILABLE) != 0)  ? (feature::ssse3) : feature::none;
		Features |= (IsProcessorFeaturePresent(PF_SSE4_1_INSTRUCTIONS_AVAILABLE) != 0) ? (feature::sse4_1) : feature::none;
		Features |= (IsProcessorFeaturePresent(PF_SSE4_2_INSTRUCTIONS_AVAILABLE) != 0) ? (feature::sse4_1) : feature::none;
		Features |= (IsProcessorFeaturePresent(PF_AVX_INSTRUCTIONS_AVAILABLE) != 0)    ? (feature::avx) : feature::none;
		Features |= (IsProcessorFeaturePresent(PF_AVX2_INSTRUCTIONS_AVAILABLE) != 0)   ? (feature::avx2 | feature::fma | feature::bmi1) : feature::none;

#elif MPT_OS_DJGPP

		const uint8 cpu_level = detect_cpu_level();
		Features |= (cpu_level >= 3) ? featureset::i386 : feature::none;
		Features |= (cpu_level >= 4) ? featureset::i486SX : feature::none;
		const uint8 fpu_level = detect_fpu_level();
		Features |= (fpu_level >= 2) ? feature::fpu : feature::none;
		Features |= (fpu_level >= 3) ? feature::fsin : feature::none;

#endif

#endif // MPT_ARCH_X86 || MPT_ARCH_AMD64

	}

};



} // namespace x86

namespace amd64 = x86;



} // namespace arch



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_ARCH_X86_AMD64_HPP
