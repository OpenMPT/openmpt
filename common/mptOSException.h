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


namespace SEH
{


struct Code
{
private:
	DWORD m_Code;
public:
	constexpr Code(DWORD code) noexcept
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


template <typename Tfn, typename Tfilter, typename Thandler>
auto TryFilterHandle(Tfn fn, const Tfilter &filter, const Thandler &handler) -> decltype(fn())
{
	DWORD code = 0;
	__try
	{
		return fn();
	} __except(filter(GetExceptionCode(), GetExceptionInformation()))
	{
		code = GetExceptionCode();
		handler(code);
	}
	throw Windows::SEH::Code(code);
}


template <typename Tfn, typename Tfilter, typename Thandler>
auto TryFilterHandleDefault(Tfn fn, const Tfilter &filter, const Thandler &handler, decltype(fn()) def = decltype(fn()){}) -> decltype(fn())
{
	auto result = def;
	DWORD code = 0;
	__try
	{
		result = fn();
	} __except(filter(GetExceptionCode(), GetExceptionInformation()))
	{
		code = GetExceptionCode();
		result = handler(code);
	}
	return result;
}


template <typename Tfn>
auto TryOrThrow(Tfn fn) -> decltype(fn())
{
	return TryFilterHandle(
		fn,
		[](auto code, auto eptr)
		{
			MPT_UNREFERENCED_PARAMETER(code);
			MPT_UNREFERENCED_PARAMETER(eptr);
			return EXCEPTION_EXECUTE_HANDLER;
		},
		[](auto code)
		{
			throw Windows::SEH::Code(code);
		});
}


template <typename Tcode, typename Tfn>
auto TryOrCapture(Tcode & dstCode, Tfn fn) -> decltype(fn())
{
	dstCode = DWORD{0};
	return TryFilterHandle(
		fn,
		[](auto code, auto eptr)
		{
			MPT_UNREFERENCED_PARAMETER(code);
			MPT_UNREFERENCED_PARAMETER(eptr);
			return EXCEPTION_EXECUTE_HANDLER;
		},
		[](auto code)
		{
			dstCode = code;
		});
}


template <typename Tfn>
auto TryOrDefault(Tfn fn, decltype(fn()) def = decltype(fn()){}) -> decltype(fn())
{
	return TryFilterHandleReturn(fn,
		[](auto code, auto eptr)
		{
			MPT_UNREFERENCED_PARAMETER(code);
			MPT_UNREFERENCED_PARAMETER(eptr);
			return EXCEPTION_EXECUTE_HANDLER;
		},
		[def](auto code)
		{
			MPT_UNREFERENCED_PARAMETER(code);
			return def;
		});
}


} // namspace SEH


}  // namespace Windows


#endif // MPT_OS_WINDOWS


#endif // MODPLUG_TRACKER


OPENMPT_NAMESPACE_END
