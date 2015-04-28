namespace file_win32_helpers {
	t_filesize get_size(HANDLE p_handle);
	void seek(HANDLE p_handle,t_sfilesize p_position,file::t_seek_mode p_mode);
	void fillOverlapped(OVERLAPPED & ol, HANDLE myEvent, t_filesize s);
	void writeOverlappedPass(HANDLE handle, HANDLE myEvent, t_filesize position, const void * in,DWORD inBytes, abort_callback & abort);
	void writeOverlapped(HANDLE handle, HANDLE myEvent, t_filesize & position, const void * in, size_t inBytes, abort_callback & abort);
	void writeStreamOverlapped(HANDLE handle, HANDLE myEvent, const void * in, size_t inBytes, abort_callback & abort);
	DWORD readOverlappedPass(HANDLE handle, HANDLE myEvent, t_filesize position, void * out, DWORD outBytes, abort_callback & abort);
	size_t readOverlapped(HANDLE handle, HANDLE myEvent, t_filesize & position, void * out, size_t outBytes, abort_callback & abort);
	size_t readStreamOverlapped(HANDLE handle, HANDLE myEvent, void * out, size_t outBytes, abort_callback & abort);
	HANDLE createFile(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile, abort_callback & abort);
};

template<bool p_seekable,bool p_writeable>
class file_win32_wrapper_t : public file {
public:
	file_win32_wrapper_t(HANDLE p_handle) : m_handle(p_handle), m_position(0)
	{
	}

	static file::ptr g_CreateFile(const char * p_path,DWORD p_access,DWORD p_sharemode,LPSECURITY_ATTRIBUTES p_security_attributes,DWORD p_createmode,DWORD p_flags,HANDLE p_template) {
		SetLastError(NO_ERROR);
		HANDLE handle = uCreateFile(p_path,p_access,p_sharemode,p_security_attributes,p_createmode,p_flags,p_template);
		if (handle == INVALID_HANDLE_VALUE) {
			const DWORD code = GetLastError();
			if (p_access & GENERIC_WRITE) win32_file_write_failure(code, p_path);
			else exception_io_from_win32(code);
		}
		try {
			return g_create_from_handle(handle);
		} catch(...) {CloseHandle(handle); throw;}
	}

	static service_ptr_t<file> g_create_from_handle(HANDLE p_handle) {
		return new service_impl_t<file_win32_wrapper_t<p_seekable,p_writeable> >(p_handle);
	}


	void reopen(abort_callback & p_abort) {seek(0,p_abort);}

	void write(const void * p_buffer,t_size p_bytes,abort_callback & p_abort) {
		if (!p_writeable) throw exception_io_denied();

		PFC_STATIC_ASSERT(sizeof(t_size) >= sizeof(DWORD));

		t_size bytes_written_total = 0;

		if (sizeof(t_size) == sizeof(DWORD)) {
			p_abort.check_e();
			DWORD bytes_written = 0;
			SetLastError(ERROR_SUCCESS);
			if (!WriteFile(m_handle,p_buffer,(DWORD)p_bytes,&bytes_written,0)) exception_io_from_win32(GetLastError());
			if (bytes_written != p_bytes) throw exception_io("Write failure");
			bytes_written_total = bytes_written;
			m_position += bytes_written;
		} else {
			while(bytes_written_total < p_bytes) {
				p_abort.check_e();
				DWORD bytes_written = 0;
				DWORD delta = (DWORD) pfc::min_t<t_size>(p_bytes - bytes_written_total, 0x80000000);
				SetLastError(ERROR_SUCCESS);
				if (!WriteFile(m_handle,(const t_uint8*)p_buffer + bytes_written_total,delta,&bytes_written,0)) exception_io_from_win32(GetLastError());
				if (bytes_written != delta) throw exception_io("Write failure");
				bytes_written_total += bytes_written;
				m_position += bytes_written;
			}
		}
	}
	
	t_size read(void * p_buffer,t_size p_bytes,abort_callback & p_abort) {
		PFC_STATIC_ASSERT(sizeof(t_size) >= sizeof(DWORD));
		
		t_size bytes_read_total = 0;
		if (sizeof(t_size) == sizeof(DWORD)) {
			p_abort.check_e();
			DWORD bytes_read = 0;
			SetLastError(ERROR_SUCCESS);
			if (!ReadFile(m_handle,p_buffer,pfc::downcast_guarded<DWORD>(p_bytes),&bytes_read,0)) exception_io_from_win32(GetLastError());
			bytes_read_total = bytes_read;
			m_position += bytes_read;
		} else {
			while(bytes_read_total < p_bytes) {
				p_abort.check_e();
				DWORD bytes_read = 0;
				DWORD delta = (DWORD) pfc::min_t<t_size>(p_bytes - bytes_read_total, 0x80000000);
				SetLastError(ERROR_SUCCESS);
				if (!ReadFile(m_handle,(t_uint8*)p_buffer + bytes_read_total,delta,&bytes_read,0)) exception_io_from_win32(GetLastError());
				bytes_read_total += bytes_read;
				m_position += bytes_read;
				if (bytes_read != delta) break;
			}
		}
		return bytes_read_total;
	}


	t_filesize get_size(abort_callback & p_abort) {
		p_abort.check_e();
		return file_win32_helpers::get_size(m_handle);
	}

	t_filesize get_position(abort_callback & p_abort) {
		p_abort.check_e();
		return m_position;
	}
	
