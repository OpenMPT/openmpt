#ifdef _WIN32

namespace ProcessUtils {
	class PipeIO : public stream_reader, public stream_writer {
	public:
		PFC_DECLARE_EXCEPTION(timeout, exception_io, "Timeout");
		PipeIO(HANDLE handle, HANDLE hEvent, bool processMessages, DWORD timeOut = INFINITE) : m_handle(handle), m_event(hEvent), m_processMessages(processMessages), m_timeOut(timeOut) {
		}
		~PipeIO() {
		}

		void write(const void * p_buffer,size_t p_bytes, abort_callback & abort) {
			if (p_bytes == 0) return;
			OVERLAPPED ol = {};
			ol.hEvent = m_event;
			ResetEvent(m_event);
			DWORD bytesWritten;
			SetLastError(NO_ERROR);
			if (WriteFile( m_handle, p_buffer, pfc::downcast_guarded<DWORD>(p_bytes), &bytesWritten, &ol)) {
				// succeeded already?
				if (bytesWritten != p_bytes) throw exception_io();
				return;
			}
		
			{
				const DWORD code = GetLastError();
				if (code != ERROR_IO_PENDING) exception_io_from_win32(code);
			}
			const HANDLE handles[] = {m_event, abort.get_abort_event()};
			SetLastError(NO_ERROR);
			DWORD state = myWait(_countof(handles), handles);
			if (state == WAIT_OBJECT_0) {
				try {
					WIN32_IO_OP( GetOverlappedResult(m_handle,&ol,&bytesWritten,TRUE) );
				} catch(...) {
					_cancel(ol);
					throw;
				}
				if (bytesWritten != p_bytes) throw exception_io();
				return;
			}
			_cancel(ol);
			abort.check();
			throw timeout();
		}
		size_t read(void * p_buffer,size_t p_bytes, abort_callback & abort) {
			uint8_t * ptr = (uint8_t*) p_buffer;
			size_t done = 0;
			while(done < p_bytes) {
				abort.check();
				size_t delta = readPass(ptr + done, p_bytes - done, abort);
				if (delta == 0) break;
				done += delta;
			}
			return done;
		}
		size_t readPass(void * p_buffer,size_t p_bytes, abort_callback & abort) {
			if (p_bytes == 0) return 0;
			OVERLAPPED ol = {};
			ol.hEvent = m_event;
			ResetEvent(m_event);
			DWORD bytesDone;
			SetLastError(NO_ERROR);
			if (ReadFile( m_handle, p_buffer, pfc::downcast_guarded<DWORD>(p_bytes), &bytesDone, &ol)) {
				// succeeded already?
				return bytesDone;
			}

			{
				const DWORD code = GetLastError();
				switch(code) {
				case ERROR_HANDLE_EOF:
					return 0;
				case ERROR_IO_PENDING:
					break; // continue
				default:
					exception_io_from_win32(code);
				};
			}

			const HANDLE handles[] = {m_event, abort.get_abort_event()};
			SetLastError(NO_ERROR);
			DWORD state = myWait(_countof(handles), handles);
			if (state == WAIT_OBJECT_0) {
				SetLastError(NO_ERROR);
				if (!GetOverlappedResult(m_handle,&ol,&bytesDone,TRUE)) {
					const DWORD code = GetLastError();
					if (code == ERROR_HANDLE_EOF) bytesDone = 0;
					else {
						_cancel(ol);
						exception_io_from_win32(code);
					}
				}
				return bytesDone;
			}
			_cancel(ol);
			abort.check();
			throw timeout();
		}
	private:
		DWORD myWait(DWORD count, const HANDLE * handles) {
			if (m_processMessages) {
				for(;;) {
					DWORD state = MsgWaitForMultipleObjects(count, handles, FALSE, m_timeOut, QS_ALLINPUT);
					if (state == WAIT_OBJECT_0 + count) {
						ProcessPendingMessages();
					} else {
						return state;
					}
				}
			} else {
				return WaitForMultipleObjects(count, handles, FALSE, m_timeOut);
			}
		}
		void _cancel(OVERLAPPED & ol) {
	#if _WIN32_WINNT >= 0x600
			CancelIoEx(m_handle,&ol);
	#else
			CancelIo(m_handle);
	#endif
		}
		static void ProcessPendingMessages() {
			MSG msg = {};
			while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessage(&msg);
		}

		HANDLE m_handle;
		HANDLE m_event;
		const DWORD m_timeOut;
		const bool m_processMessages;
	};

	class SubProcess : public stream_reader, public stream_writer {
	public:
		PFC_DECLARE_EXCEPTION(failure, std::exception, "Unexpected failure");

