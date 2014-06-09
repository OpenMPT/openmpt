/*
 * misc_util.cpp
 * -------------
 * Purpose: Various useful utility functions.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "misc_util.h"

#include <locale>
#include <sstream>

#include <time.h>

#if defined(MPT_WITH_DYNBIND)
#if !MPT_OS_WINDOWS
#include <dlfcn.h>
#endif
#endif

#if MPT_OS_WINDOWS
#include <windows.h>
#endif

#if defined(MODPLUG_TRACKER)
#include <mmsystem.h>
#endif


OPENMPT_NAMESPACE_BEGIN


template<typename T>
inline T ConvertStrToHelper(const std::string &str)
{
	std::istringstream i(str);
	i.imbue(std::locale::classic());
	T x;
	if(!(i >> x))
	{
		return T();
	}
	return x;
}
template<> inline bool ConvertStrToHelper(const std::string &str) { return ConvertStrToHelper<int>(str)?true:false; }
template<> inline signed char ConvertStrToHelper(const std::string &str) { return static_cast<signed char>(ConvertStrToHelper<signed int>(str)); }
template<> inline unsigned char ConvertStrToHelper(const std::string &str) { return static_cast<unsigned char>(ConvertStrToHelper<unsigned int>(str)); }

template<typename T>
inline T ConvertStrToHelper(const std::wstring &str)
{
	std::wistringstream i(str);
	i.imbue(std::locale::classic());
	T x;
	if(!(i >> x))
	{
		return T();
	}
	return x;
}
template<> inline bool ConvertStrToHelper(const std::wstring &str) { return ConvertStrToHelper<int>(str)?true:false; }
template<> inline signed char ConvertStrToHelper(const std::wstring &str) { return static_cast<signed char>(ConvertStrToHelper<signed int>(str)); }
template<> inline unsigned char ConvertStrToHelper(const std::wstring &str) { return static_cast<unsigned char>(ConvertStrToHelper<unsigned int>(str)); }

bool ConvertStrToBool(const std::string &str) { return ConvertStrToHelper<bool>(str); }
signed char ConvertStrToSignedChar(const std::string &str) { return ConvertStrToHelper<signed char>(str); }
unsigned char ConvertStrToUnsignedChar(const std::string &str) { return ConvertStrToHelper<unsigned char>(str); }
signed short ConvertStrToSignedShort(const std::string &str) { return ConvertStrToHelper<signed short>(str); }
unsigned short ConvertStrToUnsignedShort(const std::string &str) { return ConvertStrToHelper<unsigned short>(str); }
signed int ConvertStrToSignedInt(const std::string &str) { return ConvertStrToHelper<signed int>(str); }
unsigned int ConvertStrToUnsignedInt(const std::string &str) { return ConvertStrToHelper<unsigned int>(str); }
signed long ConvertStrToSignedLong(const std::string &str) { return ConvertStrToHelper<signed long>(str); }
unsigned long ConvertStrToUnsignedLong(const std::string &str) { return ConvertStrToHelper<unsigned long>(str); }
signed long long ConvertStrToSignedLongLong(const std::string &str) { return ConvertStrToHelper<signed long long>(str); }
unsigned long long ConvertStrToUnsignedLongLong(const std::string &str) { return ConvertStrToHelper<unsigned long long>(str); }
float ConvertStrToFloat(const std::string &str) { return ConvertStrToHelper<float>(str); }
double ConvertStrToDouble(const std::string &str) { return ConvertStrToHelper<double>(str); }
long double ConvertStrToLongDouble(const std::string &str) { return ConvertStrToHelper<long double>(str); }

bool ConvertStrToBool(const std::wstring &str) { return ConvertStrToHelper<bool>(str); }
signed char ConvertStrToSignedChar(const std::wstring &str) { return ConvertStrToHelper<signed char>(str); }
unsigned char ConvertStrToUnsignedChar(const std::wstring &str) { return ConvertStrToHelper<unsigned char>(str); }
signed short ConvertStrToSignedShort(const std::wstring &str) { return ConvertStrToHelper<signed short>(str); }
unsigned short ConvertStrToUnsignedShort(const std::wstring &str) { return ConvertStrToHelper<unsigned short>(str); }
signed int ConvertStrToSignedInt(const std::wstring &str) { return ConvertStrToHelper<signed int>(str); }
unsigned int ConvertStrToUnsignedInt(const std::wstring &str) { return ConvertStrToHelper<unsigned int>(str); }
signed long ConvertStrToSignedLong(const std::wstring &str) { return ConvertStrToHelper<signed long>(str); }
unsigned long ConvertStrToUnsignedLong(const std::wstring &str) { return ConvertStrToHelper<unsigned long>(str); }
signed long long ConvertStrToSignedLongLong(const std::wstring &str) { return ConvertStrToHelper<signed long long>(str); }
unsigned long long ConvertStrToUnsignedLongLong(const std::wstring &str) { return ConvertStrToHelper<unsigned long long>(str); }
float ConvertStrToFloat(const std::wstring &str) { return ConvertStrToHelper<float>(str); }
double ConvertStrToDouble(const std::wstring &str) { return ConvertStrToHelper<double>(str); }
long double ConvertStrToLongDouble(const std::wstring &str) { return ConvertStrToHelper<long double>(str); }


namespace Util
{

time_t MakeGmTime(tm *timeUtc)
//----------------------------
{
	#if MPT_COMPILER_MSVC
		return _mkgmtime(timeUtc);
	#else // !MPT_COMPILER_MSVC
		// There is no portable way in C/C++ to convert between time_t and struct tm in UTC.
		// Approximate it as good as possible without implementing full date handling logic.
		// NOTE:
		// This can be wrong for dates during DST switch.
		if(!timeUtc)
		{
			return time_t();
		}
		tm t = *timeUtc;
		time_t localSinceEpoch = mktime(&t);
		const tm * tmpLocal = localtime(&localSinceEpoch);
		if(!tmpLocal)
		{
			return localSinceEpoch;
		}
		tm localTM = *tmpLocal;
		const tm * tmpUTC = gmtime(&localSinceEpoch);
		if(!tmpUTC)
		{
			return localSinceEpoch;
		}
		tm utcTM = *tmpUTC;
		double offset = difftime(mktime(&localTM), mktime(&utcTM));
		double timeScaleFactor = difftime(2, 1);
		time_t utcSinceEpoch = localSinceEpoch + Util::Round<time_t>(offset / timeScaleFactor);
		return utcSinceEpoch;
	#endif // MPT_COMPILER_MSVC
}

} // namespace Util


#ifdef MODPLUG_TRACKER

namespace Util
{

void MultimediaClock::Init()
{
	m_CurrentPeriod = 0;
}

void MultimediaClock::SetPeriod(uint32 ms)
{
	TIMECAPS caps;
	MemsetZero(caps);
	if(timeGetDevCaps(&caps, sizeof(caps)) != MMSYSERR_NOERROR)
	{
		return;
	}
	if((caps.wPeriodMax == 0) || (caps.wPeriodMin > caps.wPeriodMax))
	{
		return;
	}
	ms = Clamp<uint32>(ms, caps.wPeriodMin, caps.wPeriodMax);
	if(timeBeginPeriod(ms) != MMSYSERR_NOERROR)
	{
		return;
	}
	m_CurrentPeriod = ms;
}

void MultimediaClock::Cleanup()
{
	if(m_CurrentPeriod > 0)
	{
		if(timeEndPeriod(m_CurrentPeriod) != MMSYSERR_NOERROR)
		{
			// should not happen
			ASSERT(false);
		}
		m_CurrentPeriod = 0;
	}
}

MultimediaClock::MultimediaClock()
{
	Init();
}

MultimediaClock::MultimediaClock(uint32 ms)
{
	Init();
	SetResolution(ms);
}

MultimediaClock::~MultimediaClock()
{
	Cleanup();
}

uint32 MultimediaClock::SetResolution(uint32 ms)
{
	if(m_CurrentPeriod == ms)
	{
		return m_CurrentPeriod;
	}
	Cleanup();
	SetPeriod(ms);
	return GetResolution();
}

uint32 MultimediaClock::GetResolution() const
{
	return m_CurrentPeriod;
}

uint32 MultimediaClock::Now() const
{
	return timeGetTime();
}

uint64 MultimediaClock::NowNanoseconds() const
{
	return (uint64)timeGetTime() * (uint64)1000000;
}

} // namespace Util

#endif // MODPLUG_TRACKER


#if defined(ENABLE_ASM)


uint32 ProcSupport = 0;


#if MPT_COMPILER_MSVC && (defined(ENABLE_X86) || defined(ENABLE_X64))


#include <intrin.h>


typedef char cpuid_result_string[12];


struct cpuid_result {
	uint32 a;
	uint32 b;
	uint32 c;
	uint32 d;
	std::string as_string() const
	{
		cpuid_result_string result;
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
		return std::string(result, result + 12);
	}
};


static cpuid_result cpuid(uint32 function)
//----------------------------------------
{
	cpuid_result result;
	int CPUInfo[4];
	__cpuid(CPUInfo, function);
	result.a = CPUInfo[0];
	result.b = CPUInfo[1];
	result.c = CPUInfo[2];
	result.d = CPUInfo[3];
	return result;
}


static bool has_cpuid()
//---------------------
{
	const size_t eflags_cpuid = 1 << 21;
	size_t old_eflags = __readeflags();
	__writeeflags(old_eflags ^ eflags_cpuid);
	bool result = ((__readeflags() ^ old_eflags) & eflags_cpuid) != 0;
	__writeeflags(old_eflags);
	return result;
}


void InitProcSupport()
//--------------------
{

	ProcSupport = 0;

	if(has_cpuid())
	{
		cpuid_result VendorString = cpuid(0x00000000u);

		cpuid_result StandardFeatureFlags = cpuid(0x00000001u);
		if(StandardFeatureFlags.d & (1<<23)) ProcSupport |= PROCSUPPORT_MMX;
		if(StandardFeatureFlags.d & (1<<25)) ProcSupport |= PROCSUPPORT_SSE;
		if(StandardFeatureFlags.d & (1<<26)) ProcSupport |= PROCSUPPORT_SSE2;
		if(StandardFeatureFlags.c & (1<< 0)) ProcSupport |= PROCSUPPORT_SSE3;

		if(VendorString.as_string() == "AuthenticAMD")
		{
			cpuid_result ExtendedVendorString = cpuid(0x80000000u);
			if(ExtendedVendorString.a >= 0x80000001u)
			{
				cpuid_result ExtendedFeatureFlags = cpuid(0x80000001u);
				if(ExtendedFeatureFlags.d & (1<<22)) ProcSupport |= PROCSUPPORT_AMD_MMXEXT;
				if(ExtendedFeatureFlags.d & (1<<31)) ProcSupport |= PROCSUPPORT_AMD_3DNOW;
				if(ExtendedFeatureFlags.d & (1<<30)) ProcSupport |= PROCSUPPORT_AMD_3DNOW2;
			}
		}
	}
}


#else // !( MPT_COMPILER_MSVC && ENABLE_X86 )


void InitProcSupport()
//--------------------
{
	ProcSupport = 0;
}


#endif // MPT_COMPILER_MSVC && ENABLE_X86


#endif // ENABLE_ASM



#ifdef MODPLUG_TRACKER

namespace Util
{


std::wstring CLSIDToString(CLSID clsid)
//-------------------------------------
{
	std::wstring str;
	LPOLESTR tmp = nullptr;
	::StringFromCLSID(clsid, &tmp);
	if(tmp)
	{
		str = tmp;
		::CoTaskMemFree(tmp);
		tmp = nullptr;
	}
	return str;
}


CLSID StringToCLSID(const std::wstring &str)
//------------------------------------------
{
	CLSID clsid = CLSID();
	std::vector<OLECHAR> tmp(str.c_str(), str.c_str() + str.length() + 1);
	if(::CLSIDFromString(&tmp[0], &clsid) != S_OK)
	{
		return CLSID();
	}
	return clsid;
}


bool VerifyStringToCLSID(const std::wstring &str, CLSID &clsid)
//-------------------------------------------------------------
{
	std::vector<OLECHAR> tmp(str.c_str(), str.c_str() + str.length() + 1);
	return (::CLSIDFromString(&tmp[0], &clsid) == S_OK);
}


bool IsCLSID(const std::wstring &str)
//-----------------------------------
{
	CLSID clsid = CLSID();
	std::vector<OLECHAR> tmp(str.c_str(), str.c_str() + str.length() + 1);
	return (::CLSIDFromString(&tmp[0], &clsid) == S_OK);
}


std::wstring IIDToString(IID iid)
//-------------------------------
{
	std::wstring str;
	LPOLESTR tmp = nullptr;
	::StringFromIID(iid, &tmp);
	if(tmp)
	{
		str = tmp;
		::CoTaskMemFree(tmp);
		tmp = nullptr;
	}
	return str;
}


IID StringToIID(const std::wstring &str)
//--------------------------------------
{
	IID iid = IID();
	std::vector<OLECHAR> tmp(str.c_str(), str.c_str() + str.length() + 1);
	::IIDFromString(&tmp[0], &iid);
	return iid;
}


std::wstring GUIDToString(GUID guid)
//----------------------------------
{
	std::vector<OLECHAR> tmp(256);
	::StringFromGUID2(guid, &tmp[0], static_cast<int>(tmp.size()));
	return &tmp[0];
}


GUID StringToGUID(const std::wstring &str)
//----------------------------------------
{
	return StringToIID(str);
}


UUID StringToUUID(const std::wstring &str)
//----------------------------------------
{
	UUID uuid = UUID();
	std::vector<wchar_t> tmp(str.c_str(), str.c_str() + str.length() + 1);
	if(::UuidFromStringW((RPC_WSTR)(&(tmp[0])), &uuid) != RPC_S_OK)
	{
		return UUID();
	}
	return uuid;
}


std::wstring UUIDToString(UUID uuid)
//----------------------------------
{
	std::wstring str;
	RPC_WSTR tmp = nullptr;
	if(::UuidToStringW(&uuid, &tmp) != RPC_S_OK)
	{
		return std::wstring();
	}
	str = (wchar_t*)tmp;
	::RpcStringFreeW(&tmp);
	return str;
}


bool IsValid(UUID uuid)
//---------------------
{
	return false
		|| uuid.Data1 != 0
		|| uuid.Data2 != 0
		|| uuid.Data3 != 0
		|| uuid.Data4[0] != 0
		|| uuid.Data4[1] != 0
		|| uuid.Data4[2] != 0
		|| uuid.Data4[3] != 0
		|| uuid.Data4[4] != 0
		|| uuid.Data4[5] != 0
		|| uuid.Data4[6] != 0
		|| uuid.Data4[7] != 0
		;
}


GUID CreateGUID()
//---------------
{
	GUID guid = GUID();
	if(::CoCreateGuid(&guid) != S_OK)
	{
		return GUID();
	}
	return guid;
}


UUID CreateUUID()
//---------------
{
	UUID uuid = UUID();
	RPC_STATUS status = ::UuidCreate(&uuid);
	if(status != RPC_S_OK && status != RPC_S_UUID_LOCAL_ONLY)
	{
		return UUID();
	}
	return uuid;
}


UUID CreateLocalUUID()
//--------------------
{
	UUID uuid = UUID();
	RPC_STATUS status = ::UuidCreateSequential(&uuid);
	if(status != RPC_S_OK && status != RPC_S_UUID_LOCAL_ONLY)
	{
		return UUID();
	}
	return uuid;
}


mpt::PathString GetTempDirectory()
//--------------------------------
{
	WCHAR tempPath[MAX_PATH+2];
	MemsetZero(tempPath);
	DWORD result = GetTempPathW(MAX_PATH+1, tempPath);
	if(result == 0 || result > MAX_PATH+1)
	{ // error
		// use app directory as fallback
		return mpt::GetAppPath();
	}
	return mpt::PathString::FromNative(tempPath);
}


mpt::PathString CreateTempFileName(const mpt::PathString &fileNamePrefix, const mpt::PathString &fileNameExtension)
//-----------------------------------------------------------------------------------------------------------------
{
	mpt::PathString filename = Util::GetTempDirectory();
	filename += (!fileNamePrefix.empty() ? fileNamePrefix + MPT_PATHSTRING("_") : mpt::PathString());
	filename += mpt::PathString::FromWide(Util::UUIDToString(Util::CreateLocalUUID()));
	filename += (!fileNameExtension.empty() ? MPT_PATHSTRING(".") + fileNameExtension : mpt::PathString());
	return filename;
}


} // namespace Util

#endif // MODPLUG_TRACKER


namespace Util
{

	
static const wchar_t EncodeNibble[16] = { L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9', L'A', L'B', L'C', L'D', L'E', L'F' };

static inline bool DecodeByte(uint8 &byte, wchar_t c1, wchar_t c2)
{
	byte = 0;
	if(L'0' <= c1 && c1 <= L'9')
	{
		byte += static_cast<uint8>((c1 - L'0') << 4);
	} else if(L'A' <= c1 && c1 <= L'F')
	{
		byte += static_cast<uint8>((c1 - L'A' + 10) << 4);
	} else if(L'a' <= c1 && c1 <= L'f')
	{
		byte += static_cast<uint8>((c1 - L'a' + 10) << 4);
	} else
	{
		return false;
	}
	if(L'0' <= c2 && c2 <= L'9')
	{
		byte += static_cast<uint8>(c2 - L'0');
	} else if(L'A' <= c2 && c2 <= L'F')
	{
		byte += static_cast<uint8>(c2 - L'A' + 10);
	} else if(L'a' <= c2 && c2 <= L'f')
	{
		byte += static_cast<uint8>(c2 - L'a' + 10);
	} else
	{
		return false;
	}
	return true;
}

std::wstring BinToHex(const std::vector<char> &src)
{
	std::wstring result;
	for(std::size_t i = 0; i < src.size(); ++i)
	{
		uint8 byte = src[i];
		result.push_back(EncodeNibble[(byte&0xf0)>>4]);
		result.push_back(EncodeNibble[byte&0x0f]);
	}
	return result;
}

std::vector<char> HexToBin(const std::wstring &src)
{
	std::vector<char> result;
	for(std::size_t i = 0; i+1 < src.size(); i += 2)
	{
		uint8 byte = 0;
		if(!DecodeByte(byte, src[i*2+0], src[i*2+1]))
		{
			return result;
		}
		result.push_back(byte);
	}
	return result;
}


} // namespace Util




#if MPT_OS_WINDOWS

namespace mpt
{
namespace Windows
{
namespace Version
{


#if defined(MODPLUG_TRACKER)

	
static bool SystemIsNT = true;

// Initialize to used SDK version
static uint32 SystemVersion =
#if NTDDI_VERSION >= 0x06030000 // NTDDI_WINBLUE
	mpt::Windows::Version::Win81
#elif NTDDI_VERSION >= 0x06020000 // NTDDI_WIN8
	mpt::Windows::Version::Win8
#elif NTDDI_VERSION >= 0x06010000 // NTDDI_WIN7
	mpt::Windows::Version::Win7
#elif NTDDI_VERSION >= 0x06000000 // NTDDI_VISTA
	mpt::Windows::Version::WinVista
#elif NTDDI_VERSION >= NTDDI_WINXP
	mpt::Windows::Version::WinXP
#elif NTDDI_VERSION >= NTDDI_WIN2K
	mpt::Windows::Version::Win2000
#else
	mpt::Windows::Version::WinNT4
#endif
	;

static bool SystemIsWine = false;


void Init()
//---------
{
	OSVERSIONINFOEXW versioninfoex;
	MemsetZero(versioninfoex);
	versioninfoex.dwOSVersionInfoSize = sizeof(versioninfoex);
	GetVersionExW((LPOSVERSIONINFOW)&versioninfoex);
	SystemIsNT = (versioninfoex.dwPlatformId == VER_PLATFORM_WIN32_NT);
	SystemVersion = ((uint32)mpt::saturate_cast<uint8>(versioninfoex.dwMajorVersion) << 8) | ((uint32)mpt::saturate_cast<uint8>(versioninfoex.dwMinorVersion) << 0);
	SystemIsWine = false;
	HMODULE hNTDLL = LoadLibraryW(L"ntdll.dll");
	if(hNTDLL)
	{
		SystemIsWine = (GetProcAddress(hNTDLL, "wine_get_version") != NULL);
		FreeLibrary(hNTDLL);
		hNTDLL = NULL;
	}
}


bool IsAtLeast(mpt::Windows::Version::Number version)
//---------------------------------------------------
{
	return (SystemVersion >= (uint32)version);
}


bool IsNT()
//---------
{
	return SystemIsNT;
}


bool IsWine()
//-----------
{
	return SystemIsWine;
}


#else // !MODPLUG_TRACKER


bool IsAtLeast(mpt::Windows::Version::Number version)
//---------------------------------------------------
{
	OSVERSIONINFOEXW versioninfoex;
	MemsetZero(versioninfoex);
	versioninfoex.dwOSVersionInfoSize = sizeof(versioninfoex);
	GetVersionExW((LPOSVERSIONINFOW)&versioninfoex);
	uint32 SystemVersion = ((uint32)mpt::saturate_cast<uint8>(versioninfoex.dwMajorVersion) << 8) | ((uint32)mpt::saturate_cast<uint8>(versioninfoex.dwMinorVersion) << 0);
	return (SystemVersion >= (uint32)version);
}


#endif // MODPLUG_TRACKER


} // namespace Version
} // namespace Windows
} // namespace mpt

#endif // MPT_OS_WINDOWS


#if defined(MPT_WITH_DYNBIND)


namespace mpt
{


#if MPT_OS_WINDOWS


// KB2533623 / Win8
#ifndef LOAD_LIBRARY_SEARCH_DEFAULT_DIRS
#define LOAD_LIBRARY_SEARCH_DEFAULT_DIRS    0x00001000
#endif
#ifndef LOAD_LIBRARY_SEARCH_APPLICATION_DIR
#define LOAD_LIBRARY_SEARCH_APPLICATION_DIR 0x00000200
#endif
#ifndef LOAD_LIBRARY_SEARCH_SYSTEM32
#define LOAD_LIBRARY_SEARCH_SYSTEM32        0x00000800
#endif
#ifndef LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR
#define LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR    0x00000100
#endif


mpt::PathString GetAppPath()
{
#if defined(MODPLUG_TRACKER)
	WCHAR exeFileName[MAX_PATH+1];
	MemsetZero(exeFileName);
	if(!GetModuleFileNameW(NULL, exeFileName, MAX_PATH))
	{
		return mpt::PathString();
	}
	return mpt::GetAbsolutePath(mpt::PathString::FromNative(exeFileName).GetDrive() + mpt::PathString::FromNative(exeFileName).GetDir());
#else
	return mpt::PathString(); // dummy
#endif
}


mpt::PathString GetSystemPath()
{
	WCHAR path[MAX_PATH+1];
	MemsetZero(path);
	if(!GetSystemDirectoryW(path, MAX_PATH))
	{
		return mpt::PathString();
	}
	return mpt::PathString::FromNative(path) + MPT_PATHSTRING("\\");
}


mpt::PathString GetAbsolutePath(const mpt::PathString &path)
{
	WCHAR fullPathName[MAX_PATH+1];
	MemsetZero(fullPathName);
	if(!GetFullPathNameW(path.AsNative().c_str(), MAX_PATH, fullPathName, NULL))
	{
		return path;
	}
	return mpt::PathString::FromNative(fullPathName);
}
		

class LibraryHandle
{

private:

	HMODULE hModule;

public:

	LibraryHandle(const mpt::LibraryPath &path)
		: hModule(NULL)
	{

		// Check for KB2533623:
		bool hasKB2533623 = false;
		if(mpt::Windows::Version::IsAtLeast(mpt::Windows::Version::Win8))
		{
			hasKB2533623 = true;
		} else if(mpt::Windows::Version::IsAtLeast(mpt::Windows::Version::WinVista))
		{
			HMODULE hKernel32DLL = LoadLibraryW(L"kernel32.dll");
			if(hKernel32DLL)
			{
				if(::GetProcAddress(hKernel32DLL, "SetDefaultDllDirectories") != nullptr)
				{
					hasKB2533623 = true;
				}
				FreeLibrary(hKernel32DLL);
				hKernel32DLL = NULL;
			}
		}

		if(hasKB2533623)
		{
			switch(path.GetSearchPath())
			{
				case mpt::LibrarySearchPathDefault:
					hModule = LoadLibraryExW(path.GetFileName().AsNative().c_str(), NULL, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
					break;
				case mpt::LibrarySearchPathSystem:
					hModule = LoadLibraryExW(path.GetFileName().AsNative().c_str(), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
					break;
#if 0
					// Using restricted search paths applies to potential DLL dependencies
					// recursively. 
					// This fails loading for e.g. Codec or Plugin DLLs in application
					// directory if they depend on the MSVC C or C++ runtime (which is
					// located in the system directory).
					// Just rely on the default search path here.
				case mpt::LibrarySearchPathApplication:
					hModule = LoadLibraryExW(path.GetFileName().AsNative().c_str(), NULL, LOAD_LIBRARY_SEARCH_APPLICATION_DIR);
					break;
				case mpt::LibrarySearchPathFullPath:
					hModule = LoadLibraryExW(path.GetFileName().AsNative().c_str(), NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR);
					break;
#else
				case mpt::LibrarySearchPathApplication:
					hModule = LoadLibraryW((mpt::GetAppPath() + path.GetFileName()).AsNative().c_str());
					break;
				case mpt::LibrarySearchPathFullPath:
					hModule = LoadLibraryW(path.GetFileName().AsNative().c_str());
					break;
#endif
			}
		} else
		{
			switch(path.GetSearchPath())
			{
				case mpt::LibrarySearchPathDefault:
					hModule = LoadLibraryW(path.GetFileName().AsNative().c_str());
					break;
				case mpt::LibrarySearchPathApplication:
					hModule = LoadLibraryW((mpt::GetAppPath() + path.GetFileName()).AsNative().c_str());
					break;
				case mpt::LibrarySearchPathSystem:
					hModule = LoadLibraryW((mpt::GetSystemPath() + path.GetFileName()).AsNative().c_str());
					break;
				case mpt::LibrarySearchPathFullPath:
					hModule = LoadLibraryW(path.GetFileName().AsNative().c_str());
					break;
			}
		}
	}

	~LibraryHandle()
	{
		if(IsValid())
		{
			FreeLibrary(hModule);
		}
		hModule = NULL;
	}

public:

	bool IsValid() const
	{
		return (hModule != NULL);
	}

	FuncPtr GetProcAddress(const std::string &symbol) const
	{
		if(!IsValid())
		{
			return nullptr;
		}
		return reinterpret_cast<FuncPtr>(::GetProcAddress(hModule, symbol.c_str()));
	}

};


#else


class LibraryHandle
{

private:

	void * handle;

public:

	LibraryHandle(const mpt::LibraryPath &path)
		: handle(NULL)
	{
		handle = dlopen(path.GetFileName().AsNative().c_str(), RTLD_NOW);
	}

	~LibraryHandle()
	{
		if(IsValid())
		{
			dlclose(handle);
		}
		handle = NULL;
	}
	
public:

	bool IsValid() const
	{
		return handle != NULL;
	}

	FuncPtr GetProcAddress(const std::string &symbol) const
	{
		if(!IsValid())
		{
			return NULL;
		}
		return reinterpret_cast<FuncPtr>(dlsym(handle, symbol.c_str()));
	}

};


#endif


LibraryPath::LibraryPath(mpt::LibrarySearchPath searchPath, class mpt::PathString const &fileName)
//------------------------------------------------------------------------------------------------
	: searchPath(searchPath)
	, fileName(fileName)
{
	return;
}


mpt::LibrarySearchPath LibraryPath::GetSearchPath() const
//-------------------------------------------------------
{
	return searchPath;
}


mpt::PathString LibraryPath::GetFileName() const
//----------------------------------------------
{
	return fileName;
}


mpt::PathString LibraryPath::GetDefaultPrefix()
//---------------------------------------------
{
#if MPT_OS_WINDOWS
	return MPT_PATHSTRING("");
#elif MPT_OS_MACOSX_OR_IOS
	return MPT_PATHSTRING("lib");
#else
	return MPT_PATHSTRING("lib");
#endif
}


mpt::PathString LibraryPath::GetDefaultSuffix()
//---------------------------------------------
{
#if MPT_OS_WINDOWS
	return MPT_PATHSTRING(".dll");
#elif MPT_OS_MACOSX_OR_IOS
	return MPT_PATHSTRING(".dylib");
#else
	return MPT_PATHSTRING(".so");
#endif
}


LibraryPath LibraryPath::App(const mpt::PathString &basename)
//-----------------------------------------------------------
{
	return LibraryPath(mpt::LibrarySearchPathApplication, GetDefaultPrefix() + basename + GetDefaultSuffix());
}


LibraryPath LibraryPath::AppFullName(const mpt::PathString &fullname)
//-------------------------------------------------------------------
{
	return LibraryPath(mpt::LibrarySearchPathApplication, fullname + GetDefaultSuffix());
}


LibraryPath LibraryPath::System(const mpt::PathString &basename)
//--------------------------------------------------------------
{
	return LibraryPath(mpt::LibrarySearchPathSystem, GetDefaultPrefix() + basename + GetDefaultSuffix());
}


LibraryPath LibraryPath::FullPath(const mpt::PathString &path)
//------------------------------------------------------------
{
	return LibraryPath(mpt::LibrarySearchPathFullPath, path);
}


Library::Library()
//----------------
{
	return;
}


Library::Library(const mpt::LibraryPath &path)
//--------------------------------------------
{
	m_Handle = MPT_SHARED_PTR<LibraryHandle>(new LibraryHandle(path));
}


void Library::Unload()
//--------------------
{
	*this = mpt::Library();
}


bool Library::IsValid() const
//---------------------------
{
	return m_Handle && m_Handle->IsValid();
}


FuncPtr Library::GetProcAddress(const std::string &symbol) const
//--------------------------------------------------------------
{
	if(!IsValid())
	{
		return nullptr;
	}
	return m_Handle->GetProcAddress(symbol);
}


} // namespace mpt


#endif // MPT_WITH_DYNBIND


OPENMPT_NAMESPACE_END
