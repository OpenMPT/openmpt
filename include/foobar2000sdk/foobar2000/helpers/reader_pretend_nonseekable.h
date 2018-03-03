#pragma once

class reader_pretend_nonseekable : public file {
public:
	reader_pretend_nonseekable( file::ptr f, bool pretendRemote = true ) : m_file(f), m_pretendRemote(pretendRemote) {}

	t_filesize get_size(abort_callback & p_abort) {
		return m_file->get_size(p_abort);
	}


	t_filesize get_position(abort_callback & p_abort) {
		return m_file->get_position(p_abort);
	}

	void resize(t_filesize p_size, abort_callback & p_abort) {
		throw exception_io_denied();
	}

	void seek(t_filesize p_position, abort_callback & p_abort) {
		throw exception_io_object_not_seekable();
	}

	void seek_ex(t_sfilesize p_position, t_seek_mode p_mode, abort_callback & p_abort) {
		throw exception_io_object_not_seekable();
	}

	bool can_seek() {return false;}

	bool get_content_type(pfc::string_base & p_out) {
		return m_file->get_content_type(p_out);
	}

	bool is_in_memory() { return m_file->is_in_memory(); }

	void on_idle(abort_callback & p_abort) {m_file->on_idle(p_abort);}

	t_filetimestamp get_timestamp(abort_callback & p_abort) { return m_file->get_timestamp(p_abort); }

	void reopen(abort_callback & p_abort) {
		m_file->reopen(p_abort);
	}

	bool is_remote() {return m_pretendRemote;}

	size_t read( void * ptr, size_t bytes, abort_callback & abort ) { return m_file->read(ptr, bytes, abort); }
	void read_object(void * p_buffer, t_size p_bytes, abort_callback & p_abort) {m_file->read_object(p_buffer, p_bytes, p_abort); }
	t_filesize skip(t_filesize p_bytes, abort_callback & p_abort) { return m_file->skip(p_bytes, p_abort); } 
	void skip_object(t_filesize p_bytes, abort_callback & p_abort) { m_file->skip_object(p_bytes, p_abort); }
	void write( const void * ptr, size_t bytes, abort_callback & abort ) { throw exception_io_denied(); }
private:
	const file::ptr m_file;
	const bool m_pretendRemote;
};