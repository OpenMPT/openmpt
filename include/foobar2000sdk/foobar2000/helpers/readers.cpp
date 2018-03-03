#include "StdAfx.h"
#include "readers.h"
#include "fullFileBuffer.h"
#include "fileReadAhead.h"

t_size reader_membuffer_base::read(void * p_buffer, t_size p_bytes, abort_callback & p_abort) {
	p_abort.check_e();
	t_size max = get_buffer_size();
	if (max < m_offset) uBugCheck();
	max -= m_offset;
	t_size delta = p_bytes;
	if (delta > max) delta = max;
	memcpy(p_buffer, (char*)get_buffer() + m_offset, delta);
	m_offset += delta;
	return delta;
}

void reader_membuffer_base::seek(t_filesize position, abort_callback & p_abort) {
	p_abort.check_e();
	t_filesize max = get_buffer_size();
	if (position == filesize_invalid || position > max) throw exception_io_seek_out_of_range();
	m_offset = (t_size)position;
}




file::ptr fullFileBuffer::open(const char * path, abort_callback & abort, file::ptr hint, t_filesize sizeMax) {
	//mutexScope scope(hMutex, abort);

	file::ptr f;
	if (hint.is_valid()) f = hint;
	else filesystem::g_open_read(f, path, abort);
	t_filesize fs = f->get_size(abort);
	if (fs < sizeMax) /*rejects size-unknown too*/ {
		try {
			service_ptr_t<reader_bigmem_mirror> r = new service_impl_t<reader_bigmem_mirror>();
			r->init(f, abort);
			f = r;
		}
		catch (std::bad_alloc) {}
	}
	return f;
}










#include <memory>
#include "rethrow.h"
#include <pfc/synchro.h>
#include <pfc/threads.h>

namespace {
	struct readAheadInstance_t {
		file::ptr m_file;
		size_t m_readAhead;

		pfc::array_t<uint8_t> m_buffer;
		size_t m_bufferBegin, m_bufferEnd;
		pfc::event m_canRead, m_canWrite;
		pfc::mutex m_guard;
		ThreadUtils::CRethrow m_error;
		t_filesize m_seekto;
		abort_callback_impl m_abort;
	};
	typedef std::shared_ptr<readAheadInstance_t> readAheadInstanceRef;
	static const t_filesize seek_reopen = (filesize_invalid-1);
	class fileReadAhead : public file_readonly_t<file> {
	public:
		readAheadInstanceRef m_instance;
		~fileReadAhead() {
			if ( m_instance ) {
				auto & i = *m_instance;
				pfc::mutexScope guard( i.m_guard );
				i.m_abort.set();
				i.m_canWrite.set_state(true);
			}
		}
		void initialize( file::ptr chain, size_t readAhead, abort_callback & aborter ) {
			m_remote = chain->is_remote();
			m_stats = chain->get_stats( aborter );
			if (!chain->get_content_type(m_contentType)) m_contentType = "";
			m_canSeek = chain->can_seek();
			m_position = chain->get_position( aborter );

			auto i = std::make_shared<readAheadInstance_t>();;
			i->m_file = chain;
			i->m_readAhead = readAhead;
			i->m_buffer.set_size_discard( readAhead * 2 );
			i->m_bufferBegin = 0; i->m_bufferEnd = 0;
			i->m_canWrite.set_state(true);
			i->m_seekto = filesize_invalid;
			m_instance = i;
			pfc::splitThread( [i] {
				worker(*i);
			} );
		}
		static void waitHelper( pfc::event & evt, abort_callback & aborter ) {
			if (pfc::event::g_twoEventWait( evt.get_handle(), aborter.get_abort_event(), -1) == 2) throw exception_aborted();
		}
		t_size read(void * p_buffer,t_size p_bytes,abort_callback & p_abort) {
			auto & i = * m_instance;
			size_t done = 0;
			while( done < p_bytes ) {
				waitHelper( i.m_canRead, p_abort );
				pfc::mutexScope guard ( i.m_guard );
				size_t got = i.m_bufferEnd - i.m_bufferBegin;
				if (got == 0) {
					i.m_error.rethrow();
					break; // EOF
				}

				size_t delta = pfc::min_t<size_t>( p_bytes - done, got );

				auto bufptr = i.m_buffer.get_ptr();
				memcpy( (uint8_t*) p_buffer + done, bufptr + i.m_bufferBegin, delta );
				done += delta;
				i.m_bufferBegin += delta;
				got -= delta;
				m_position += delta;

				if (!i.m_error.didFail()) {
					if ( got == 0 ) i.m_canRead.set_state( false );
					if ( got < 3 * i.m_readAhead / 4 ) i.m_canWrite.set_state( true );
				}

			}
			return done;
		}
		t_filesize get_size(abort_callback & p_abort) {
			p_abort.check();
			return m_stats.m_size;
		}
		t_filesize get_position(abort_callback & p_abort) {
			p_abort.check();
			return m_position;
		}

