class fullFileBuffer {
public:
	fullFileBuffer() {
		//hMutex = CreateMutex(NULL, FALSE, pfc::stringcvt::string_os_from_utf8(pfc::string_formatter() << "{3ABC4D98-2510-431C-A720-26BEB45F0278}-" << (uint32_t) GetCurrentProcessId()));
	}
	~fullFileBuffer() {
		//CloseHandle(hMutex);
	}
	file::ptr open(const char * path, abort_callback & abort, file::ptr hint, t_filesize sizeMax = 1024*1024*256) {
		//mutexScope scope(hMutex, abort);

		file::ptr f;
		if (hint.is_valid()) f = hint;
		else filesystem::g_open_read( f, path, abort );
		t_filesize fs = f->get_size(abort);
		if (fs < sizeMax) /*rejects size-unknown too*/ {
			try {
				service_ptr_t<reader_bigmem_mirror> r = new service_impl_t<reader_bigmem_mirror>();
				r->init( f, abort );
				f = r;
			} catch(std::bad_alloc) {}
		}
		return f;
	}

private:
	fullFileBuffer(const fullFileBuffer&);
	void operator=(const fullFileBuffer&);

	//HANDLE hMutex;
};
