/*
 * typedefs.h
 * ----------
 * Purpose: Basic data type definitions and assorted compiler-related helpers.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once


// Definitions for MSVC versions to write more understandable conditional-compilation,
// e.g. #if (_MSC_VER > MSVC_VER_2008) instead of #if (_MSC_VER > 1500) 
#define MSVC_VER_VC9		1500
#define MSVC_VER_2008		MSVC_VER_VC9
#define MSVC_VER_VC10		1600
#define MSVC_VER_2010		MSVC_VER_VC10

#if defined(_MSC_VER)
#pragma warning(error : 4309) // Treat "truncation of constant value"-warning as error.
#endif


#if defined(_MSC_VER) && (_MSC_VER < MSVC_VER_2010)
	#define nullptr		0
#endif


#if defined(_MSC_VER) && (_MSC_VER >= MSVC_VER_2010)
#define HAS_TYPE_TRAITS
#endif


//  CountOf macro computes the number of elements in a statically-allocated array.
#ifdef _MSC_VER
	#define CountOf(x) _countof(x)
#else
	#define CountOf(x) (sizeof((x))/sizeof((x)[0]))
#endif


#if defined(_MSC_VER)
#define PACKED __declspec(align(1))
#define NEEDS_PRAGMA_PACK
#elif defined(__GNUC__)
#define PACKED __attribute__((packed))) __attribute__((aligned(1))))
#endif

#if defined(_MSC_VER)
#define ALIGN(n) __declspec(align(n))
#elif defined(__GNUC__)
#define ALIGN(n) __attribute__((aligned(n)))
#endif


#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif


#if !defined(_MFC_VER)
void AssertHandler(const char *file, int line, const char *function, const char *expr);
#if defined(_DEBUG)
#define ASSERT(expr) do { if(!(expr)) { AssertHandler(__FILE__, __LINE__, __FUNCTION__, #expr); } } while(0)
#else
#define ASSERT(expr) do { } while(0)
#endif
#endif


#if defined(_DEBUG)
#define ALWAYS_ASSERT(expr) ASSERT(expr)
#else
void AlwaysAssertHandler(const char *file, int line, const char *function, const char *expr);
#define ALWAYS_ASSERT(expr) do { if(!(expr)) { AlwaysAssertHandler(__FILE__, __LINE__, __FUNCTION__, #expr); } } while(0)
#endif


// Compile time assert.
#if defined(_MSC_VER) && (_MSC_VER < MSVC_VER_2010)
	#define static_assert(expr, msg) typedef char OPENMPT_STATIC_ASSERT[(expr)?1:-1]
#endif
#define STATIC_ASSERT(expr) static_assert((expr), "compile time assertion failed: " #expr)


// Advanced inline attributes
#if defined(_MSC_VER)
#define forceinline __forceinline
#define noinline __declspec(noinline)
#elif defined(__GNUC__)
#define forceinline __attribute__((always_inline)) inline
#define noinline __attribute__((noinline))
#else
#define forceinline inline
#define noinline
#endif

// Some functions might be deprecated although they are still in use.
// Tag them with "DEPRECATED".
#if defined(_MSC_VER)
#define DEPRECATED __declspec(deprecated)
#elif defined(__GNUC__)
#define DEPRECATED __attribute__((deprecated))
#else
#define DEPRECATED
#endif


#if defined(_MSC_VER) && (_MSC_VER <= MSVC_VER_2008)

typedef __int8 int8;
typedef __int16 int16;
typedef __int32 int32;
typedef __int64 int64;
typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;
typedef unsigned __int64 uint64;

const int8 int8_min	    = -127-1;
const int16 int16_min   = -32767-1;
const int32 int32_min   = -2147483647-1;
const int64 int64_min   = -9223372036854775807-1;

const int8 int8_max     = 127;
const int16 int16_max   = 32767;
const int32 int32_max   = 2147483647;
const int64 int64_max   = 9223372036854775807;

const uint8 uint8_max   = 255;
const uint16 uint16_max = 65535;
const uint32 uint32_max = 4294967295;
const uint64 uint64_max = 18446744073709551615;

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

#endif


#if defined(WIN32) || defined(_WIN32)
#include <wtypes.h>
#else // !WIN32
// openmpt assumes these type have exact WIN32 semantics
#define VOID void
typedef std::uint8_t  BYTE;
typedef std::uint16_t WORD;
typedef std::uint32_t DWORD;
typedef std::uint64_t QWORD;
typedef std::int8_t   CHAR;
typedef std::int16_t  SHORT;
typedef std::int32_t  INT;
typedef std::int32_t  LONG;
typedef std::uint8_t  UCHAR;
typedef std::uint16_t USHORT;
typedef std::uint32_t UINT;
typedef std::uint32_t ULONG;
typedef VOID *        LPVOID;
typedef VOID *        PVOID;
typedef std::int8_t   CHAR;
typedef char          TCHAR;
typedef const char *  LPCSTR;
typedef char *        LPSTR;
typedef const char *  LPCTSTR;
typedef char *        LPTSTR;
#endif // WIN32


typedef float float32;
STATIC_ASSERT(sizeof(float32) == 4);

union FloatInt32
{
	float32 f;
	uint32 i;
};
STATIC_ASSERT(sizeof(FloatInt32) == 4);


#include <cstdarg>
#ifdef _MSC_VER
#ifndef va_copy
#define va_copy(dst, src) do { (dst) = (src); } while (0)
#endif
#endif


#include "../common/mptString.h"

//To mark string that should be translated in case of multilingual version.
#define GetStrI18N(x)	(x)


#ifndef NO_LOGGING
void Log(const char *format, ...);
class Logger
{
private:
	const char * const file;
	int const line;
	const char * const function;
public:
	Logger(const char *file_, int line_, const char *function_) : file(file_), line(line_), function(function_) {}
	void operator () (const char *format, ...);
};
#define Log Logger(__FILE__, __LINE__, __FUNCTION__)
#else // !NO_LOGGING
inline void Log(const char *format, ...) {}
class Logger { public: void operator () (const char *format, ...) {} };
#define Log if(true) {} else Logger() // completely compile out arguments to Log() so that they do not even get evaluated
#endif // NO_LOGGING

// just #undef Log in files, where this Log redefinition causes problems
//#undef Log


#include <stdio.h>
#ifdef _MSC_VER
int c99_vsnprintf(char *str, size_t size, const char *format, va_list args);
int c99_snprintf(char *str, size_t size, const char *format, ...);
#define snprintf c99_snprintf
#endif

