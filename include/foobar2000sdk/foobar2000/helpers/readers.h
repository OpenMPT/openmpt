#pragma once

class NOVTABLE reader_membuffer_base : public file_readonly {
public:
	reader_membuffer_base() : m_offset(0) {}

	t_size read(void * p_buffer, t_size p_bytes, abort_callback & p_abort);

	void write(const void * p_buffer, t_size p_bytes, abort_callback & p_abort) { throw exception_io_denied(); }

	t_filesize get_size(abort_callback & p_abort) { return get_buffer_size(); }
	t_filesize get_position(abort_callback & p_abort) { return m_offset; }
	void seek(t_filesize position, abort_callback & p_abort);
	void reopen(abort_callback & p_abort) { seek(0, p_abort); }

	bool can_seek() { return true; }
	bool is_in_memory() { return true; }

protected:
	virtual const void * get_buffer() = 0;
	virtual t_size get_buffer_size() = 0;
	virtual t_filetimestamp get_timestamp(abort_callback & p_abort) = 0;
	virtual bool get_content_type(pfc::string_base &) { return false; }
	inline void seek_internal(t_size p_offset) { if (p_offset > get_buffer_size()) throw exception_io_seek_out_of_range(); m_offset = p_offset; }
private:
	t_size m_offset;
};

class reader_membuffer_simple : public reader_membuffer_base {
public:
	reader_membuffer_simple(const void * ptr, t_size size, t_filetimestamp ts = filetimestamp_invalid, bool is_remote = false) : m_isRemote(is_remote), m_ts(ts) {
		m_data.set_size_discard(size);
		memcpy(m_data.get_ptr(), ptr, size);
	}
	const void * get_buffer() { return m_data.get_ptr(); }
	t_size get_buffer_size() { return m_data.get_size(); }
	t_filetimestamp get_timestamp(abort_callback & p_abort) { return m_ts; }
	bool is_remote() { return m_isRemote; }
private:
	pfc::array_staticsize_t<t_uint8> m_data;
	t_filetimestamp m_ts;
	bool m_isRemote;
};

class reader_membuffer_mirror : public reader_membuffer_base
{
public:
	t_filetimestamp get_timestamp(abort_callback & p_abort) { return m_timestamp; }
	bool is_remote() { return m_remote; }

	//! Returns false when the object could not be mirrored (too big) or did not need mirroring.
	static bool g_create(service_ptr_t<file> & p_out, const service_ptr_t<file> & p_src, abort_callback & p_abort) {
		service_ptr_t<reader_membuffer_mirror> ptr = new service_impl_t<reader_membuffer_mirror>();
		if (!ptr->init(p_src, p_abort)) return false;
		p_out = ptr.get_ptr();
		return true;
	}
	bool get_content_type(pfc::string_base & out) {
		if (m_contentType.is_empty()) return false;
		out = m_contentType; return true;
	}
private:
	const void * get_buffer() { return m_buffer.get_ptr(); }
	t_size get_buffer_size() { return m_buffer.get_size(); }

	bool init(const service_ptr_t<file> & p_src, abort_callback & p_abort) {
		if (p_src->is_in_memory()) return false;//already buffered
		if (!p_src->get_content_type(m_contentType)) m_contentType.reset();
		m_remote = p_src->is_remote();

		t_size size = pfc::downcast_guarded<t_size>(p_src->get_size(p_abort));

		m_buffer.set_size(size);

		p_src->reopen(p_abort);

		p_src->read_object(m_buffer.get_ptr(), size, p_abort);

		m_timestamp = p_src->get_timestamp(p_abort);

		return true;
	}


	t_filetimestamp m_timestamp;
	pfc::array_t<char> m_buffer;
	bool m_remote;
	pfc::string8 m_contentType;

};

class reader_limited : public file_readonly {
	service_ptr_t<file> r;
	t_filesize begin;
	t_filesize end;

public:
	static file::ptr g_create(file::ptr base, t_filesize offset, t_filesize size, abort_callback & abort) {
		service_ptr_t<reader_limited> r = new service_impl_t<reader_limited>();
		if (offset + size < offset) throw pfc::exception_overflow();
		r->init(base, offset, offset + size, abort);
		return r;
	}
	reader_limited() { begin = 0;end = 0; }
	reader_limited(const service_ptr_t<file> & p_r, t_filesize p_begin, t_filesize p_end, abort_callback & p_abort) {
		r = p_r;
		begin = p_begin;
		end = p_end;
		reopen(p_abort);
	}

