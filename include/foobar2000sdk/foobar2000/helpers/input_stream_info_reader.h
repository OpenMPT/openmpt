#pragma once

#ifdef FOOBAR2000_DESKTOP

template<typename input_t>
class input_stream_info_reader_impl : public input_stream_info_reader {
public:
	input_t theInput;

	uint32_t get_stream_count() {
		return theInput.get_stream_count();
	}
	void get_stream_info(uint32_t index, file_info & out, abort_callback & a) {
		theInput.get_stream_info(index, out, a);
	}
	uint32_t get_default_stream() {
		return theInput.get_default_stream();
	}
};

template<typename input_t>
class input_stream_info_reader_entry_impl : public input_stream_info_reader_entry {
public:
	input_stream_info_reader::ptr open(const char * path, file::ptr fileHint, abort_callback & abort) {
		typedef input_stream_info_reader_impl<input_t> obj_t;
		service_ptr_t<obj_t> p = new service_impl_t<obj_t>();
		p->theInput.open(fileHint, path, input_open_info_read, abort);
		return p;
	}
	GUID get_guid() {
		return input_t::g_get_guid();
	}
};

#endif // FOOBAR2000_DESKTOP
