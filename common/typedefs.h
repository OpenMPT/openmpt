/*
 * typedefs.h
 * ----------
 * Purpose: Basic data type definitions and assorted compiler-related helpers.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once



#if MPT_COMPILER_MSVC
#pragma warning(error : 4309) // Treat "truncation of constant value"-warning as error.
#endif



#if MPT_COMPILER_MSVC && MPT_MSVC_AT_LEAST(2010,0)
#define MPT_COMPILER_HAS_RVALUE_REF
#endif



#if MPT_COMPILER_MSVC && MPT_MSVC_AT_LEAST(2010,0)
#define HAS_TYPE_TRAITS
#endif



#if MPT_COMPILER_MSVC

#if MPT_MSVC_BEFORE(2010,0)
#define nullptr		0
#endif

#elif MPT_COMPILER_GCC

#if MPT_GCC_BEFORE(4,6,0)
#define nullptr 0
#endif

#endif



//  CountOf macro computes the number of elements in a statically-allocated array.
#if MPT_COMPILER_MSVC
	#define CountOf(x) _countof(x)
#else
	#define CountOf(x) (sizeof((x))/sizeof((x)[0]))
#endif



#if MPT_COMPILER_MSVC
#define PACKED __declspec(align(1))
#define NEEDS_PRAGMA_PACK
#elif MPT_COMPILER_GCC || MPT_COMPILER_CLANG
#define PACKED __attribute__((packed)) __attribute__((aligned(1)))
#else
#define PACKED alignas(1)
#endif



#if MPT_COMPILER_MSVC
#define ALIGN(n) __declspec(align(n))
#elif MPT_COMPILER_GCC || MPT_COMPILER_CLANG
#define ALIGN(n) __attribute__((aligned(n)))
#else
#define ALIGN(n) alignas(n)
#endif



// Advanced inline attributes
#if MPT_COMPILER_MSVC
#define forceinline __forceinline
#define noinline __declspec(noinline)
#elif MPT_COMPILER_GCC || MPT_COMPILER_CLANG
#define forceinline __attribute__((always_inline)) inline
#define noinline __attribute__((noinline))
#else
#define forceinline inline
#define noinline
#endif



// Some functions might be deprecated although they are still in use.
// Tag them with "DEPRECATED".
#if MPT_COMPILER_MSVC
#define DEPRECATED __declspec(deprecated)
#elif MPT_COMPILER_GCC || MPT_COMPILER_CLANG
#define DEPRECATED __attribute__((deprecated))
#else
#define DEPRECATED
#endif



// Exception type that is used to catch "operator new" exceptions.
#if defined(_MFC_VER)
typedef CMemoryException * MPTMemoryException;
#else
#include <new>
typedef std::bad_alloc & MPTMemoryException;
#endif



#include <memory>
#if MPT_COMPILER_MSVC && MPT_MSVC_BEFORE(2010,0)
#define MPT_SHARED_PTR std::tr1::shared_ptr
#else
#define MPT_SHARED_PTR std::shared_ptr
#endif



#if !defined(_MFC_VER)
void AssertHandler(const char *file, int line, const char *function, const char *expr);
#if defined(_DEBUG)
#define ASSERT(expr) do { if(!(expr)) { AssertHandler(__FILE__, __LINE__, __FUNCTION__, #expr); } } while(0)
#else
#define ASSERT(expr) do { } while(0)
#endif
#endif

#define ASSERT_WARN_MESSAGE(expr,msg) ASSERT(expr)

#if defined(_DEBUG)
#define ALWAYS_ASSERT(expr) ASSERT(expr)
#define ALWAYS_ASSERT_WARN_MESSAGE(expr,msg) ASSERT(expr)
#else
void AlwaysAssertHandler(const char *file, int line, const char *function, const char *expr, const char *msg=nullptr);
#define ALWAYS_ASSERT(expr) do { if(!(expr)) { AlwaysAssertHandler(__FILE__, __LINE__, __FUNCTION__, #expr); } } while(0)
#define ALWAYS_ASSERT_WARN_MESSAGE(expr,msg) do { if(!(expr)) { AlwaysAssertHandler(__FILE__, __LINE__, __FUNCTION__, #expr, msg); } } while(0)
#endif

// Compile time assert.
#if MPT_COMPILER_MSVC && MPT_MSVC_BEFORE(2010,0)
	#define static_assert(expr, msg) typedef char OPENMPT_STATIC_ASSERT[(expr)?1:-1]
#endif
#define STATIC_ASSERT(expr) static_assert((expr), "compile time assertion failed: " #expr)



#include <cstdarg>
#if MPT_COMPILER_MSVC
#ifndef va_copy
#define va_copy(dst, src) do { (dst) = (src); } while (0)
#endif
#endif



#if MPT_COMPILER_MSVC && MPT_MSVC_BEFORE(2010,0)

#define __STDC_LIMIT_MACROS
#include "stdint.h"

typedef int8_t   int8;
typedef int16_t  int16;
typedef int32_t  int32;
typedef int64_t  int64;
typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

#else

#include <cstdint>

typedef std::int8_t   int8;
typedef std::int16_t  int16;
typedef std::int32_t  int32;
typedef std::int64_t  int64;
typedef std::uint8_t  uint8;
typedef std::uint16_t uint16;
typedef std::uint32_t uint32;
typedef std::uint64_t uint64;

#endif

const int8 int8_min     = INT8_MIN;
const int16 int16_min   = INT16_MIN;
const int32 int32_min   = INT32_MIN;
const int64 int64_min   = INT64_MIN;

const int8 int8_max     = INT8_MAX;
const int16 int16_max   = INT16_MAX;
const int32 int32_max   = INT32_MAX;
const int64 int64_max   = INT64_MAX;

const uint8 uint8_max   = UINT8_MAX;
const uint16 uint16_max = UINT16_MAX;
const uint32 uint32_max = UINT32_MAX;
const uint64 uint64_max = UINT64_MAX;

struct int24
{
	uint8 bytes[3];
	int24() { bytes[0] = bytes[1] = bytes[2] = 0; }
	explicit int24(int other)
	{
		#ifdef MPT_PLATFORM_BIG_ENDIAN
			bytes[0] = ((unsigned int)other>>16)&0xff;
			bytes[1] = ((unsigned int)other>> 8)&0xff;
			bytes[2] = ((unsigned int)other>> 0)&0xff;
		#else
			bytes[0] = ((unsigned int)other>> 0)&0xff;
			bytes[1] = ((unsigned int)other>> 8)&0xff;
			bytes[2] = ((unsigned int)other>>16)&0xff;
		#endif
	}
	operator int() const
	{
		#ifdef MPT_PLATFORM_BIG_ENDIAN
			return ((int8)bytes[0] * 65536) + (bytes[1] * 256) + bytes[2];
		#else
			return ((int8)bytes[2] * 65536) + (bytes[1] * 256) + bytes[0];
		#endif
	}
};
STATIC_ASSERT(sizeof(int24) == 3);
#define int24_min (0-0x00800000)
#define int24_max (0+0x007fffff)

struct fixed5p27
{
	// 5 integer bits (including sign)
	// 27 fractional bits
	int32 raw;

	static fixed5p27 Raw(int32 x) { return fixed5p27().SetRaw(x); }

	fixed5p27& SetRaw(int32 x) { raw = x; return *this; }
	int32 GetRaw() const { return raw; }

};
STATIC_ASSERT(sizeof(fixed5p27) == 4);
#define fixed5p27_min fixed5p27::Raw(int32_min)
#define fixed5p27_max fixed5p27::Raw(int32_max)

struct uint8_4
{
	uint8 x[4];
	uint8_4() { }
	uint8_4(uint8 a, uint8 b, uint8 c, uint8 d) { x[0] = a; x[1] = b; x[2] = c; x[3] = d; }
	uint32 GetBE() const { return (x[0] << 24) | (x[1] << 16) | (x[2] <<  8) | (x[3] <<  0); }
	uint32 GetLE() const { return (x[0] <<  0) | (x[1] <<  8) | (x[2] << 16) | (x[3] << 24); }
	uint8_4 & SetBE(uint32 y) { x[0] = (y >> 24)&0xff; x[1] = (y >> 16)&0xff; x[2] = (y >>  8)&0xff; x[3] = (y >>  0)&0xff; return *this; }
	uint8_4 & SetLE(uint32 y) { x[0] = (y >>  0)&0xff; x[1] = (y >>  8)&0xff; x[2] = (y >> 16)&0xff; x[3] = (y >> 24)&0xff; return *this; }
};
STATIC_ASSERT(sizeof(uint8_4) == 4);

typedef float float32;
STATIC_ASSERT(sizeof(float32) == 4);

union FloatInt32
{
	float32 f;
	uint32 i;
};
STATIC_ASSERT(sizeof(FloatInt32) == 4);



#if defined(_WIN32) && !defined(NO_WINDOWS_H)

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h> // defines WIN32

#define MPT_TEXT(x) TEXT(x)

#else // !_WIN32

// openmpt assumes these type have exact WIN32 semantics

typedef std::int32_t  BOOL;
typedef std::uint8_t  BYTE;
typedef std::uint16_t WORD;
typedef std::uint32_t DWORD;
typedef std::uint64_t QWORD;
typedef std::int8_t   CHAR;
typedef std::int16_t  SHORT;
typedef std::int32_t  INT;
typedef std::int32_t  LONG;
typedef std::int64_t  LONGLONG;
typedef std::uint8_t  UCHAR;
typedef std::uint16_t USHORT;
typedef std::uint32_t UINT;
typedef std::uint32_t ULONG;
typedef std::uint64_t ULONGLONG;
typedef void *        LPVOID;
typedef BYTE *        LPBYTE;
typedef WORD *        LPWORD;
typedef DWORD *       LPDWORD;
typedef INT *         LPINT;
typedef LONG *        LPLONG;

typedef const char *  LPCSTR;
typedef char *        LPSTR;

// for BOOL
#define TRUE (1)
#define FALSE (0)

#define MPT_TEXT(x) x

#endif // _WIN32



#include "../common/mptString.h"

//To mark string that should be translated in case of multilingual version.
#define GetStrI18N(x)	(x)

#define MULTICHAR4_LE_MSVC(a,b,c,d) static_cast<uint32>( (static_cast<uint8>(a) << 24) | (static_cast<uint8>(b) << 16) | (static_cast<uint8>(c) << 8) | (static_cast<uint8>(d) << 0) )



//STRINGIZE makes a string of given argument. If used with #defined value,
//the string is made of the contents of the defined value.
#define HELPER_STRINGIZE(x)			#x
#define STRINGIZE(x)				HELPER_STRINGIZE(x)



#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif



#ifndef NO_LOGGING
void MPT_PRINTF_FUNC(1,2) Log(const char *format, ...);
class Logger
{
private:
	const char * const file;
	int const line;
	const char * const function;
public:
	Logger(const char *file_, int line_, const char *function_) : file(file_), line(line_), function(function_) {}
	void MPT_PRINTF_FUNC(2,3) operator () (const char *format, ...);
};
#define Log Logger(__FILE__, __LINE__, __FUNCTION__)
#else // !NO_LOGGING
static inline void MPT_PRINTF_FUNC(1,2) Log(const char * /*format*/, ...) {}
class Logger { public: void MPT_PRINTF_FUNC(2,3) operator () (const char * /*format*/, ...) {} };
#define Log if(true) {} else Logger() // completely compile out arguments to Log() so that they do not even get evaluated
#endif // NO_LOGGING

// just #undef Log in files, where this Log redefinition causes problems
//#undef Log



#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(x) (void)(x)
#endif

