#include "stdafx.h"

#ifdef _WIN32
namespace file_win32_helpers {
	t_filesize get_size(HANDLE p_handle) {
		union {
			t_uint64 val64;
			t_uint32 val32[2];
		} u;

		u.val64 = 0;
		SetLastError(NO_ERROR);
		u.val32[0] = GetFileSize(p_handle,reinterpret_cast<DWORD*>(&u.val32[1]));
		if (GetLastError()!=NO_ERROR) exception_io_from_win32(GetLastError());
		return u.val64;
	}
	void seek(HANDLE p_handle,t_sfilesize p_position,file::t_seek_mode p_mode) {
		union  {
			t_int64 temp64;
			struct {
				DWORD temp_lo;
				LONG temp_hi;
			};
		};

		temp64 = p_position;
		SetLastError(ERROR_SUCCESS);		
		temp_lo = SetFilePointer(p_handle,temp_lo,&temp_hi,(DWORD)p_mode);
		if (GetLastError() != ERROR_SUCCESS) exception_io_from_win32(GetLastError());
	}

	void fillOverlapped(OVERLAPPED & ol, HANDLE myEvent, t_filesize s) {
		ol.hEvent = myEvent;
		ol.Offset = (DWORD)( s & 0xFFFFFFFF );
		ol.OffsetHigh = (DWORD)(s >> 32);
	}

	void writeOverlappedPass(HANDLE handle, HANDLE myEvent, t_filesize position, const void * in,DWORD inBytes, abort_callback & abort) {
		abort.check();
		if (inBytes == 0) return;
		OVERLAPPED ol = {};
		fillOverlapped(ol, myEvent, position);
		ResetEvent(myEvent);
		DWORD bytesWritten;
		SetLastError(NO_ERROR);
		if (WriteFile( handle, in, inBytes, &bytesWritten, &ol)) {
			// succeeded already?
			if (bytesWritten != inBytes) throw exception_io();
			return;
		}
		
		{
			const DWORD code = GetLastError();
			if (code != ERROR_IO_PENDING) exception_io_from_win32(code);
		}
		const HANDLE handles[] = {myEvent, abort.get_abort_event()};
		SetLastError(NO_ERROR);
		DWORD state = WaitForMultipleObjects(_countof(handles), handles, FALSE, INFINITE);
		if (state == WAIT_OBJECT_0) {
			try {
				WIN32_IO_OP( GetOverlappedResult(handle,&ol,&bytesWritten,TRUE) );
			} catch(...) {
				CancelIo(handle);
				throw;
			}
			if (bytesWritten != inBytes) throw exception_io();
			return;
		}
		CancelIo(handle);
		throw exception_aborted();
	}

	void writeOverlapped(HANDLE handle, HANDLE myEvent, t_filesize & position, const void * in, size_t inBytes, abort_callback & abort) {
		const enum {writeMAX = 16*1024*1024};
		size_t done = 0;
		while(done < inBytes) {
			size_t delta = inBytes - done;
			if (delta > writeMAX) delta = writeMAX;
			writeOverlappedPass(handle, myEvent, position, (const BYTE*)in + done, (DWORD) delta, abort);
			done += delta;
			position += delta;
		}
	}
	void writeStreamOverlapped(HANDLE handle, HANDLE myEvent, const void * in, size_t inBytes, abort_callback & abort) {
		const enum {writeMAX = 16*1024*1024};
		size_t done = 0;
		while(done < inBytes) {
			size_t delta = inBytes - done;
			if (delta > writeMAX) delta = writeMAX;
			writeOverlappedPass(handle, myEvent, 0, (const BYTE*)in + done, (DWORD) delta, abort);
			done += delta;
		}
	}

