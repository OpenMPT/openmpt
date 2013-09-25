/*
 * CompilerDetect.h
 * ----------------
 * Purpose: Detect current compiler and provide readable version test macros.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once



#define MPT_COMPILER_MAKE_VERSION2(version,sp)         ((version) * 100 + (sp))
#define MPT_COMPILER_MAKE_VERSION3(major,minor,patch)  ((major) * 10000 + (minor) * 100 + (patch))



#if defined(MPT_COMPILER_GENERIC)

#undef MPT_COMPILER_GENERIC
#define MPT_COMPILER_GENERIC                         1

#elif defined(__clang__)

#define MPT_COMPILER_CLANG                           1
#define MPT_COMPILER_CLANG_VERSION                   MPT_COMPILER_MAKE_VERSION3(__clang_major__,__clang_minor__,__clang_patchlevel__)
#define MPT_CLANG_AT_LEAST(major,minor,patch)        (MPT_COMPILER_CLANG_VERSION >= MPT_COMPILER_MAKE_VERSION3((major),(minor),(patch)))
#define MPT_CLANG_BEFORE(major,minor,patch)          (MPT_COMPILER_CLANG_VERSION <  MPT_COMPILER_MAKE_VERSION3((major),(minor),(patch)))

#if MPT_CLANG_BEFORE(3,0,0)
#error "clang version 3.0 required"
#endif

#elif defined(__GNUC__)

#define MPT_COMPILER_GCC                             1
#define MPT_COMPILER_GCC_VERSION                     MPT_COMPILER_MAKE_VERSION3(__GNUC__,__GNUC_MINOR__,__GNUC_PATCHLEVEL__)
#define MPT_GCC_AT_LEAST(major,minor,patch)          (MPT_COMPILER_GCC_VERSION >= MPT_COMPILER_MAKE_VERSION3((major),(minor),(patch)))
#define MPT_GCC_BEFORE(major,minor,patch)            (MPT_COMPILER_GCC_VERSION <  MPT_COMPILER_MAKE_VERSION3((major),(minor),(patch)))

#if MPT_GCC_BEFORE(4,4,0)
#error "GCC version 4.4 required"
#endif

#elif defined(_MSC_VER)

#define MPT_COMPILER_MSVC                            1
#if (_MSC_VER >= 1600)
#define MPT_COMPILER_MSVC_VERSION                    MPT_COMPILER_MAKE_VERSION2(2010,0)
#elif (_MSC_VER >= 1500)
#define MPT_COMPILER_MSVC_VERSION                    MPT_COMPILER_MAKE_VERSION2(2008,0)
#else
#define MPT_COMPILER_MSVC_VERSION                    MPT_COMPILER_MAKE_VERSION2(2005,0)
#endif
#define MPT_MSVC_AT_LEAST(version,sp)                (MPT_COMPILER_MSVC_VERSION >= MPT_COMPILER_MAKE_VERSION2((version),(sp)))
#define MPT_MSVC_BEFORE(version,sp)                  (MPT_COMPILER_MSVC_VERSION <  MPT_COMPILER_MAKE_VERSION2((version),(sp)))

#if MPT_MSVC_BEFORE(2008,0)
#error "MSVC version 2008 required"
#endif

#else

#error "Your compiler is unknown to openmpt and thus not supported. You might want to edit CompilerDetect.h und typedefs.h."

#endif



#ifndef MPT_COMPILER_GENERIC
#define MPT_COMPILER_GENERIC                  0
#endif
#ifndef MPT_COMPILER_CLANG
#define MPT_COMPILER_CLANG 0
#define MPT_CLANG_AT_LEAST(major,minor,patch) 0
#define MPT_CLANG_BEFORE(major,minor,patch)   0
#endif
#ifndef MPT_COMPILER_GCC
#define MPT_COMPILER_GCC   0
#define MPT_GCC_AT_LEAST(major,minor,patch)   0
#define MPT_GCC_BEFORE(major,minor,patch)     0
#endif
#ifndef MPT_COMPILER_MSVC
#define MPT_COMPILER_MSVC  0
#define MPT_MSVC_AT_LEAST(version,sp)         0
#define MPT_MSVC_BEFORE(version,sp)           0
#endif



#if MPT_COMPILER_MSVC
	#define MPT_PLATFORM_LITTLE_ENDIAN
#elif MPT_COMPILER_GCC
	#if MPT_GCC_AT_LEAST(4,6,0)
		#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
			#define MPT_PLATFORM_BIG_ENDIAN
		#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
			#define MPT_PLATFORM_LITTLE_ENDIAN
		#endif
	#endif
#elif MPT_COMPILER_CLANG
	#if MPT_CLANG_AT_LEAST(3,2,0)
		#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
			#define MPT_PLATFORM_BIG_ENDIAN
		#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
			#define MPT_PLATFORM_LITTLE_ENDIAN
		#endif
	#endif
#endif

// fallback:
#if !defined(MPT_PLATFORM_BIG_ENDIAN) && !defined(MPT_PLATFORM_LITTLE_ENDIAN)
	// taken from boost/detail/endian.hpp
	#if (defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)) \
		|| (defined(__BIG_ENDIAN__) && !defined(__LITTLE_ENDIAN__)) \
		|| (defined(_STLP_BIG_ENDIAN) && !defined(_STLP_LITTLE_ENDIAN))
			#define MPT_PLATFORM_BIG_ENDIAN
	#elif (defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)) \
		|| (defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)) \
		|| (defined(_STLP_LITTLE_ENDIAN) && !defined(_STLP_BIG_ENDIAN))
			#define MPT_PLATFORM_LITTLE_ENDIAN
	#elif defined(__sparc) || defined(__sparc__) \
		|| defined(_POWER) || defined(__powerpc__) \
		|| defined(__ppc__) || defined(__hpux) || defined(__hppa) \
		|| defined(_MIPSEB) || defined(_POWER) \
		|| defined(__s390__)
			#define MPT_PLATFORM_BIG_ENDIAN
	#elif defined(__i386__) || defined(__alpha__) \
		|| defined(__ia64) || defined(__ia64__) \
		|| defined(_M_IX86) || defined(_M_IA64) \
		|| defined(_M_ALPHA) || defined(__amd64) \
		|| defined(__amd64__) || defined(_M_AMD64) \
		|| defined(__x86_64) || defined(__x86_64__) \
		|| defined(_M_X64) || defined(__bfin__)
			#define MPT_PLATFORM_LITTLE_ENDIAN
	#else
		#error "unknown endianness"
	#endif
#endif
