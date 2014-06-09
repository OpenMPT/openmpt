/*
 * misc_util.h
 * -----------
 * Purpose: Various useful utility functions.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include <limits>
#include <string>
#if defined(HAS_TYPE_TRAITS)
#include <type_traits>
#endif
#include <vector>

#include <cmath>
#include <cstring>

#include <time.h>


OPENMPT_NAMESPACE_BEGIN


bool ConvertStrToBool(const std::string &str);
signed char ConvertStrToSignedChar(const std::string &str);
unsigned char ConvertStrToUnsignedChar(const std::string &str);
signed short ConvertStrToSignedShort(const std::string &str);
unsigned short ConvertStrToUnsignedShort(const std::string &str);
signed int ConvertStrToSignedInt(const std::string &str);
unsigned int ConvertStrToUnsignedInt(const std::string &str);
signed long ConvertStrToSignedLong(const std::string &str);
unsigned long ConvertStrToUnsignedLong(const std::string &str);
signed long long ConvertStrToSignedLongLong(const std::string &str);
unsigned long long ConvertStrToUnsignedLongLong(const std::string &str);
float ConvertStrToFloat(const std::string &str);
double ConvertStrToDouble(const std::string &str);
long double ConvertStrToLongDouble(const std::string &str);

template<typename T> inline T ConvertStrTo(const std::string &str); // not defined, generates compiler error for non-specialized types
template<> inline bool ConvertStrTo(const std::string &str) { return ConvertStrToBool(str); }
template<> inline signed char ConvertStrTo(const std::string &str) { return ConvertStrToSignedChar(str); }
template<> inline unsigned char ConvertStrTo(const std::string &str) { return ConvertStrToUnsignedChar(str); }
template<> inline signed short ConvertStrTo(const std::string &str) { return ConvertStrToSignedShort(str); }
template<> inline unsigned short ConvertStrTo(const std::string &str) { return ConvertStrToUnsignedShort(str); }
template<> inline signed int ConvertStrTo(const std::string &str) { return ConvertStrToSignedInt(str); }
template<> inline unsigned int ConvertStrTo(const std::string &str) { return ConvertStrToUnsignedInt(str); }
template<> inline signed long ConvertStrTo(const std::string &str) { return ConvertStrToSignedLong(str); }
template<> inline unsigned long ConvertStrTo(const std::string &str) { return ConvertStrToUnsignedLong(str); }
template<> inline signed long long ConvertStrTo(const std::string &str) { return ConvertStrToSignedLongLong(str); }
template<> inline unsigned long long ConvertStrTo(const std::string &str) { return ConvertStrToUnsignedLongLong(str); }
template<> inline float ConvertStrTo(const std::string &str) { return ConvertStrToFloat(str); }
template<> inline double ConvertStrTo(const std::string &str) { return ConvertStrToDouble(str); }
template<> inline long double ConvertStrTo(const std::string &str) { return ConvertStrToLongDouble(str); }

bool ConvertStrToBool(const std::wstring &str);
signed char ConvertStrToSignedChar(const std::wstring &str);
unsigned char ConvertStrToUnsignedChar(const std::wstring &str);
signed short ConvertStrToSignedShort(const std::wstring &str);
unsigned short ConvertStrToUnsignedShort(const std::wstring &str);
signed int ConvertStrToSignedInt(const std::wstring &str);
unsigned int ConvertStrToUnsignedInt(const std::wstring &str);
signed long ConvertStrToSignedLong(const std::wstring &str);
unsigned long ConvertStrToUnsignedLong(const std::wstring &str);
signed long long ConvertStrToSignedLongLong(const std::wstring &str);
unsigned long long ConvertStrToUnsignedLongLong(const std::wstring &str);
float ConvertStrToFloat(const std::wstring &str);
double ConvertStrToDouble(const std::wstring &str);
long double ConvertStrToLongDouble(const std::wstring &str);

template<typename T> inline T ConvertStrTo(const std::wstring &str); // not defined, generates compiler error for non-specialized types
template<> inline bool ConvertStrTo(const std::wstring &str) { return ConvertStrToBool(str); }
template<> inline signed char ConvertStrTo(const std::wstring &str) { return ConvertStrToSignedChar(str); }
template<> inline unsigned char ConvertStrTo(const std::wstring &str) { return ConvertStrToUnsignedChar(str); }
template<> inline signed short ConvertStrTo(const std::wstring &str) { return ConvertStrToSignedShort(str); }
template<> inline unsigned short ConvertStrTo(const std::wstring &str) { return ConvertStrToUnsignedShort(str); }
template<> inline signed int ConvertStrTo(const std::wstring &str) { return ConvertStrToSignedInt(str); }
template<> inline unsigned int ConvertStrTo(const std::wstring &str) { return ConvertStrToUnsignedInt(str); }
template<> inline signed long ConvertStrTo(const std::wstring &str) { return ConvertStrToSignedLong(str); }
template<> inline unsigned long ConvertStrTo(const std::wstring &str) { return ConvertStrToUnsignedLong(str); }
template<> inline signed long long ConvertStrTo(const std::wstring &str) { return ConvertStrToSignedLongLong(str); }
template<> inline unsigned long long ConvertStrTo(const std::wstring &str) { return ConvertStrToUnsignedLongLong(str); }
template<> inline float ConvertStrTo(const std::wstring &str) { return ConvertStrToFloat(str); }
template<> inline double ConvertStrTo(const std::wstring &str) { return ConvertStrToDouble(str); }
template<> inline long double ConvertStrTo(const std::wstring &str) { return ConvertStrToLongDouble(str); }

template<typename T>
inline T ConvertStrTo(const char *str)
{
	if(!str)
	{
		return T();
	}
	return ConvertStrTo<T>(std::string(str));
}

template<typename T>
inline T ConvertStrTo(const wchar_t *str)
{
	if(!str)
	{
		return T();
	}
	return ConvertStrTo<T>(std::wstring(str));
}


namespace mpt { namespace String {

// Combine a vector of values into a string, separated with the given separator.
// No escaping is performed.
template<typename T>
std::string Combine(const std::vector<T> &vals, const std::string &sep=",")
//-------------------------------------------------------------------------
{
	std::string str;
	for(std::size_t i = 0; i < vals.size(); ++i)
	{
		if(i > 0)
		{
			str += sep;
		}
		str += mpt::ToString(vals[i]);
	}
	return str;
}

// Split the given string at separator positions into individual values returned as a vector.
// An empty string results in an empty vector.
// Leading or trailing separators result in a default-constructed element being inserted before or after the other elements.
template<typename T>
std::vector<T> Split(const std::string &str, const std::string &sep=",")
//----------------------------------------------------------------------
{
	std::vector<T> vals;
	std::size_t pos = 0;
	while(str.find(sep, pos) != std::string::npos)
	{
		vals.push_back(ConvertStrTo<int>(str.substr(pos, str.find(sep, pos) - pos)));
		pos = str.find(sep, pos) + sep.length();
	}
	if(!vals.empty() || (str.substr(pos).length() > 0))
	{
		vals.push_back(ConvertStrTo<T>(str.substr(pos)));
	}
	return vals;
}

} } // namespace mpt::String


// Memset given object to zero.
template <class T>
inline void MemsetZero(T &a)
//--------------------------
{
#ifdef HAS_TYPE_TRAITS
	static_assert(std::is_pointer<T>::value == false, "Won't memset pointers.");
	static_assert(std::is_pod<T>::value == true, "Won't memset non-pods.");
#endif
	std::memset(&a, 0, sizeof(T));
}


// Copy given object to other location.
template <class T>
inline T &MemCopy(T &destination, const T &source)
//------------------------------------------------
{
#ifdef HAS_TYPE_TRAITS
	static_assert(std::is_pointer<T>::value == false, "Won't copy pointers.");
	static_assert(std::is_pod<T>::value == true, "Won't copy non-pods.");
#endif
	return *static_cast<T *>(std::memcpy(&destination, &source, sizeof(T)));
}


namespace mpt {

// Saturate the value of src to the domain of Tdst
template <typename Tdst, typename Tsrc>
inline Tdst saturate_cast(Tsrc src)
//---------------------------------
{
	// This code tries not only to obviously avoid overflows but also to avoid signed/unsigned comparison warnings and type truncation warnings (which in fact would be safe here) by explicit casting.
	STATIC_ASSERT(std::numeric_limits<Tdst>::is_integer);
	STATIC_ASSERT(std::numeric_limits<Tsrc>::is_integer);
	if(std::numeric_limits<Tdst>::is_signed && std::numeric_limits<Tsrc>::is_signed)
	{
		if(sizeof(Tdst) >= sizeof(Tsrc))
		{
			return static_cast<Tdst>(src);
		}
		return static_cast<Tdst>(std::max<Tsrc>(std::numeric_limits<Tdst>::min(), std::min<Tsrc>(src, std::numeric_limits<Tdst>::max())));
	} else if(!std::numeric_limits<Tdst>::is_signed && !std::numeric_limits<Tsrc>::is_signed)
	{
		if(sizeof(Tdst) >= sizeof(Tsrc))
		{
			return static_cast<Tdst>(src);
		}
		return static_cast<Tdst>(std::min<Tsrc>(src, std::numeric_limits<Tdst>::max()));
	} else if(std::numeric_limits<Tdst>::is_signed && !std::numeric_limits<Tsrc>::is_signed)
	{
		if(sizeof(Tdst) >= sizeof(Tsrc))
		{
			return static_cast<Tdst>(std::min<Tsrc>(src, static_cast<Tsrc>(std::numeric_limits<Tdst>::max())));
		}
		return static_cast<Tdst>(std::max<Tsrc>(std::numeric_limits<Tdst>::min(), std::min<Tsrc>(src, std::numeric_limits<Tdst>::max())));
	} else // Tdst unsigned, Tsrc signed
	{
		if(sizeof(Tdst) >= sizeof(Tsrc))
		{
			return static_cast<Tdst>(std::max<Tsrc>(0, src));
		}
		return static_cast<Tdst>(std::max<Tsrc>(0, std::min<Tsrc>(src, std::numeric_limits<Tdst>::max())));
	}
}

template <typename Tdst>
inline Tdst saturate_cast(double src)
//-----------------------------------
{
	if(src >= std::numeric_limits<Tdst>::max())
	{
		return std::numeric_limits<Tdst>::max();
	}
	if(src <= std::numeric_limits<Tdst>::min())
	{
		return std::numeric_limits<Tdst>::min();
	}
	return static_cast<Tdst>(src);
}

template <typename Tdst>
inline Tdst saturate_cast(float src)
//----------------------------------
{
	if(src >= std::numeric_limits<Tdst>::max())
	{
		return std::numeric_limits<Tdst>::max();
	}
	if(src <= std::numeric_limits<Tdst>::min())
	{
		return std::numeric_limits<Tdst>::min();
	}
	return static_cast<Tdst>(src);
}

} // namespace mpt


// Limits 'val' to given range. If 'val' is less than 'lowerLimit', 'val' is set to value 'lowerLimit'.
// Similarly if 'val' is greater than 'upperLimit', 'val' is set to value 'upperLimit'.
// If 'lowerLimit' > 'upperLimit', 'val' won't be modified.
template<class T, class C>
inline void Limit(T& val, const C lowerLimit, const C upperLimit)
//---------------------------------------------------------------
{
	if(lowerLimit > upperLimit) return;
	if(val < lowerLimit) val = lowerLimit;
	else if(val > upperLimit) val = upperLimit;
}


// Like Limit, but returns value
template<class T, class C>
inline T Clamp(T val, const C lowerLimit, const C upperLimit)
//-----------------------------------------------------------
{
	if(val < lowerLimit) return lowerLimit;
	else if(val > upperLimit) return upperLimit;
	else return val;
}

// Check if val is in [lo,hi] without causing compiler warnings
// if theses checks are always true due to the domain of T.
// GCC does not warn if the type is templated.
template<typename T, typename C>
inline bool IsInRange(T val, C lo, C hi)
//--------------------------------------
{
	return lo <= val && val <= hi;
}

// Like Limit, but with upperlimit only.
template<class T, class C>
inline void LimitMax(T& val, const C upperLimit)
//----------------------------------------------
{
	if(val > upperLimit)
		val = upperLimit;
}


// Greatest Common Divisor.
template <class T>
T gcd(T a, T b)
//-------------
{
	if(a < 0)
		a = -a;
	if(b < 0)
		b = -b;
	do
	{
		if(a == 0)
			return b;
		b %= a;
		if(b == 0)
			return a;
		a %= b;
	} while(true);
}


// Returns sign of a number (-1 for negative numbers, 1 for positive numbers, 0 for 0)
template <class T>
int sgn(T value)
//--------------
{
	return (value > T(0)) - (value < T(0));
}


// Convert a variable-length MIDI integer held in the byte buffer <value> to a normal integer <result>.
// maxLength bytes are read from the byte buffer at max.
// Function returns how many bytes have been read.
// TODO This should report overflow errors if TOut is too small!
template <class TOut>
size_t ConvertMIDI2Int(TOut &result, uint8 *value, size_t maxLength)
//------------------------------------------------------------------
{
	static_assert(std::numeric_limits<TOut>::is_integer == true, "Output type is a not an integer");

	if(maxLength <= 0)
	{
		result = 0;
		return 0;
	}
	size_t bytesUsed = 0;
	result = 0;
	uint8 b;
	do
	{
		b = *value;
		result <<= 7;
		result |= (b & 0x7F);
		value++;
	} while (++bytesUsed < maxLength && (b & 0x80) != 0);
	return bytesUsed;
}


// Convert an integer <value> to a variable-length MIDI integer, held in the byte buffer <result>.
// maxLength bytes are written to the byte buffer at max.
// Function returns how many bytes have been written.
template <class TIn>
size_t ConvertInt2MIDI(uint8 *result, size_t maxLength, TIn value)
//----------------------------------------------------------------
{
	static_assert(std::numeric_limits<TIn>::is_integer == true, "Input type is a not an integer");

	if(maxLength <= 0)
	{
		*result = 0;
		return 0;
	}
	size_t bytesUsed = 0;
	do
	{
		*result = (value & 0x7F);
		value >>= 7;
		if(value != 0)
		{
			*result |= 0x80;
		}
		result++;
	} while (++bytesUsed < maxLength && value != 0);
	return bytesUsed;
}


namespace Util
{

	time_t MakeGmTime(tm *timeUtc);

	// Minimum of 3 values
	template <class T> inline const T& Min(const T& a, const T& b, const T& c) {return std::min(std::min(a, b), c);}

	// Returns maximum value of given integer type.
	template <class T> inline T MaxValueOfType(const T&) {static_assert(std::numeric_limits<T>::is_integer == true, "Only integer types are allowed."); return (std::numeric_limits<T>::max)();}

	/// Returns value rounded to nearest integer.
#if (MPT_COMPILER_MSVC && MPT_MSVC_BEFORE(2012,0)) || defined(ANDROID)
	// revisit this with vs2012
	inline double Round(const double& val) {if(val >= 0.0) return std::floor(val + 0.5); else return std::ceil(val - 0.5);}
	inline float Round(const float& val) {if(val >= 0.0f) return std::floor(val + 0.5f); else return std::ceil(val - 0.5f);}
#else
	inline double Round(const double& val) { return std::round(val); }
	inline float Round(const float& val) { return std::round(val); }
#endif

	/// Rounds given double value to nearest integer value of type T.
	template <class T> inline T Round(const double& val)
	{
		static_assert(std::numeric_limits<T>::is_integer == true, "Type is a not an integer");
		const double valRounded = Round(val);
		ASSERT(valRounded >= (std::numeric_limits<T>::min)() && valRounded <= (std::numeric_limits<T>::max)());
		const T intval = static_cast<T>(valRounded);
		return intval;
	}

	template <class T> inline T Round(const float& val)
	{
		static_assert(std::numeric_limits<T>::is_integer == true, "Type is a not an integer");
		const float valRounded = Round(val);
		ASSERT(valRounded >= (std::numeric_limits<T>::min)() && valRounded <= (std::numeric_limits<T>::max)());
		const T intval = static_cast<T>(valRounded);
		return intval;
	}

	template<typename T>
	T Weight(T x)
	{
		STATIC_ASSERT(std::numeric_limits<T>::is_integer);
		T c;
		for(c = 0; x; x >>= 1)
		{
			c += x & 1;
		}
		return c;
	}
	
}

namespace Util {

	inline int32 muldiv(int32 a, int32 b, int32 c)
	{
		return static_cast<int32>( ( static_cast<int64>(a) * b ) / c );
	}

	inline int32 muldivr(int32 a, int32 b, int32 c)
	{
		return static_cast<int32>( ( static_cast<int64>(a) * b + ( c / 2 ) ) / c );
	}

	// Do not use overloading because catching unsigned version by accident results in slower X86 code.
	inline uint32 muldiv_unsigned(uint32 a, uint32 b, uint32 c)
	{
		return static_cast<uint32>( ( static_cast<uint64>(a) * b ) / c );
	}
	inline uint32 muldivr_unsigned(uint32 a, uint32 b, uint32 c)
	{
		return static_cast<uint32>( ( static_cast<uint64>(a) * b + ( c / 2 ) ) / c );
	}

	inline int32 muldivrfloor(int64 a, uint32 b, uint32 c)
	{
		a *= b;
		a += c / 2;
		return (a >= 0) ? mpt::saturate_cast<int32>(a / c) : mpt::saturate_cast<int32>((a - (c - 1)) / c);
	}

	template<typename T, std::size_t n>
	class fixed_size_queue
	{
	private:
		T buffer[n+1];
		std::size_t read_position;
		std::size_t write_position;
	public:
		fixed_size_queue() : read_position(0), write_position(0)
		{
			return;
		}
		void clear()
		{
			read_position = 0;
			write_position = 0;
		}
		std::size_t read_size() const
		{
			if ( write_position > read_position )
			{
				return write_position - read_position;
			} else if ( write_position < read_position )
			{
				return write_position - read_position + n + 1;
			} else
			{
				return 0;
			}
		}
		std::size_t write_size() const
		{
			if ( write_position > read_position )
			{
				return read_position - write_position + n;
			} else if ( write_position < read_position )
			{
				return read_position - write_position - 1;
			} else
			{
				return n;
			}
		}
		bool push( const T & v )
		{
			if ( !write_size() )
			{
				return false;
			}
			buffer[write_position] = v;
			write_position = ( write_position + 1 ) % ( n + 1 );
			return true;
		}
		bool pop() {
			if ( !read_size() )
			{
				return false;
			}
			read_position = ( read_position + 1 ) % ( n + 1 );
			return true;
		}
		T peek() {
			if ( !read_size() )
			{
				return T();
			}
			return buffer[read_position];
		}
		const T * peek_p()
		{
			if ( !read_size() )
			{
				return nullptr;
			}
			return &(buffer[read_position]);
		}
		const T * peek_next_p()
		{
			if ( read_size() < 2 )
			{
				return nullptr;
			}
			return &(buffer[(read_position+1)%(n+1)]);
		}
	};

} // namespace Util


#ifdef MODPLUG_TRACKER

namespace Util
{

// RAII wrapper around timeBeginPeriod/timeEndPeriod/timeGetTime (on Windows).
// This clock is monotonic, even across changing its resolution.
// This is needed to synchronize time in Steinberg APIs (ASIO and VST).
class MultimediaClock
{
private:
	uint32 m_CurrentPeriod;
private:
	void Init();
	void SetPeriod(uint32 ms);
	void Cleanup();
public:
	MultimediaClock();
	MultimediaClock(uint32 ms);
	~MultimediaClock();
public:
	// Sets the desired resolution in milliseconds, returns the obtained resolution in milliseconds.
	// A parameter of 0 causes the resolution to be reset to system defaults.
	// A return value of 0 means the resolution is unknown, but timestamps will still be valid.
	uint32 SetResolution(uint32 ms);
	// Returns obtained resolution in milliseconds.
	// A return value of 0 means the resolution is unknown, but timestamps will still be valid.
	uint32 GetResolution() const; 
	// Returns current instantaneous timestamp in milliseconds.
	// The epoch (offset) of the timestamps is undefined but constant until the next system reboot.
	// The resolution is the value returned from GetResolution().
	uint32 Now() const;
	// Returns current instantaneous timestamp in nanoseconds.
	// The epoch (offset) of the timestamps is undefined but constant until the next system reboot.
	// The resolution is the value returned from GetResolution() in milliseconds.
	uint64 NowNanoseconds() const;
};

} // namespace Util

#endif

#ifdef ENABLE_ASM
#define PROCSUPPORT_MMX        0x00001 // Processor supports MMX instructions
#define PROCSUPPORT_SSE        0x00010 // Processor supports SSE instructions
#define PROCSUPPORT_SSE2       0x00020 // Processor supports SSE2 instructions
#define PROCSUPPORT_SSE3       0x00040 // Processor supports SSE3 instructions
#define PROCSUPPORT_AMD_MMXEXT 0x10000 // Processor supports AMD MMX extensions
#define PROCSUPPORT_AMD_3DNOW  0x20000 // Processor supports AMD 3DNow! instructions
#define PROCSUPPORT_AMD_3DNOW2 0x40000 // Processor supports AMD 3DNow!2 instructions
extern uint32 ProcSupport;
void InitProcSupport();
static inline uint32 GetProcSupport()
{
	return ProcSupport;
}
#endif // ENABLE_ASM


#ifdef MODPLUG_TRACKER

namespace Util
{

// COM CLSID<->string conversion
// A CLSID string is not necessarily a standard UUID string,
// it might also be a symbolic name for the interface.
// (see CLSIDFromString ( http://msdn.microsoft.com/en-us/library/windows/desktop/ms680589%28v=vs.85%29.aspx ))
std::wstring CLSIDToString(CLSID clsid);
CLSID StringToCLSID(const std::wstring &str);
bool VerifyStringToCLSID(const std::wstring &str, CLSID &clsid);
bool IsCLSID(const std::wstring &str);

// COM IID<->string conversion
IID StringToIID(const std::wstring &str);
std::wstring IIDToString(IID iid);

// General GUID<->string conversion.
// The string must/will be in standard GUID format: {4F9A455D-E7EF-4367-B2F0-0C83A38A5C72}
GUID StringToGUID(const std::wstring &str);
std::wstring GUIDToString(GUID guid);

// General UUID<->string conversion.
// The string must/will be in standard UUID format: 4f9a455d-e7ef-4367-b2f0-0c83a38a5c72
UUID StringToUUID(const std::wstring &str);
std::wstring UUIDToString(UUID uuid);

// Checks the UUID against the NULL UUID. Returns false if it is NULL, true otherwise.
bool IsValid(UUID uuid);

// Create a COM GUID
GUID CreateGUID();

// Create a UUID
UUID CreateUUID();

// Create a UUID that contains local, traceable information. Safe for local use.
UUID CreateLocalUUID();

// Returns temporary directory (with trailing backslash added) (e.g. "C:\TEMP\")
mpt::PathString GetTempDirectory();

// Returns a new unique absolute path.
mpt::PathString CreateTempFileName(const mpt::PathString &fileNamePrefix = mpt::PathString(), const mpt::PathString &fileNameExtension = MPT_PATHSTRING("tmp"));

} // namespace Util

#endif // MODPLUG_TRACKER

namespace Util
{

std::wstring BinToHex(const std::vector<char> &src);
std::vector<char> HexToBin(const std::wstring &src);

} // namespace Util


#if MPT_OS_WINDOWS

namespace mpt
{
namespace Windows
{
namespace Version
{


enum Number
{

	// Win9x
	Win98    = 0x040a,
	WinME    = 0x045a,

	// WinNT
	WinNT4   = 0x0400,
	Win2000  = 0x0500,
	WinXP    = 0x0501,
	WinVista = 0x0600,
	Win7     = 0x0601,
	Win8     = 0x0602,
	Win81    = 0x0603,

};


#if defined(MODPLUG_TRACKER)

// Initializes version information.
// Version information is cached in order to be safely available in audio real-time contexts.
// Checking version information (especially detecting Wine) can be slow.
void Init();

bool IsAtLeast(mpt::Windows::Version::Number version);
bool IsNT();
bool IsWine();

#else // !MODPLUG_TRACKER

bool IsAtLeast(mpt::Windows::Version::Number version);

#endif // MODPLUG_TRACKER


} // namespace Version
} // namespace Windows
} // namespace mpt

#endif // MPT_OS_WINDOWS


#if defined(MPT_WITH_DYNBIND)

namespace mpt
{


#if MPT_OS_WINDOWS

// Returns the application path or an empty string (if unknown), e.g. "C:\mptrack\"
mpt::PathString GetAppPath();

// Returns the system directory path, e.g. "C:\Windows\System32\"
mpt::PathString GetSystemPath();

// Returns the absolute path for a potentially relative path and removes ".." or "." components. (same as GetFullPathNameW)
mpt::PathString GetAbsolutePath(const mpt::PathString &path);

#endif // MPT_OS_WINDOWS


typedef void* (*FuncPtr)(); // pointer to function returning void*

class LibraryHandle;

enum LibrarySearchPath
{
	LibrarySearchPathDefault,
	LibrarySearchPathApplication,
	LibrarySearchPathSystem,
	LibrarySearchPathFullPath,
};

class LibraryPath
{

private:
	
	mpt::LibrarySearchPath searchPath;
	mpt::PathString fileName;

private:

	LibraryPath(mpt::LibrarySearchPath searchPath, const mpt::PathString &fileName);

public:

	mpt::LibrarySearchPath GetSearchPath() const;

	mpt::PathString GetFileName() const;

public:
	
	// "lib" on Unix-like systems, "" on Windows
	static mpt::PathString GetDefaultPrefix();

	// ".so" or ".dylib" or ".dll"
	static mpt::PathString GetDefaultSuffix();

	// Returns the library path in the application directory, with os-specific prefix and suffix added to basename.
	// e.g.: basename = "unmo3" --> "libunmo3.so" / "apppath/unmo3.dll"
	static LibraryPath App(const mpt::PathString &basename);

	// Returns the library path in the application directory, with os-specific suffix added to fullname.
	// e.g.: fullname = "libunmo3" --> "libunmo3.so" / "apppath/libunmo3.dll" 
	static LibraryPath AppFullName(const mpt::PathString &fullname);

	// Returns a system library name with os-specific prefix and suffix added to basename, but without any full path in order to be searched in the default search path.
	// e.g.: basename = "unmo3" --> "libunmo3.so" / "unmo3.dll"
	static LibraryPath System(const mpt::PathString &basename);

	// Returns a system library name with os-specific suffix added to path.
	// e.g.: path = "somepath/foo" --> "somepath/foo.so" / "somepath/foo.dll"
	static LibraryPath FullPath(const mpt::PathString &path);

};

class Library
{
protected:
	MPT_SHARED_PTR<LibraryHandle> m_Handle;
public:
	Library();
	Library(const mpt::LibraryPath &path);
public:
	void Unload();
	bool IsValid() const;
	FuncPtr GetProcAddress(const std::string &symbol) const;
	template <typename Tfunc>
	bool Bind(Tfunc * & f, const std::string &symbol) const
	{
		#ifdef HAS_TYPE_TRAITS
			#if (MPT_COMPILER_MSVC && MPT_MSVC_AT_LEAST(2013,0)) || !MPT_COMPILER_MSVC
				// MSVC std::is_function is always false for non __cdecl functions.
				// See https://connect.microsoft.com/VisualStudio/feedback/details/774720/stl-is-function-bug .
				// Revisit with VS2013.
				STATIC_ASSERT(std::is_function<Tfunc>::value);
			#endif
		#endif
		const FuncPtr addr = GetProcAddress(symbol);
		f = reinterpret_cast<Tfunc*>(addr);
		return (addr != nullptr);
	}
};

} // namespace mpt

#endif // MPT_WITH_DYNBIND


OPENMPT_NAMESPACE_END