		SubProcess(const char * exePath, DWORD timeOutMS = 60*1000) : ExePath(exePath), hStdIn(), hStdOut(), hProcess(), ProcessMessages(false), TimeOutMS(timeOutMS) {
			HANDLE ev;
			WIN32_OP( (ev = CreateEvent(NULL, TRUE, FALSE, NULL)) != NULL );
			hEventRead = ev;
			WIN32_OP( (ev = CreateEvent(NULL, TRUE, FALSE, NULL)) != NULL );
			hEventWrite = ev;
			Restart();
		}
		void Restart() {
			CleanUp();
			STARTUPINFO si = {};
			try {
				si.cb = sizeof(si);
				si.dwFlags = STARTF_USESTDHANDLES | STARTF_FORCEOFFFEEDBACK;
				//si.wShowWindow = SW_HIDE;

				myCreatePipeOut(si.hStdInput, hStdIn);
				myCreatePipeIn(hStdOut, si.hStdOutput);
				si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

				PROCESS_INFORMATION pi = {};
				try {
					WIN32_OP( CreateProcess(pfc::stringcvt::string_os_from_utf8(ExePath), NULL, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi) );
				} catch(std::exception const & e) {
					throw failure(pfc::string_formatter() << "Could not start the worker process - " << e);
				}
				hProcess = pi.hProcess; _Close(pi.hThread);
			} catch(...) {
				_Close(si.hStdInput);
				_Close(si.hStdOutput);
				CleanUp(); throw;
			}
			_Close(si.hStdInput);
			_Close(si.hStdOutput);
		}
		~SubProcess() {
			CleanUp();
			CloseHandle(hEventRead);
			CloseHandle(hEventWrite);
		}

		bool IsRunning() const {
			return hProcess != NULL;
		}
		void Detach() {
			CleanUp(true);
		}
		bool ProcessMessages;
		DWORD TimeOutMS;


		void write(const void * p_buffer,size_t p_bytes, abort_callback & abort) {
			PipeIO writer(hStdIn, hEventWrite, ProcessMessages, TimeOutMS);
			writer.write(p_buffer, p_bytes, abort);
		}
		size_t read(void * p_buffer,size_t p_bytes, abort_callback & abort) {
			PipeIO reader(hStdOut, hEventRead, ProcessMessages, TimeOutMS);
			return reader.read(p_buffer, p_bytes, abort);
		}
		void SetPriority(DWORD val) {
			SetPriorityClass(hProcess, val);
		}
	protected:
		HANDLE hStdIn, hStdOut, hProcess, hEventRead, hEventWrite;
		const pfc::string8 ExePath;
	
		void CleanUp(bool bDetach = false) {
			_Close(hStdIn); _Close(hStdOut);
			if (hProcess != NULL) {
				if (!bDetach) {
					if (WaitForSingleObject(hProcess, TimeOutMS) != WAIT_OBJECT_0) {
						//PFC_ASSERT( !"Should not get here - worker stuck" );
						console::formatter() << pfc::string_filename_ext(ExePath) << " unresponsive - terminating";
						TerminateProcess(hProcess, -1);
					}
				}
				_Close(hProcess);
			}
		}
	private:
		static void _Close(HANDLE & h) {
			if (h != NULL) {CloseHandle(h); h = NULL;}
		}

		static void myCreatePipe(HANDLE & in, HANDLE & out) {
			SECURITY_ATTRIBUTES Attributes = { sizeof(SECURITY_ATTRIBUTES), 0, true };
			WIN32_OP( CreatePipe( &in, &out, &Attributes, 0 ) );
		}

		static pfc::string_formatter makePipeName() {
			GUID id;
			CoCreateGuid (&id);
			return pfc::string_formatter() << "\\\\.\\pipe\\" << pfc::print_guid(id);
		}

		static void myCreatePipeOut(HANDLE & in, HANDLE & out) {
			SECURITY_ATTRIBUTES Attributes = { sizeof(SECURITY_ATTRIBUTES), 0, true };
			const pfc::stringcvt::string_os_from_utf8 pipeName( makePipeName() );
			SetLastError(NO_ERROR);
			HANDLE pipe = CreateNamedPipe(
				pipeName,
				FILE_FLAG_FIRST_PIPE_INSTANCE | PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED,
				PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
				1,
				1024*64,
				1024*64,
				NMPWAIT_USE_DEFAULT_WAIT,&Attributes);
			if (pipe == INVALID_HANDLE_VALUE) throw exception_win32(GetLastError());
			
			in = CreateFile(pipeName,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,&Attributes,OPEN_EXISTING,0,NULL);
			DuplicateHandle ( GetCurrentProcess(), pipe, GetCurrentProcess(), &out, 0, FALSE, DUPLICATE_SAME_ACCESS );
			CloseHandle(pipe);
		}

		static void myCreatePipeIn(HANDLE & in, HANDLE & out) {
			SECURITY_ATTRIBUTES Attributes = { sizeof(SECURITY_ATTRIBUTES), 0, true };
			const pfc::stringcvt::string_os_from_utf8 pipeName( makePipeName() );
			SetLastError(NO_ERROR);
			HANDLE pipe = CreateNamedPipe(
				pipeName,
				FILE_FLAG_FIRST_PIPE_INSTANCE | PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
				PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
				1,
				1024*64,
				1024*64,
				NMPWAIT_USE_DEFAULT_WAIT,&Attributes);
			if (pipe == INVALID_HANDLE_VALUE) throw exception_win32(GetLastError());
			
			out = CreateFile(pipeName,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&Attributes,OPEN_EXISTING,0,NULL);
			DuplicateHandle ( GetCurrentProcess(), pipe, GetCurrentProcess(), &in, 0, FALSE, DUPLICATE_SAME_ACCESS );
			CloseHandle(pipe);
		}

		PFC_CLASS_NOT_COPYABLE_EX(SubProcess)
	};
}

#endif // _WIN32

