#include "foobar2000.h"

bool tag_processor_id3v2::g_get(service_ptr_t<tag_processor_id3v2> & p_out)
{
	return service_enum_t<tag_processor_id3v2>().first(p_out);
}

void tag_processor_id3v2::g_remove(const service_ptr_t<file> & p_file,t_uint64 & p_size_removed,abort_callback & p_abort) {
    tag_write_callback_dummy cb;
	g_remove_ex(cb,p_file,p_size_removed,p_abort);
}

void tag_processor_id3v2::g_remove_ex(tag_write_callback & p_callback,const service_ptr_t<file> & p_file,t_uint64 & p_size_removed,abort_callback & p_abort)
{
	p_file->ensure_seekable();

	t_filesize len;
	
	len = p_file->get_size(p_abort);

	if (len == filesize_invalid) throw exception_io_no_length();
	
	p_file->seek(0,p_abort);
	
	t_uint64 offset;
	g_multiskip(p_file,offset,p_abort);
	
	if (offset>0 && offset<len)
	{
		len-=offset;
		service_ptr_t<file> temp;
		if (p_callback.open_temp_file(temp,p_abort)) {
			file::g_transfer_object(p_file,temp,len,p_abort);
		} else {
			if (len > 16*1024*1024) filesystem::g_open_temp(temp,p_abort);
			else filesystem::g_open_tempmem(temp,p_abort);
			file::g_transfer_object(p_file,temp,len,p_abort);
			p_file->seek(0,p_abort);
			p_file->set_eof(p_abort);
			temp->seek(0,p_abort);
			file::g_transfer_object(temp,p_file,len,p_abort);
		}
	}
	p_size_removed = offset;
}

t_size tag_processor_id3v2::g_multiskip(const service_ptr_t<file> & p_file,t_filesize & p_size_skipped,abort_callback & p_abort) {
	t_filesize offset = 0;
	t_size count = 0;
	for(;;) {
		t_filesize delta;
		g_skip_at(p_file, offset, delta, p_abort);
		if (delta == 0) break;
		offset += delta;
		++count;
	}
	p_size_skipped = offset;
	return count;
}
void tag_processor_id3v2::g_skip(const service_ptr_t<file> & p_file,t_uint64 & p_size_skipped,abort_callback & p_abort) {
	g_skip_at(p_file, 0, p_size_skipped, p_abort);
}

void tag_processor_id3v2::g_skip_at(const service_ptr_t<file> & p_file,t_filesize p_base, t_filesize & p_size_skipped,abort_callback & p_abort) {

	unsigned char  tmp[10];

	p_file->seek ( p_base, p_abort );

	if (p_file->read( tmp, sizeof(tmp),  p_abort) != sizeof(tmp))  {
		p_file->seek ( p_base, p_abort );
		p_size_skipped = 0;
		return;
	}

	if ( 
		0 != memcmp ( tmp, "ID3", 3) ||
		( tmp[5] & 0x0F ) != 0 || 
		((tmp[6] | tmp[7] | tmp[8] | tmp[9]) & 0x80) != 0 
		) {
		p_file->seek ( p_base, p_abort );
		p_size_skipped = 0;
		return;
	}

	int FooterPresent     = tmp[5] & 0x10;

	t_uint32 ret;
	ret  = tmp[6] << 21;
	ret += tmp[7] << 14;
	ret += tmp[8] <<  7;
	ret += tmp[9]      ;
	ret += 10;
	if ( FooterPresent ) ret += 10;

	try {
		p_file->seek ( p_base + ret, p_abort );
	} catch(exception_io_seek_out_of_range) {
		p_file->seek( p_base, p_abort );
		p_size_skipped = 0;
		return;
	}

	p_size_skipped = ret;

}
