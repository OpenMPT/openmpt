namespace ProfileCache {
	inline file::ptr FetchFile(const char * context, const char * name, const char * webURL, t_filetimestamp acceptableAge, abort_callback & abort) {
		const double timeoutVal = 5;

		pfc::string_formatter path ( core_api::get_profile_path() );
		path.fix_dir_separator('\\');
		path << context;
		try {
			filesystem::g_create_directory(path, abort);
		} catch(exception_io_already_exists) {}
		path << "\\" << name;

		bool fetch = false;
		file::ptr fLocal;
		
		try {
			filesystem::g_open_timeout(fLocal, path, filesystem::open_mode_write_existing, timeoutVal, abort);
			fetch = fLocal->get_timestamp(abort) < filetimestamp_from_system_timer() - acceptableAge;
		} catch(exception_io_not_found) {
			filesystem::g_open_timeout(fLocal, path, filesystem::open_mode_write_new, timeoutVal, abort);
			fetch = true;
		}
		if (fetch) {
			fLocal->resize(0, abort);
			file::ptr fRemote;
			try {
				filesystem::g_open(fRemote, webURL, filesystem::open_mode_read, abort);
			} catch(exception_io_not_found) {
				fLocal.release();
				try { filesystem::g_remove_timeout(path, timeoutVal, abort); } catch(...) {}
				throw;
			}
			pfc::array_t<t_uint8> buffer; buffer.set_size(64*1024);
			for(;;) {
				t_size delta = buffer.get_size();
				delta = fRemote->read(buffer.get_ptr(), delta, abort);
				fLocal->write(buffer.get_ptr(), delta, abort);
				if (delta < buffer.get_size()) break;
			}
			fLocal->seek(0, abort);
		}
		return fLocal;
	}
};