	void init(const service_ptr_t<file> & p_r, t_filesize p_begin, t_filesize p_end, abort_callback & p_abort) {
		r = p_r;
		begin = p_begin;
		end = p_end;
		reopen(p_abort);
	}

	t_filetimestamp get_timestamp(abort_callback & p_abort) { return r->get_timestamp(p_abort); }

	t_size read(void *p_buffer, t_size p_bytes, abort_callback & p_abort) {
		t_filesize pos;
		pos = r->get_position(p_abort);
		if (p_bytes > end - pos) p_bytes = (t_size)(end - pos);
		return r->read(p_buffer, p_bytes, p_abort);
	}

	t_filesize get_size(abort_callback & p_abort) { return end - begin; }

	t_filesize get_position(abort_callback & p_abort) {
		return r->get_position(p_abort) - begin;
	}

	void seek(t_filesize position, abort_callback & p_abort) {
		r->seek(position + begin, p_abort);
	}
	bool can_seek() { return r->can_seek(); }
	bool is_remote() { return r->is_remote(); }

	bool get_content_type(pfc::string_base &) { return false; }

	void reopen(abort_callback & p_abort) {
		seekInternal(begin, p_abort);
	}
private:
	void seekInternal(t_filesize position, abort_callback & abort) {
		if (r->can_seek()) {
			r->seek(position, abort);
		}
		else {
			t_filesize positionWas = r->get_position(abort);
			if (positionWas == filesize_invalid || positionWas > position) {
				r->reopen(abort);
				try { r->skip_object(position, abort); }
				catch (exception_io_data) { throw exception_io_seek_out_of_range(); }
			}
			else {
				t_filesize skipMe = position - positionWas;
				if (skipMe > 0) {
					try { r->skip_object(skipMe, abort); }
					catch (exception_io_data) { throw exception_io_seek_out_of_range(); }
				}
			}
		}
	}
};



// A more clever version of reader_membuffer_*.
// Behaves more nicely with large files within 32bit address space.
class reader_bigmem : public file_readonly {
public:
	reader_bigmem() : m_offset() {}
	t_size read(void * p_buffer, t_size p_bytes, abort_callback & p_abort) {
		pfc::min_acc(p_bytes, remaining());
		m_mem.read(p_buffer, p_bytes, m_offset);
		m_offset += p_bytes;
		return p_bytes;
	}
	void read_object(void * p_buffer, t_size p_bytes, abort_callback & p_abort) {
		if (p_bytes > remaining()) throw exception_io_data_truncation();
		m_mem.read(p_buffer, p_bytes, m_offset);
		m_offset += p_bytes;
	}
	t_filesize skip(t_filesize p_bytes, abort_callback & p_abort) {
		pfc::min_acc(p_bytes, (t_filesize)remaining());
		m_offset += (size_t)p_bytes;
		return p_bytes;
	}
	void skip_object(t_filesize p_bytes, abort_callback & p_abort) {
		if (p_bytes > remaining()) throw exception_io_data_truncation();
		m_offset += (size_t)p_bytes;
	}

	t_filesize get_size(abort_callback & p_abort) { p_abort.check(); return m_mem.size(); }
	t_filesize get_position(abort_callback & p_abort) { p_abort.check(); return m_offset; }
	void seek(t_filesize p_position, abort_callback & p_abort) {
		if (p_position > m_mem.size()) throw exception_io_seek_out_of_range();
		m_offset = (size_t)p_position;
	}
	bool can_seek() { return true; }
	bool is_in_memory() { return true; }
	void reopen(abort_callback & p_abort) { seek(0, p_abort); }