	void resize(t_filesize p_size,abort_callback & p_abort) {
		if (!p_writeable) throw exception_io_denied();
		p_abort.check_e();
		if (m_position != p_size) {
			file_win32_helpers::seek(m_handle,p_size,file::seek_from_beginning);
		}
		SetLastError(ERROR_SUCCESS);
		if (!SetEndOfFile(m_handle)) {
			DWORD code = GetLastError();
			if (m_position != p_size) try {file_win32_helpers::seek(m_handle,m_position,file::seek_from_beginning);} catch(...) {}
			exception_io_from_win32(code);
		}
		if (m_position > p_size) m_position = p_size;
		if (m_position != p_size) file_win32_helpers::seek(m_handle,m_position,file::seek_from_beginning);
	}


	void seek(t_filesize p_position,abort_callback & p_abort) {
		if (!p_seekable) throw exception_io_object_not_seekable();
		p_abort.check_e();
		if (p_position > file_win32_helpers::get_size(m_handle)) throw exception_io_seek_out_of_range();
		file_win32_helpers::seek(m_handle,p_position,file::seek_from_beginning);
		m_position = p_position;
	}

	bool can_seek() {return p_seekable;}
	bool get_content_type(pfc::string_base & out) {return false;}
	bool is_in_memory() {return false;}
	void on_idle(abort_callback & p_abort) {p_abort.check_e();}
	
	t_filetimestamp get_timestamp(abort_callback & p_abort) {
		p_abort.check_e();
		FlushFileBuffers(m_handle);
		SetLastError(ERROR_SUCCESS);
		t_filetimestamp temp;
		if (!GetFileTime(m_handle,0,0,(FILETIME*)&temp)) exception_io_from_win32(GetLastError());
		return temp;
	}

	bool is_remote() {return false;}
	~file_win32_wrapper_t() {CloseHandle(m_handle);}
protected:
	HANDLE m_handle;
	t_filesize m_position;
};

template<bool p_writeable>
class file_win32_wrapper_overlapped_t : public file {
public:
	file_win32_wrapper_overlapped_t(HANDLE file) : m_handle(file), m_position() {
		WIN32_OP( (m_event = CreateEvent(NULL, TRUE, FALSE, NULL)) != NULL );
	}
	~file_win32_wrapper_overlapped_t() {CloseHandle(m_event); CloseHandle(m_handle);}
	void write(const void * p_buffer,t_size p_bytes,abort_callback & p_abort) {
		if (!p_writeable) throw exception_io_denied();
		return file_win32_helpers::writeOverlapped(m_handle, m_event, m_position, p_buffer, p_bytes, p_abort);
	}
	t_size read(void * p_buffer,t_size p_bytes,abort_callback & p_abort) {
		return file_win32_helpers::readOverlapped(m_handle, m_event, m_position, p_buffer, p_bytes, p_abort);
	}

	void reopen(abort_callback & p_abort) {seek(0,p_abort);}


	t_filesize get_size(abort_callback & p_abort) {
		p_abort.check_e();
		return file_win32_helpers::get_size(m_handle);
	}

	t_filesize get_position(abort_callback & p_abort) {
		p_abort.check_e();
		return m_position;
	}
	
	void resize(t_filesize p_size,abort_callback & p_abort) {
		if (!p_writeable) throw exception_io_denied();
		p_abort.check_e();
		file_win32_helpers::seek(m_handle,p_size,file::seek_from_beginning);
		SetLastError(ERROR_SUCCESS);
		if (!SetEndOfFile(m_handle)) {
			DWORD code = GetLastError();
			exception_io_from_win32(code);
		}
		if (m_position > p_size) m_position = p_size;
	}


	void seek(t_filesize p_position,abort_callback & p_abort) {
		p_abort.check_e();
		if (p_position > file_win32_helpers::get_size(m_handle)) throw exception_io_seek_out_of_range();
		// file_win32_helpers::seek(m_handle,p_position,file::seek_from_beginning);
		m_position = p_position;
	}

	bool can_seek() {return true;}
	bool get_content_type(pfc::string_base & out) {return false;}
	bool is_in_memory() {return false;}
	void on_idle(abort_callback & p_abort) {p_abort.check_e();}
	
	t_filetimestamp get_timestamp(abort_callback & p_abort) {
		p_abort.check_e();
		FlushFileBuffers(m_handle);
		SetLastError(ERROR_SUCCESS);
		t_filetimestamp temp;
		if (!GetFileTime(m_handle,0,0,(FILETIME*)&temp)) exception_io_from_win32(GetLastError());
		return temp;
	}

	bool is_remote() {return false;}
	

	static file::ptr g_CreateFile(const char * p_path,DWORD p_access,DWORD p_sharemode,LPSECURITY_ATTRIBUTES p_security_attributes,DWORD p_createmode,DWORD p_flags,HANDLE p_template) {
		p_flags |= FILE_FLAG_OVERLAPPED;
		SetLastError(NO_ERROR);
		HANDLE handle = uCreateFile(p_path,p_access,p_sharemode,p_security_attributes,p_createmode,p_flags,p_template);
		if (handle == INVALID_HANDLE_VALUE) {
			const DWORD code = GetLastError();
			if (p_access & GENERIC_WRITE) win32_file_write_failure(code, p_path);
			else exception_io_from_win32(code);
		}
		try {
			return g_create_from_handle(handle);
		} catch(...) {CloseHandle(handle); throw;}
	}

	static file::ptr g_create_from_handle(HANDLE p_handle) {
		return new service_impl_t<file_win32_wrapper_overlapped_t<p_writeable> >(p_handle);
	}

protected:
	HANDLE m_event, m_handle;
	t_filesize m_position;
};
