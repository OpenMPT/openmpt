#include "foobar2000.h"

void stream_writer_chunk::write(const void * p_buffer,t_size p_bytes,abort_callback & p_abort) {
	t_size remaining = p_bytes, written = 0;
	while(remaining > 0) {
		t_size delta = sizeof(m_buffer) - m_buffer_state;
		if (delta > remaining) delta = remaining;
		memcpy(m_buffer,(const t_uint8*)p_buffer + written,delta);
		written += delta;
		remaining -= delta;

		if (m_buffer_state == sizeof(m_buffer)) {
			m_writer->write_lendian_t((t_uint8)m_buffer_state,p_abort);
			m_writer->write_object(m_buffer,m_buffer_state,p_abort);
			m_buffer_state = 0;
		}
	}
}

void stream_writer_chunk::flush(abort_callback & p_abort)
{
	m_writer->write_lendian_t((t_uint8)m_buffer_state,p_abort);
	if (m_buffer_state > 0) {
		m_writer->write_object(m_buffer,m_buffer_state,p_abort);
		m_buffer_state = 0;
	}
}
	
/*
	stream_writer * m_writer;
	unsigned m_buffer_state;
	unsigned char m_buffer[255];
*/

t_size stream_reader_chunk::read(void * p_buffer,t_size p_bytes,abort_callback & p_abort)
{
	t_size todo = p_bytes, done = 0;
	while(todo > 0) {
		if (m_buffer_size == m_buffer_state) {
			if (m_eof) break;
			t_uint8 temp;
			m_reader->read_lendian_t(temp,p_abort);
			m_buffer_size = temp;
			if (temp != sizeof(m_buffer)) m_eof = true;
			m_buffer_state = 0;
			if (m_buffer_size>0) {
				m_reader->read_object(m_buffer,m_buffer_size,p_abort);
			}
		}


		t_size delta = m_buffer_size - m_buffer_state;
		if (delta > todo) delta = todo;
		if (delta > 0) {
			memcpy((unsigned char*)p_buffer + done,m_buffer + m_buffer_state,delta);
			todo -= delta;
			done += delta;
			m_buffer_state += delta;
		}
	}
	return done;
}

void stream_reader_chunk::flush(abort_callback & p_abort) {
	while(!m_eof) {
		p_abort.check_e();
		t_uint8 temp;
		m_reader->read_lendian_t(temp,p_abort);
		m_buffer_size = temp;
		if (temp != sizeof(m_buffer)) m_eof = true;
		m_buffer_state = 0;
		if (m_buffer_size>0) {
			m_reader->skip_object(m_buffer_size,p_abort);
		}
	}
}

/*
	stream_reader * m_reader;
	unsigned m_buffer_state, m_buffer_size;
	bool m_eof;
	unsigned char m_buffer[255];
*/

void stream_reader_chunk::g_skip(stream_reader * p_stream,abort_callback & p_abort) {
	stream_reader_chunk(p_stream).flush(p_abort);
}



static void fileSanitySeek(file::ptr f, pfc::array_t<uint8_t> const & content, size_t offset, abort_callback & aborter) {
	const size_t readAmount = 64 * 1024;
	pfc::array_staticsize_t<uint8_t> buf; buf.set_size_discard(readAmount);
	f->seek(offset, aborter);
	t_filesize positionGot = f->get_position(aborter);
	if (positionGot != offset) {
		FB2K_console_formatter() << "File sanity: at " << offset << " reported position became " << positionGot;
		throw std::runtime_error("Seek test failure");
	}
	size_t did = f->read(buf.get_ptr(), readAmount, aborter);
	size_t expected = pfc::min_t<size_t>(readAmount, content.get_size() - offset);
	if (expected != did) {
		FB2K_console_formatter() << "File sanity: at " << offset << " bytes, expected read size of " << expected << ", got " << did;
		if (did > expected) FB2K_console_formatter() << "Read past EOF";
		else  FB2K_console_formatter() << "Premature EOF";
		throw std::runtime_error("Seek test failure");
	}
	if (memcmp(buf.get_ptr(), content.get_ptr() + offset, did) != 0) {
		FB2K_console_formatter() << "File sanity: data mismatch at " << offset << " - " << (offset + did) << " bytes";
		throw std::runtime_error("Seek test failure");
	}
	positionGot = f->get_position(aborter);
	if (positionGot != offset + did) {
		FB2K_console_formatter() << "File sanity: at " << offset << "+" << did << "=" << (offset + did) << " reported position became " << positionGot;
		throw std::runtime_error("Seek test failure");
	}
}