		void seek(t_filesize p_position,abort_callback & p_abort) {
			p_abort.check();
			if (!m_canSeek) throw exception_io_object_not_seekable();
			if ( m_stats.m_size != filesize_invalid && p_position > m_stats.m_size ) throw exception_io_seek_out_of_range();

			seekInternal( p_position );
		}
		bool can_seek() {
			return m_canSeek;
		}
		bool get_content_type(pfc::string_base & p_out) {
			if (m_contentType.length() == 0) return false;
			p_out = m_contentType; return true;
		}
		t_filestats get_stats( abort_callback & p_abort ) {
			p_abort.check();
			return m_stats;
				
		}
		t_filetimestamp get_timestamp(abort_callback & p_abort) {
			p_abort.check();
			return m_stats.m_timestamp;
		}
		bool is_remote() {
			return m_remote;
		}

		void reopen( abort_callback & p_abort ) {
			p_abort.check();
			seekInternal( seek_reopen );
		}

	private:
		void seekInternal( t_filesize p_position ) {
			auto & i = * m_instance;
			insync( i.m_guard );
			i.m_error.rethrow();
			i.m_bufferBegin = i.m_bufferEnd = 0;
			i.m_canWrite.set_state(true);
			i.m_seekto = p_position;
			i.m_canRead.set_state(false);

			m_position = ( p_position == seek_reopen ) ? 0 : p_position;
		}
		static void worker( readAheadInstance_t & i ) {
			ThreadUtils::CRethrow err;
			err.exec( [&i] {
				uint8_t* bufptr = i.m_buffer.get_ptr();
				for ( ;; ) {
					i.m_canWrite.wait_for(-1);
					size_t readHowMuch = 0, readOffset = 0;
					{
						pfc::mutexScope guard(i.m_guard);
						i.m_abort.check();
						if ( i.m_seekto != filesize_invalid ) {
							if ( i.m_seekto == seek_reopen ) {
								i.m_file->reopen( i.m_abort );
							} else {
								i.m_file->seek( i.m_seekto, i.m_abort );
							}

							i.m_seekto = filesize_invalid;
						}
						size_t got = i.m_bufferEnd - i.m_bufferBegin;

						if ( i.m_bufferBegin >= i.m_readAhead ) {
							memmove( bufptr, bufptr + i.m_bufferBegin, got );
							i.m_bufferBegin = 0;
							i.m_bufferEnd = got;
						}

						if ( got < i.m_readAhead ) {
							readHowMuch = i.m_readAhead - got;
							readOffset = i.m_bufferEnd;
						}
					}

					if ( readHowMuch > 0 ) {
						readHowMuch = i.m_file->read( bufptr + readOffset, readHowMuch, i.m_abort );
					}

					{
						pfc::mutexScope guard( i.m_guard );
						if ( i.m_seekto != filesize_invalid ) {
							// Seek request happened while we were reading - discard and continue
							continue; 
						}
						i.m_canRead.set_state( true );
						i.m_bufferEnd += readHowMuch;
						size_t got = i.m_bufferEnd - i.m_bufferBegin;
						if ( got >= i.m_readAhead ) i.m_canWrite.set_state(false);
					}
				}
			} );

			if ( err.didFail( ) ) {
				pfc::mutexScope guard( i.m_guard );
				i.m_error = err;
				i.m_canRead.set_state(true);
			}
		}

		bool m_remote, m_canSeek;
		t_filestats m_stats;
		pfc::string8 m_contentType;
		t_filesize m_position;
	};

}


file::ptr fileCreateReadAhead(file::ptr chain, size_t readAheadBytes, abort_callback & aborter ) {
	service_ptr_t<fileReadAhead> obj = new service_impl_t<fileReadAhead>();
	obj->initialize( chain, readAheadBytes, aborter );
	return obj;
}