	// To be overridden by individual derived classes
	bool get_content_type(pfc::string_base & p_out) { return false; }
	t_filetimestamp get_timestamp(abort_callback & p_abort) { return filetimestamp_invalid; }
	bool is_remote() { return false; }
protected:
	void resize(size_t newSize) {
		m_offset = 0;
		m_mem.resize(newSize);
	}
	size_t remaining() const { return m_mem.size() - m_offset; }
	pfc::bigmem m_mem;
	size_t m_offset;
};

class reader_bigmem_mirror : public reader_bigmem {
public:
	reader_bigmem_mirror() {}

	void init(file::ptr source, abort_callback & abort) {
		source->reopen(abort);
		t_filesize fs = source->get_size(abort);
		if (fs > 1024 * 1024 * 1024) { // reject > 1GB
			throw std::bad_alloc();
		}
		size_t s = (size_t)fs;
		resize(s);
		for (size_t walk = 0; walk < m_mem._sliceCount(); ++walk) {
			source->read(m_mem._slicePtr(walk), m_mem._sliceSize(walk), abort);
		}

		if (!source->get_content_type(m_contentType)) m_contentType.reset();
		m_isRemote = source->is_remote();
		m_ts = source->get_timestamp(abort);
	}

	bool get_content_type(pfc::string_base & p_out) {
		if (m_contentType.is_empty()) return false;
		p_out = m_contentType; return true;
	}
	t_filetimestamp get_timestamp(abort_callback & p_abort) { return m_ts; }
	bool is_remote() { return m_isRemote; }
private:
	t_filetimestamp m_ts;
	pfc::string8 m_contentType;
	bool m_isRemote;
};

class file_chain : public file {
public:
	t_size read(void * p_buffer, t_size p_bytes, abort_callback & p_abort) {
		return m_file->read(p_buffer, p_bytes, p_abort);
	}
	void read_object(void * p_buffer, t_size p_bytes, abort_callback & p_abort) {
		m_file->read_object(p_buffer, p_bytes, p_abort);
	}
	t_filesize skip(t_filesize p_bytes, abort_callback & p_abort) {
		return m_file->skip(p_bytes, p_abort);
	}
	void skip_object(t_filesize p_bytes, abort_callback & p_abort) {
		m_file->skip_object(p_bytes, p_abort);
	}
	void write(const void * p_buffer, t_size p_bytes, abort_callback & p_abort) {
		m_file->write(p_buffer, p_bytes, p_abort);
	}

	t_filesize get_size(abort_callback & p_abort) {
		return m_file->get_size(p_abort);
	}

	t_filesize get_position(abort_callback & p_abort) {
		return m_file->get_position(p_abort);
	}

	void resize(t_filesize p_size, abort_callback & p_abort) {
		m_file->resize(p_size, p_abort);
	}

	void seek(t_filesize p_position, abort_callback & p_abort) {
		m_file->seek(p_position, p_abort);
	}

	void seek_ex(t_sfilesize p_position, t_seek_mode p_mode, abort_callback & p_abort) {
		m_file->seek_ex(p_position, p_mode, p_abort);
	}

	bool can_seek() { return m_file->can_seek(); }
	bool get_content_type(pfc::string_base & p_out) { return m_file->get_content_type(p_out); }
	bool is_in_memory() { return m_file->is_in_memory(); }
	void on_idle(abort_callback & p_abort) { m_file->on_idle(p_abort); }
#if FOOBAR2000_TARGET_VERSION >= 2000
	t_filestats get_stats(abort_callback & abort) { return m_file->get_stats(abort); }
#else
	t_filetimestamp get_timestamp(abort_callback & p_abort) { return m_file->get_timestamp(p_abort); }
#endif
	void reopen(abort_callback & p_abort) { m_file->reopen(p_abort); }
	bool is_remote() { return m_file->is_remote(); }

	file_chain(file::ptr chain) : m_file(chain) {}
private:
	file::ptr m_file;
};

class file_chain_readonly : public file_chain {
public:
	void write(const void * p_buffer, t_size p_bytes, abort_callback & p_abort) { throw exception_io_denied(); }
	void resize(t_filesize p_size, abort_callback & p_abort) { throw exception_io_denied(); }
	file_chain_readonly(file::ptr chain) : file_chain(chain) {}
	static file::ptr create(file::ptr chain) { return new service_impl_t< file_chain_readonly >(chain); }
};
