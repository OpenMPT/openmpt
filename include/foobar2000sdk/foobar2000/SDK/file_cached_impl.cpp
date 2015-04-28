#include "foobar2000.h"
namespace {

#define FILE_CACHED_DEBUG_LOG 0

class file_cached_impl_v2 : public file_cached {
public:
	enum {minBlockSize = 4096};
	enum {maxSkipSize = 128*1024};
	file_cached_impl_v2(size_t maxBlockSize) : m_maxBlockSize(maxBlockSize) {
		//m_buffer.set_size(blocksize);
	}
	size_t get_cache_block_size() {return m_maxBlockSize;}
	void suggest_grow_cache(size_t suggestSize) {
		if (m_maxBlockSize < suggestSize) m_maxBlockSize = suggestSize;
	}

	void initialize(service_ptr_t<file> p_base,abort_callback & p_abort) {
		m_base = p_base;
		m_can_seek = m_base->can_seek();
		_reinit(p_abort);
	}
private:
	void _reinit(abort_callback & p_abort) {
		m_position = 0;
		
		if (m_can_seek) {
			m_position_base = m_base->get_position(p_abort);
		} else {
			m_position_base = 0;
		}

		m_size = m_base->get_size(p_abort);

		flush_buffer();
	}
public:

	t_filesize skip(t_filesize p_bytes,abort_callback & p_abort) {
		if (p_bytes > maxSkipSize) {
			const t_filesize size = get_size(p_abort);
			if (size != filesize_invalid) {
				const t_filesize position = get_position(p_abort);
				const t_filesize toskip = pfc::min_t( p_bytes, size - position );
				seek(position + toskip,p_abort);
				return toskip;
			}
		}
		return skip_( p_bytes, p_abort );
	}
	t_filesize skip_(t_filesize p_bytes,abort_callback & p_abort) {
#if FILE_CACHED_DEBUG_LOG
		FB2K_DebugLog() << "Skipping bytes: " << p_bytes;
#endif
		t_filesize todo = p_bytes;
		for(;;) {
			size_t inBuffer = this->bufferRemaining();
			size_t delta = (size_t) pfc::min_t<t_filesize>(inBuffer, todo);
			m_bufferReadPtr += delta;
			m_position += delta;
			todo -= delta;
			if (todo == 0) break;
			p_abort.check();
			this->m_bufferState = 0; // null it early to leave in a consistent state if base read fails
			this->m_bufferReadPtr = 0;
			baseSeek(m_position,p_abort);
			m_readSize = pfc::min_t<size_t>(m_readSize << 1, this->m_maxBlockSize);
			if (m_readSize < minBlockSize) m_readSize = minBlockSize;
#if FILE_CACHED_DEBUG_LOG
			FB2K_DebugLog() << "Growing read size: " << m_readSize;
#endif
			m_buffer.grow_size(m_readSize);
			m_bufferState = m_base->read(m_buffer.get_ptr(), m_readSize, p_abort);
			if (m_bufferState == 0) break;
			m_position_base += m_bufferState;
		}

		return p_bytes - todo;
	}

	t_size read(void * p_buffer,t_size p_bytes,abort_callback & p_abort) {
#if FILE_CACHED_DEBUG_LOG
		FB2K_DebugLog() << "Reading bytes: " << p_bytes;
#endif
		t_uint8 * outptr = (t_uint8*)p_buffer;
		size_t todo = p_bytes;
		for(;;) {
			size_t inBuffer = this->bufferRemaining();
			size_t delta = pfc::min_t<size_t>(inBuffer, todo);
			memcpy(outptr, this->m_buffer.get_ptr() + m_bufferReadPtr, delta);
			m_bufferReadPtr += delta;
			m_position += delta;
			todo -= delta;
			if (todo == 0) break;
			p_abort.check();
			outptr += delta;
			this->m_bufferState = 0; // null it early to leave in a consistent state if base read fails
			this->m_bufferReadPtr = 0;
			baseSeek(m_position,p_abort);
			m_readSize = pfc::min_t<size_t>(m_readSize << 1, this->m_maxBlockSize);
			if (m_readSize < minBlockSize) m_readSize = minBlockSize;
#if FILE_CACHED_DEBUG_LOG
			FB2K_DebugLog() << "Growing read size: " << m_readSize;
#endif
			m_buffer.grow_size(m_readSize);
			m_bufferState = m_base->read(m_buffer.get_ptr(), m_readSize, p_abort);
			if (m_bufferState == 0) break;
			m_position_base += m_bufferState;
		}

		return p_bytes - todo;
	}

