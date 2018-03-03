#pragma once

#ifdef PFC_WINDOWS_STORE_APP
#include <pfc/pp-winapi.h>
#endif

#if defined(_WIN32) && defined(FOOBAR2000_MOBILE)
#include <pfc/timers.h>
#include <pfc/string_conv.h>
#endif


class tag_write_callback_impl : public tag_write_callback {
public:
	tag_write_callback_impl(const char * p_origpath) : m_origpath(p_origpath) {}
	bool open_temp_file(service_ptr_t<file> & p_out,abort_callback & p_abort) {
		pfc::dynamic_assert(m_tempfile.is_empty());
		generate_temp_location_for_file(m_temppath,m_origpath,/*pfc::string_extension(m_origpath)*/ "tmp","retagging temporary file");
		service_ptr_t<file> l_tempfile;
		try {
			openTempFile(l_tempfile, p_abort);
		} catch(exception_io_denied) {return false;}
		p_out = m_tempfile = l_tempfile;
		return true;
	}
	bool got_temp_file() const {return m_tempfile.is_valid();}

	// p_owner must be the only reference to open source file, it will be closed + reopened
	//WARNING: if this errors out, it may leave caller with null file pointer; take appropriate measures not to crash in such cases
	void finalize(service_ptr_t<file> & p_owner,abort_callback & p_abort) {
		if (m_tempfile.is_valid()) {
			m_tempfile.release();
			p_owner.release();
			handleFileMove(m_temppath, m_origpath, p_abort);
			input_open_file_helper(p_owner,m_origpath,input_open_info_write,p_abort);
		}
	}
	// Alternate finalizer without owner file object, caller takes responsibility for closing the source file before calling
	void finalize_no_reopen( abort_callback & p_abort ) {
		if (m_tempfile.is_valid()) {
			m_tempfile.release();
			handleFileMove(m_temppath, m_origpath, p_abort);
		}
	}
	void handle_failure() throw() {
		if (m_tempfile.is_valid()) {
			m_tempfile.release();
			try {
                abort_callback_dummy dumdumdum;
				filesystem::g_remove_timeout(m_temppath,1.0,dumdumdum);
			} catch(...) {}
		}
	}
private:
	void openTempFile(file::ptr & out, abort_callback & abort) {
		filesystem::g_open_write_new(out,m_temppath,abort);
	}
	void handleFileMove(const char * from, const char * to, abort_callback & abort) {
		pfc::string8 from_, to_;
		if (foobar2000_io::extract_native_path_ex(from, from_) && foobar2000_io::extract_native_path_ex(to, to_)) {
			try {
				myMoveFile(from_, to_, abort);
			} catch(...) {
                abort_callback_dummy dumdumdum;
				try {filesystem::g_remove_timeout(m_temppath,1.0,dumdumdum);} catch(...) {}
				throw;
			}
		} else {
			handleFileMoveFallback(from, to, abort);
		}
	}
	static void myMoveFile(const char * from, const char * to, abort_callback & abort) {
#ifdef _WIN32
		const double timeout = 2 * 60.0;
		pfc::lores_timer timer;
		timer.start();
		pfc::stringcvt::string_os_from_utf8 fromT(from), toT(to);
		for(;;) {
			try {
				XferAttribs(toT, fromT);
				WIN32_IO_OP( MoveFileEx(fromT, toT, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED) );
				break;
			} catch(exception_io_sharing_violation) {
				if (timer.query() > timeout) throw;
				abort.sleep(0.01);
			} catch(exception_io_denied) {
				// Windows bug: when the target file is open by someone else not permitting us to write there, we get access denied and not sharing violation
				if (timer.query() > timeout) throw;
				abort.sleep(0.01);
			}

		}
#else
        abort.check();
        NIX_IO_OP( rename( from, to ) == 0 );
        // FIX ME reverse transfer attribs
        
#endif
	}
#ifdef _WIN32
	static void OpenFileHelper(pfc::winHandle & out, const TCHAR * path, DWORD access, DWORD shareMode) {
		SetLastError(NO_ERROR);
		HANDLE h = CreateFileW(path, access, shareMode, NULL, OPEN_EXISTING, 0, NULL);
		if (h == INVALID_HANDLE_VALUE) exception_io_from_win32(GetLastError());
		out.Attach(h);
	}
	static void XferAttribs(const TCHAR * from, const TCHAR * to) {
		pfc::winHandle hFrom, hTo;
		const DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
		OpenFileHelper(hFrom, from, GENERIC_READ, share);
		OpenFileHelper(hTo, to, GENERIC_WRITE, share);
#ifdef FOOBAR2000_DESKTOP_WINDOWS
		FILETIME creation = {};
		WIN32_IO_OP( GetFileTime(hFrom, &creation, NULL, NULL) );
		WIN32_IO_OP( SetFileTime(hTo, &creation, NULL, NULL) );
#endif
		SetLastError(NO_ERROR);
		DWORD attribs = GetFileAttributes(from);
		if (attribs == ~0) exception_io_from_win32(GetLastError());
		SetFileAttributes(to, attribs);
	}
#endif
#if 0 // unused
	static void myReplaceFile(const char * from, const char * to, abort_callback & abort) {
#ifdef _WIN32
		const double timeout = 2 * 60.0;
		pfc::lores_timer timer;
		timer.start();
		pfc::stringcvt::string_os_from_utf8 fromT(from), toT(to);
		for(;;) {
			try {
				WIN32_IO_OP( ReplaceFile(toT, fromT, /*backup*/ NULL, /*flags*/ 0, /*reserved*/ NULL, /*reserved*/NULL) );
				break;
			} catch(exception_io) {
				if (timer.query() > timeout) throw;
				abort.sleep(0.01);
			}
		}
#else
        abort.check();
        NIX_IO_OP( rename( from, to ) == 0 );
#endif
	}
#endif

	void handleFileMoveFallback(const char * from, const char * to, abort_callback & abort) {
		try {
			filesystem::g_remove_timeout(m_origpath,10.0,abort);
		} catch(...) {
            abort_callback_dummy dumdumdum;
			try {filesystem::g_remove_timeout(m_temppath,1.0,dumdumdum);} catch(...) {}
			throw;
		}

		filesystem::g_move_timeout(m_temppath,m_origpath,5*60.0,abort);
	}
	pfc::string8 m_origpath;
	pfc::string8 m_temppath;
	service_ptr_t<file> m_tempfile;
};
