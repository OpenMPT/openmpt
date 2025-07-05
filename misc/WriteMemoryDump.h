/*
 * WriteMemoryDump.h
 * -----------------
 * Purpose: Code for writing memory dumps to a file.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "mpt/base/detect.hpp"
#include "mpt/base/macros.hpp"

#if MPT_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable:4091) // 'typedef ': ignored on left of '' when no variable is declared
#endif // MPT_COMPILER_MSVC
#include <dbghelp.h>
#if MPT_COMPILER_MSVC
#pragma warning(pop)
#endif // MPT_COMPILER_MSVC

OPENMPT_NAMESPACE_BEGIN

MPT_ATTR_NOINLINE MPT_DECL_NOINLINE inline bool WriteMemoryDump(_EXCEPTION_POINTERS *pExceptionInfo, const TCHAR *filename, bool fullMemDump)
{
	bool result = false;
	HMODULE hDll = ::LoadLibrary(_T("DBGHELP.DLL"));
	if(hDll)
	{
		using MINIDUMPWRITEDUMP = BOOL(WINAPI *)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType, CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);
#if (MPT_CLANG_AT_LEAST(19, 0, 0) && !MPT_OS_ANDROID) || MPT_CLANG_AT_LEAST(20, 0, 0)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-function-type-mismatch"
#endif
		MINIDUMPWRITEDUMP pDump = reinterpret_cast<MINIDUMPWRITEDUMP>(::GetProcAddress(hDll, "MiniDumpWriteDump"));
#if (MPT_CLANG_AT_LEAST(19, 0, 0) && !MPT_OS_ANDROID) || MPT_CLANG_AT_LEAST(20, 0, 0)
#pragma clang diagnostic pop
#endif
		if(pDump)
		{
			HANDLE hFile = ::CreateFile(filename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if((hFile != INVALID_HANDLE_VALUE) && (hFile != NULL))
			{
				const MINIDUMP_TYPE flags = fullMemDump ?
						(MINIDUMP_TYPE)(MiniDumpWithFullMemory | MiniDumpWithHandleData | MiniDumpWithThreadInfo | MiniDumpWithProcessThreadData | MiniDumpWithFullMemoryInfo | MiniDumpIgnoreInaccessibleMemory | MiniDumpWithTokenInformation)
					:
						MiniDumpNormal
					;
				if(pExceptionInfo)
				{
					_MINIDUMP_EXCEPTION_INFORMATION ExInfo{};
					ExInfo.ThreadId = ::GetCurrentThreadId();
					ExInfo.ExceptionPointers = pExceptionInfo;
					ExInfo.ClientPointers = NULL;
					result = (pDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, flags, &ExInfo, NULL, NULL) == TRUE);
				} else
				{
					result = (pDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, flags, NULL, NULL, NULL) == TRUE);
				}
				::CloseHandle(hFile);
			}
		}
		::FreeLibrary(hDll);
	}
	return result;
}

OPENMPT_NAMESPACE_END