	void write(const void * p_buffer,t_size p_bytes,abort_callback & p_abort) {
#if FILE_CACHED_DEBUG_LOG
		FB2K_DebugLog() << "Writing bytes: " << p_bytes;
#endif
		p_abort.check();
		baseSeek(m_position,p_abort);
		m_base->write(p_buffer,p_bytes,p_abort);
		m_position_base = m_position = m_position + p_bytes;
		if (m_size < m_position) m_size = m_position;
		flush_buffer();
	}

	t_filesize get_size(abort_callback & p_abort) {
		p_abort.check();
		return m_size;
	}
	t_filesize get_position(abort_callback & p_abort) {
		p_abort.check();
		return m_position;
	}
	void set_eof(abort_callback & p_abort) {
		p_abort.check();
		baseSeek(m_position,p_abort);
		m_base->set_eof(p_abort);
		flush_buffer();
	}
	void seek(t_filesize p_position,abort_callback & p_abort) {
#if FILE_CACHED_DEBUG_LOG
		FB2K_DebugLog() << "Seeking: " << p_position;
#endif
		p_abort.check();
		if (!m_can_seek) throw exception_io_object_not_seekable();
		if (p_position > m_size) throw exception_io_seek_out_of_range();
		int64_t delta = p_position - m_position;

		// special case
		if (delta >= 0 && delta <= this->minBlockSize) {
#if FILE_CACHED_DEBUG_LOG
			FB2K_DebugLog() << "Skip-seeking: " << p_position;
#endif
			t_filesize skipped = this->skip_( delta, p_abort );
			PFC_ASSERT( skipped == delta ); (void) skipped;
			return;
		}

		m_position = p_position;
		// within currently buffered data?
		if ((delta >= 0 && (uint64_t) delta <= bufferRemaining()) || (delta < 0 && (uint64_t)(-delta) <= m_bufferReadPtr)) {
#if FILE_CACHED_DEBUG_LOG
			FB2K_DebugLog() << "Quick-seeking: " << p_position;
#endif
			m_bufferReadPtr += (ptrdiff_t)delta;
		} else {
#if FILE_CACHED_DEBUG_LOG
			FB2K_DebugLog() << "Slow-seeking: " << p_position;
#endif
			this->flush_buffer();
		}
	}
	void reopen(abort_callback & p_abort) {
		if (this->m_can_seek) {
			seek(0,p_abort);
		} else {
			this->m_base->reopen( p_abort );
			this->_reinit( p_abort );
		}
	}
	bool can_seek() {return m_can_seek;}
	bool get_content_type(pfc::string_base & out) {return m_base->get_content_type(out);}
	void on_idle(abort_callback & p_abort) {p_abort.check();m_base->on_idle(p_abort);}
	t_filetimestamp get_timestamp(abort_callback & p_abort) {p_abort.check(); return m_base->get_timestamp(p_abort);}
	bool is_remote() {return m_base->is_remote();}
	void resize(t_filesize p_size,abort_callback & p_abort) {
		flush_buffer();
		m_base->resize(p_size,p_abort);
		m_size = p_size;
		if (m_position > m_size) m_position = m_size;
		if (m_position_base > m_size) m_position_base = m_size;
	}
private:
	size_t bufferRemaining() const {return m_bufferState - m_bufferReadPtr;}
	void baseSeek(t_filesize p_target,abort_callback & p_abort) {
		if (p_target != m_position_base) {
			m_base->seek(p_target,p_abort);
			m_position_base = p_target;
		}
	}

	void flush_buffer() {
		m_bufferState = m_bufferReadPtr = 0;
		m_readSize = 0;
	}

	service_ptr_t<file> m_base;
	t_filesize m_position,m_position_base,m_size;
	bool m_can_seek;
	size_t m_bufferState, m_bufferReadPtr;
	pfc::array_t<t_uint8> m_buffer;
	size_t m_maxBlockSize;
	size_t m_readSize;
};

class file_cached_impl : public file_cached {
public:
	file_cached_impl(t_size blocksize) {
		m_buffer.set_size(blocksize);
	}
	size_t get_cache_block_size() {return m_buffer.get_size();}
	void suggest_grow_cache(size_t suggestSize) {}
	void initialize(service_ptr_t<file> p_base,abort_callback & p_abort) {
		m_base = p_base;
		m_can_seek = m_base->can_seek();
		_reinit(p_abort);
	}
private:
	void _reinit(abort_callback & p_abort) {
		m_position = 0;
		
		if (m_can_seek) {
			m_position_base = m_base->get_position(p_abort);
		} else {
			m_position_base = 0;
		}

		m_size = m_base->get_size(p_abort);

		flush_buffer();
	}
public:

	t_size read(void * p_buffer,t_size p_bytes,abort_callback & p_abort) {
		t_uint8 * outptr = (t_uint8*)p_buffer;
		t_size done = 0;
		while(done < p_bytes && m_position < m_size) {
			p_abort.check();

			if (m_position >= m_buffer_position && m_position < m_buffer_position + m_buffer_status) {				
				t_size delta = pfc::min_t<t_size>((t_size)(m_buffer_position + m_buffer_status - m_position),p_bytes - done);
				t_size bufptr = (t_size)(m_position - m_buffer_position);
				memcpy(outptr+done,m_buffer.get_ptr()+bufptr,delta);
				done += delta;
				m_position += delta;
				if (m_buffer_status != m_buffer.get_size() && done < p_bytes) break;//EOF before m_size is hit
			} else {
				m_buffer_position = m_position - m_position % m_buffer.get_size();
				baseSeek(m_buffer_position,p_abort);

				m_buffer_status = m_base->read(m_buffer.get_ptr(),m_buffer.get_size(),p_abort);
				m_position_base += m_buffer_status;

				if (m_buffer_status <= (t_size)(m_position - m_buffer_position)) break;
			}
		}

		return done;
	}

	void write(const void * p_buffer,t_size p_bytes,abort_callback & p_abort) {
		p_abort.check();
		baseSeek(m_position,p_abort);
		m_base->write(p_buffer,p_bytes,p_abort);
		m_position_base = m_position = m_position + p_bytes;
		if (m_size < m_position) m_size = m_position;
		flush_buffer();
	}

	t_filesize get_size(abort_callback & p_abort) {
		p_abort.check();
		return m_size;
	}
	t_filesize get_position(abort_callback & p_abort) {
		p_abort.check();
		return m_position;
	}
	void set_eof(abort_callback & p_abort) {
		p_abort.check();
		baseSeek(m_position,p_abort);
		m_base->set_eof(p_abort);
		flush_buffer();
	}
	void seek(t_filesize p_position,abort_callback & p_abort) {
		p_abort.check();
		if (!m_can_seek) throw exception_io_object_not_seekable();
		if (p_position > m_size) throw exception_io_seek_out_of_range();
		m_position = p_position;
	}
	void reopen(abort_callback & p_abort) {
		if (this->m_can_seek) {
			seek(0,p_abort);
		} else {
			this->m_base->reopen( p_abort );
			this->_reinit( p_abort );
		}
	}
	bool can_seek() {return m_can_seek;}
	bool get_content_type(pfc::string_base & out) {return m_base->get_content_type(out);}
	void on_idle(abort_callback & p_abort) {p_abort.check();m_base->on_idle(p_abort);}
	t_filetimestamp get_timestamp(abort_callback & p_abort) {p_abort.check(); return m_base->get_timestamp(p_abort);}
	bool is_remote() {return m_base->is_remote();}
	void resize(t_filesize p_size,abort_callback & p_abort) {
		flush_buffer();
		m_base->resize(p_size,p_abort);
		m_size = p_size;
		if (m_position > m_size) m_position = m_size;
		if (m_position_base > m_size) m_position_base = m_size;
	}
private:
	void baseSeek(t_filesize p_target,abort_callback & p_abort) {
		if (p_target != m_position_base) {
			m_base->seek(p_target,p_abort);
			m_position_base = p_target;
		}
	}

	void flush_buffer() {
		m_buffer_status = 0;
		m_buffer_position = 0;
	}

	service_ptr_t<file> m_base;
	t_filesize m_position,m_position_base,m_size;
	bool m_can_seek;
	t_filesize m_buffer_position;
	t_size m_buffer_status;
	pfc::array_t<t_uint8> m_buffer;
};

}

file::ptr file_cached::g_create(service_ptr_t<file> p_base,abort_callback & p_abort, t_size blockSize) {

	if (p_base->is_in_memory()) {
		return p_base; // do not want
	}

	{ // do not duplicate cache layers, check if the file we're being handed isn't already cached
		file_cached::ptr c;
		if (p_base->service_query_t(c)) {
			c->suggest_grow_cache(blockSize);
			return p_base;
		}
	}

	service_ptr_t<file_cached_impl_v2> temp = new service_impl_t<file_cached_impl_v2>(blockSize);
	temp->initialize(p_base,p_abort);
	return temp;
}

void file_cached::g_create(service_ptr_t<file> & p_out,service_ptr_t<file> p_base,abort_callback & p_abort, t_size blockSize) {
	p_out = g_create(p_base, p_abort, blockSize);
}

void file_cached::g_decodeInitCache(file::ptr & theFile, abort_callback & abort, size_t blockSize) {
	if (theFile->is_remote() || !theFile->can_seek()) return;

	g_create(theFile, theFile, abort, blockSize);
}
