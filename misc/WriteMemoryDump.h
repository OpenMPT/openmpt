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

#include "mpt/base/utility.hpp"

#if MPT_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable:4091) // 'typedef ': ignored on left of '' when no variable is declared
#endif // MPT_COMPILER_MSVC
#include <dbghelp.h>
#if MPT_COMPILER_MSVC
#pragma warning(pop)
#endif // MPT_COMPILER_MSVC

OPENMPT_NAMESPACE_BEGIN

inline bool WriteMemoryDump(_EXCEPTION_POINTERS *pExceptionInfo, const TCHAR *filename, bool fullMemDump)
{
	using MINIDUMPWRITEDUMP = BOOL(WINAPI *)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType, CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

	bool result = false;

	HMODULE hDll = ::LoadLibrary(_T("DBGHELP.DLL"));
	if(hDll)
	{
		MINIDUMPWRITEDUMP pDump = mpt::function_pointer_cast<MINIDUMPWRITEDUMP>(::GetProcAddress(hDll, "MiniDumpWriteDump"));
		if(pDump)
		{

			HANDLE hFile = ::CreateFile(filename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile != INVALID_HANDLE_VALUE)
			{
				_MINIDUMP_EXCEPTION_INFORMATION ExInfo;

				if(pExceptionInfo)
				{
					ExInfo.ThreadId = ::GetCurrentThreadId();
					ExInfo.ExceptionPointers = pExceptionInfo;
					ExInfo.ClientPointers = NULL;
				}

				BOOL DumpResult = pDump(GetCurrentProcess(), GetCurrentProcessId(), hFile,
					fullMemDump ?
					(MINIDUMP_TYPE)(MiniDumpWithFullMemory | MiniDumpWithHandleData | MiniDumpWithThreadInfo | MiniDumpWithProcessThreadData | MiniDumpWithFullMemoryInfo
#if MPT_COMPILER_MSVC
					| MiniDumpIgnoreInaccessibleMemory | MiniDumpWithTokenInformation
#endif
					)
					:
				MiniDumpNormal,
					pExceptionInfo ? &ExInfo : NULL, NULL, NULL);
				::CloseHandle(hFile);

				result = (DumpResult == TRUE);
			}
		}
		::FreeLibrary(hDll);
	}
	return result;
}

OPENMPT_NAMESPACE_END