	DWORD readOverlappedPass(HANDLE handle, HANDLE myEvent, t_filesize position, void * out, DWORD outBytes, abort_callback & abort) {
		abort.check();
		if (outBytes == 0) return 0;
		OVERLAPPED ol = {};
		fillOverlapped(ol, myEvent, position);
		ResetEvent(myEvent);
		DWORD bytesDone;
		SetLastError(NO_ERROR);
		if (ReadFile( handle, out, outBytes, &bytesDone, &ol)) {
			// succeeded already?
			return bytesDone;
		}

		{
			const DWORD code = GetLastError();
			switch(code) {
			case ERROR_HANDLE_EOF:
			case ERROR_BROKEN_PIPE:
				return 0;
			case ERROR_IO_PENDING:
				break; // continue
			default:
				exception_io_from_win32(code);
			};
		}

		const HANDLE handles[] = {myEvent, abort.get_abort_event()};
		SetLastError(NO_ERROR);
		DWORD state = WaitForMultipleObjects(_countof(handles), handles, FALSE, INFINITE);
		if (state == WAIT_OBJECT_0) {
			SetLastError(NO_ERROR);
			if (!GetOverlappedResult(handle,&ol,&bytesDone,TRUE)) {
				const DWORD code = GetLastError();
				if (code == ERROR_HANDLE_EOF || code == ERROR_BROKEN_PIPE) bytesDone = 0;
				else {
					CancelIo(handle);
					exception_io_from_win32(code);
				}
			}
			return bytesDone;
		}
		CancelIo(handle);
		throw exception_aborted();
	}
	size_t readOverlapped(HANDLE handle, HANDLE myEvent, t_filesize & position, void * out, size_t outBytes, abort_callback & abort) {
		const enum {readMAX = 16*1024*1024};
		size_t done = 0;
		while(done < outBytes) {
			size_t delta = outBytes - done;
			if (delta > readMAX) delta = readMAX;
			delta = readOverlappedPass(handle, myEvent, position, (BYTE*) out + done, (DWORD) delta, abort);
			if (delta == 0) break;
			done += delta;
			position += delta;
		}
		return done;
	}

	size_t readStreamOverlapped(HANDLE handle, HANDLE myEvent, void * out, size_t outBytes, abort_callback & abort) {
		const enum {readMAX = 16*1024*1024};
		size_t done = 0;
		while(done < outBytes) {
			size_t delta = outBytes - done;
			if (delta > readMAX) delta = readMAX;
			delta = readOverlappedPass(handle, myEvent, 0, (BYTE*) out + done, (DWORD) delta, abort);
			if (delta == 0) break;
			done += delta;
		}
		return done;
	}

	typedef BOOL (WINAPI * pCancelSynchronousIo_t)(HANDLE hThread);


	struct createFileData_t {
		LPCTSTR lpFileName;
		DWORD dwDesiredAccess;
		DWORD dwShareMode;
		LPSECURITY_ATTRIBUTES lpSecurityAttributes;
		DWORD dwCreationDisposition;
		DWORD dwFlagsAndAttributes;
		HANDLE hTemplateFile;
		HANDLE hResult;
		DWORD dwErrorCode;
	};

	static unsigned CALLBACK createFileProc(void * data) {
		createFileData_t * cfd = (createFileData_t*)data;
		SetLastError(0);
		cfd->hResult = CreateFile(cfd->lpFileName, cfd->dwDesiredAccess, cfd->dwShareMode, cfd->lpSecurityAttributes, cfd->dwCreationDisposition, cfd->dwFlagsAndAttributes, cfd->hTemplateFile);
		cfd->dwErrorCode = GetLastError();
		return 0;
	}

	HANDLE createFile(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile, abort_callback & abort) {
		abort.check();
		
		return CreateFile(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

		// CancelSynchronousIo() doesn't fucking work. Useless.
#if 0
		pCancelSynchronousIo_t pCancelSynchronousIo = (pCancelSynchronousIo_t) GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "CancelSynchronousIo");
		if (pCancelSynchronousIo == NULL) {
#ifdef _DEBUG
			uDebugLog() << "Async CreateFile unavailable - using regular";
#endif
			return CreateFile(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
		} else {
#ifdef _DEBUG
			uDebugLog() << "Starting async CreateFile...";
			pfc::hires_timer t; t.start();
#endif
			createFileData_t data = {lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile, NULL, 0};
			HANDLE hThread = (HANDLE) _beginthreadex(NULL, 0, createFileProc, &data, 0, NULL);
			HANDLE waitHandles[2] = {hThread, abort.get_abort_event()};
			switch(WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE)) {
			case WAIT_OBJECT_0: // succeeded
				break;
			case WAIT_OBJECT_0 + 1: // abort
#ifdef _DEBUG
				uDebugLog() << "Aborting async CreateFile...";
#endif
				pCancelSynchronousIo(hThread);
				WaitForSingleObject(hThread, INFINITE);
				break;
			default:
				uBugCheck();
			}
			CloseHandle(hThread);
			SetLastError(data.dwErrorCode);
#ifdef _DEBUG
			uDebugLog() << "Async CreateFile completed in " << pfc::format_time_ex(t.query(), 6) << ", status: " << (uint32_t) data.dwErrorCode;
#endif
			if (abort.is_aborting()) {
				if (data.hResult != INVALID_HANDLE_VALUE) CloseHandle(data.hResult);
				throw exception_aborted();
			}
			return data.hResult;
		}
#endif
	}

}

#endif // _WIN32