bool fb2kFileSelfTest(file::ptr f, abort_callback & aborter) {
	try {
		pfc::array_t<uint8_t> fileContent;
		f->reopen(aborter);
		f->read_till_eof(fileContent, aborter);

		{
			t_filesize sizeClaimed = f->get_size(aborter);
			if (sizeClaimed == filesize_invalid) {
				FB2K_console_formatter() << "File sanity: file reports unknown size, actual size read is " << fileContent.get_size();
			}
			else {
				if (sizeClaimed != fileContent.get_size()) {
					FB2K_console_formatter() << "File sanity: file reports size of " << sizeClaimed << ", actual size read is " << fileContent.get_size();
					throw std::runtime_error("File size mismatch");
				}
				else {
					FB2K_console_formatter() << "File sanity: file size check OK: " << sizeClaimed;
				}
			}
		}

		{
			FB2K_console_formatter() << "File sanity: testing N-first-bytes reads...";
			const size_t sizeUpTo = pfc::min_t<size_t>(fileContent.get_size(), 1024 * 1024);
			pfc::array_staticsize_t<uint8_t> buf1;
			buf1.set_size_discard(sizeUpTo);

			for (size_t w = 1; w <= sizeUpTo; w <<= 1) {
				f->reopen(aborter);
				size_t did = f->read(buf1.get_ptr(), w, aborter);
				if (did != w) {
					FB2K_console_formatter() << "File sanity: premature EOF reading first " << w << " bytes, got " << did;
					throw std::runtime_error("Premature EOF");
				}
				if (memcmp(fileContent.get_ptr(), buf1.get_ptr(), did) != 0) {
					FB2K_console_formatter() << "File sanity: file content mismatch reading first " << w << " bytes";
					throw std::runtime_error("File content mismatch");
				}
			}
		}
		if (f->can_seek()) {
			FB2K_console_formatter() << "File sanity: testing random access...";

			{
				size_t sizeUpTo = pfc::min_t<size_t>(fileContent.get_size(), 1024 * 1024);
				for (size_t w = 1; w < sizeUpTo; w <<= 1) {
					fileSanitySeek(f, fileContent, w, aborter);
				}
				fileSanitySeek(f, fileContent, fileContent.get_size(), aborter);
				for (size_t w = 1; w < sizeUpTo; w <<= 1) {
					fileSanitySeek(f, fileContent, fileContent.get_size() - w, aborter);
				}
				fileSanitySeek(f, fileContent, fileContent.get_size() / 2, aborter);
			}
		}
		FB2K_console_formatter() << "File sanity test: all OK";
		return true;
	}
	catch (std::exception const & e) {
		FB2K_console_formatter() << "File sanity test failure: " << e.what();
		return false;
	}
}



namespace {
    class listDirectoryCallback : public directory_callback {
    public:
        bool on_entry(filesystem * p_owner,abort_callback & p_abort,const char * p_url,bool p_is_subdirectory,const t_filestats & p_stats) {
            m_func( p_url, p_stats, p_is_subdirectory );
            return true;
        }
        listDirectoryFunc_t m_func;
    };

}
namespace foobar2000_io {
    void listDirectory( const char * path, abort_callback & aborter, listDirectoryFunc_t func) {
        listDirectoryCallback cb; cb.m_func = func;
        filesystem::g_list_directory(path, cb, aborter);
    }

	pfc::string8 stripParentFolders( const char * inPath ) {
		size_t prefixLen = pfc::string_find_first(inPath, "://");
		if ( prefixLen != pfc_infinite ) prefixLen += 3;
		else prefixLen = 0;

		pfc::chain_list_v2_t<pfc::string_part_ref> segments;
		pfc::splitStringByChar(segments, inPath + prefixLen, '\\' );
		for ( auto i = segments.first(); i.is_valid(); ) {
			auto n = i; ++n;
			if ( i->equals( "." ) ) {
				segments.remove_single( i );
			} else if ( i->equals( ".." ) ) {
				auto p = i; --p;
				if ( p.is_valid() ) segments.remove_single( p );
				segments.remove_single( i );
			}
			i = n;
		}
		pfc::string8 ret;
		if ( prefixLen > 0 ) ret.add_string( inPath, prefixLen );
		bool bFirst = true;
		for ( auto i = segments.first(); i.is_valid(); ++ i ) {
			if (!bFirst) ret << "\\";
			ret << *i;		
			bFirst = false;
		}
		return ret;
	}

}
