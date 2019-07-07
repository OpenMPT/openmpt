/*
 * mptOSException.h
 * ----------------
 * Purpose: platform-specific exception/signal handling
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

#include "mptBaseMacros.h"
#include "mptBaseTypes.h"

#if defined(MODPLUG_TRACKER)
#if MPT_OS_WINDOWS
#include <windows.h>
#endif // MPT_OS_WINDOWS
#endif // MODPLUG_TRACKER


OPENMPT_NAMESPACE_BEGIN


#if defined(MODPLUG_TRACKER)


#if MPT_OS_WINDOWS


namespace Windows
{


struct StructuredException
{
private:
	DWORD m_Code;
public:
	constexpr StructuredException(DWORD code) noexcept
		: m_Code(code)
	{
		return;
	}
public:
	constexpr DWORD code() const noexcept
	{
		return m_Code;
	}
};


template <typename Tfn>
auto ThrowOnStructuredException(Tfn fn) -> decltype(fn())
{
	DWORD code = 0;
	__try
	{
		return fn();
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
		code = GetExceptionCode();
	}
	throw StructuredException(code);
}


}  // namespace Windows


#endif // MPT_OS_WINDOWS


#endif // MODPLUG_TRACKER


OPENMPT_NAMESPACE_END
