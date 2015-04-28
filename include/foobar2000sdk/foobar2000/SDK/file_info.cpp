#include "foobar2000.h"

#ifndef _MSC_VER
#define strcat_s strcat
#define _atoi64 atoll
#endif

t_size file_info::meta_find_ex(const char * p_name,t_size p_name_length) const
{
	t_size n, m = meta_get_count();
	for(n=0;n<m;n++)
	{
		if (pfc::stricmp_ascii_ex(meta_enum_name(n),pfc_infinite,p_name,p_name_length) == 0) return n;
	}
	return pfc_infinite;
}

bool file_info::meta_exists_ex(const char * p_name,t_size p_name_length) const
{
	return meta_find_ex(p_name,p_name_length) != pfc_infinite;
}

void file_info::meta_remove_field_ex(const char * p_name,t_size p_name_length)
{
	t_size index = meta_find_ex(p_name,p_name_length);
	if (index!=pfc_infinite) meta_remove_index(index);
}


void file_info::meta_remove_index(t_size p_index)
{
	meta_remove_mask(bit_array_one(p_index));
}

void file_info::meta_remove_all()
{
	meta_remove_mask(bit_array_true());
}

void file_info::meta_remove_value(t_size p_index,t_size p_value)
{
	meta_remove_values(p_index,bit_array_one(p_value));
}

t_size file_info::meta_get_count_by_name_ex(const char * p_name,t_size p_name_length) const
{
	t_size index = meta_find_ex(p_name,p_name_length);
	if (index == pfc_infinite) return 0;
	return meta_enum_value_count(index);
}

t_size file_info::info_find_ex(const char * p_name,t_size p_name_length) const
{
	t_size n, m = info_get_count();
	for(n=0;n<m;n++) {
		if (pfc::stricmp_ascii_ex(info_enum_name(n),pfc_infinite,p_name,p_name_length) == 0) return n;
	}
	return pfc_infinite;
}

bool file_info::info_exists_ex(const char * p_name,t_size p_name_length) const
{
	return info_find_ex(p_name,p_name_length) != pfc_infinite;
}

void file_info::info_remove_index(t_size p_index)
{
	info_remove_mask(bit_array_one(p_index));
}

void file_info::info_remove_all()
{
	info_remove_mask(bit_array_true());
}

bool file_info::info_remove_ex(const char * p_name,t_size p_name_length)
{
	t_size index = info_find_ex(p_name,p_name_length);
	if (index != pfc_infinite)
	{
		info_remove_index(index);
		return true;
	}
	else return false;
}

void file_info::overwrite_meta(const file_info & p_source) {
	const t_size total = p_source.meta_get_count();
	for(t_size walk = 0; walk < total; ++walk) {
		copy_meta_single(p_source, walk);
	}
}

void file_info::copy_meta_single(const file_info & p_source,t_size p_index)
{
	copy_meta_single_rename(p_source,p_index,p_source.meta_enum_name(p_index));
}

void file_info::copy_meta_single_nocheck(const file_info & p_source,t_size p_index)
{
	const char * name = p_source.meta_enum_name(p_index);
	t_size n, m = p_source.meta_enum_value_count(p_index);
	t_size new_index = pfc_infinite;
	for(n=0;n<m;n++)
	{
		const char * value = p_source.meta_enum_value(p_index,n);
		if (n == 0) new_index = meta_set_nocheck(name,value);
		else meta_add_value(new_index,value);
	}
}

void file_info::copy_meta_single_by_name_ex(const file_info & p_source,const char * p_name,t_size p_name_length)
{
	t_size index = p_source.meta_find_ex(p_name,p_name_length);
	if (index != pfc_infinite) copy_meta_single(p_source,index);
}

void file_info::copy_info_single_by_name_ex(const file_info & p_source,const char * p_name,t_size p_name_length)
{
	t_size index = p_source.info_find_ex(p_name,p_name_length);
	if (index != pfc_infinite) copy_info_single(p_source,index);
}

void file_info::copy_meta_single_by_name_nocheck_ex(const file_info & p_source,const char * p_name,t_size p_name_length)
{
	t_size index = p_source.meta_find_ex(p_name,p_name_length);
	if (index != pfc_infinite) copy_meta_single_nocheck(p_source,index);
}

void file_info::copy_info_single_by_name_nocheck_ex(const file_info & p_source,const char * p_name,t_size p_name_length)
{
	t_size index = p_source.info_find_ex(p_name,p_name_length);
	if (index != pfc_infinite) copy_info_single_nocheck(p_source,index);
}

void file_info::copy_info_single(const file_info & p_source,t_size p_index)
{
	info_set(p_source.info_enum_name(p_index),p_source.info_enum_value(p_index));
}

void file_info::copy_info_single_nocheck(const file_info & p_source,t_size p_index)
{
	info_set_nocheck(p_source.info_enum_name(p_index),p_source.info_enum_value(p_index));
}

void file_info::copy_meta(const file_info & p_source)
{
	if (&p_source != this) {
		meta_remove_all();
		t_size n, m = p_source.meta_get_count();
		for(n=0;n<m;n++)
			copy_meta_single_nocheck(p_source,n);
	}
}

void file_info::copy_info(const file_info & p_source)
{
	if (&p_source != this) {
		info_remove_all();
		t_size n, m = p_source.info_get_count();
		for(n=0;n<m;n++)
			copy_info_single_nocheck(p_source,n);
	}
}

void file_info::copy(const file_info & p_source)
{
	if (&p_source != this) {
		copy_meta(p_source);
		copy_info(p_source);
		set_length(p_source.get_length());
		set_replaygain(p_source.get_replaygain());
	}
}


const char * file_info::meta_get_ex(const char * p_name,t_size p_name_length,t_size p_index) const
{
	t_size index = meta_find_ex(p_name,p_name_length);
	if (index == pfc_infinite) return 0;
	t_size max = meta_enum_value_count(index);
	if (p_index >= max) return 0;
	return meta_enum_value(index,p_index);
}

const char * file_info::info_get_ex(const char * p_name,t_size p_name_length) const
{
	t_size index = info_find_ex(p_name,p_name_length);
	if (index == pfc_infinite) return 0;
	return info_enum_value(index);
}

t_int64 file_info::info_get_int(const char * name) const
{
	PFC_ASSERT(pfc::is_valid_utf8(name));
	const char * val = info_get(name);
	if (val==0) return 0;
	return _atoi64(val);
}

t_int64 file_info::info_get_length_samples() const
{
	t_int64 ret = 0;
	double len = get_length();
	t_int64 srate = info_get_int("samplerate");

	if (srate>0 && len>0)
	{
		ret = audio_math::time_to_samples(len,(unsigned)srate);
	}
	return ret;
}

double file_info::info_get_float(const char * name) const
{
	const char * ptr = info_get(name);
	if (ptr) return pfc::string_to_float(ptr);
	else return 0;
}

void file_info::info_set_int(const char * name,t_int64 value)
{
	PFC_ASSERT(pfc::is_valid_utf8(name));
	info_set(name,pfc::format_int(value));
}

void file_info::info_set_float(const char * name,double value,unsigned precision,bool force_sign,const char * unit)
{
	PFC_ASSERT(pfc::is_valid_utf8(name));
	PFC_ASSERT(unit==0 || strlen(unit) <= 64);
	char temp[128];
	pfc::float_to_string(temp,64,value,precision,force_sign);
	temp[63] = 0;
	if (unit)
	{
		strcat_s(temp," ");
		strcat_s(temp,unit);
	}
	info_set(name,temp);
}


void file_info::info_set_replaygain_album_gain(float value)
{
	replaygain_info temp = get_replaygain();
	temp.m_album_gain = value;
	set_replaygain(temp);
}

void file_info::info_set_replaygain_album_peak(float value)
{
	replaygain_info temp = get_replaygain();
	temp.m_album_peak = value;
	set_replaygain(temp);
}

void file_info::info_set_replaygain_track_gain(float value)
{
	replaygain_info temp = get_replaygain();
	temp.m_track_gain = value;
	set_replaygain(temp);
}

void file_info::info_set_replaygain_track_peak(float value)
{
	replaygain_info temp = get_replaygain();
	temp.m_track_peak = value;
	set_replaygain(temp);
}


static bool is_valid_bps(t_int64 val)
{
	return val>0 && val<=256;
}

unsigned file_info::info_get_decoded_bps() const
{
	t_int64 val = info_get_int("decoded_bitspersample");
	if (is_valid_bps(val)) return (unsigned)val;
	val = info_get_int("bitspersample");
	if (is_valid_bps(val)) return (unsigned)val;
	return 0;

}

void file_info::reset()
{
	info_remove_all();
	meta_remove_all();
	set_length(0);
	reset_replaygain();
}

void file_info::reset_replaygain()
{
	replaygain_info temp;
	temp.reset();
	set_replaygain(temp);
}

void file_info::copy_meta_single_rename_ex(const file_info & p_source,t_size p_index,const char * p_new_name,t_size p_new_name_length)
{
	t_size n, m = p_source.meta_enum_value_count(p_index);
	t_size new_index = pfc_infinite;
	for(n=0;n<m;n++)
	{
		const char * value = p_source.meta_enum_value(p_index,n);
		if (n == 0) new_index = meta_set_ex(p_new_name,p_new_name_length,value,pfc_infinite);
		else meta_add_value(new_index,value);
	}
}

t_size file_info::meta_add_ex(const char * p_name,t_size p_name_length,const char * p_value,t_size p_value_length)
{
	t_size index = meta_find_ex(p_name,p_name_length);
	if (index == pfc_infinite) return meta_set_nocheck_ex(p_name,p_name_length,p_value,p_value_length);
	else
	{
		meta_add_value_ex(index,p_value,p_value_length);
		return index;
	}
}

void file_info::meta_add_value_ex(t_size p_index,const char * p_value,t_size p_value_length)
{
	meta_insert_value_ex(p_index,meta_enum_value_count(p_index),p_value,p_value_length);
}


t_size file_info::meta_calc_total_value_count() const
{
	t_size n, m = meta_get_count(), ret = 0;
	for(n=0;n<m;n++) ret += meta_enum_value_count(n);
	return ret;
}

bool file_info::info_set_replaygain_ex(const char * p_name,t_size p_name_len,const char * p_value,t_size p_value_len)
{
	replaygain_info temp = get_replaygain();
	if (temp.set_from_meta_ex(p_name,p_name_len,p_value,p_value_len))
	{
		set_replaygain(temp);
		return true;
	}
	else return false;
}

void file_info::info_set_replaygain_auto_ex(const char * p_name,t_size p_name_len,const char * p_value,t_size p_value_len)
{
	if (!info_set_replaygain_ex(p_name,p_name_len,p_value,p_value_len))
		info_set_ex(p_name,p_name_len,p_value,p_value_len);
}

bool replaygain_info::g_equal(const replaygain_info & item1,const replaygain_info & item2)
{
	return	item1.m_album_gain == item2.m_album_gain &&
			item1.m_track_gain == item2.m_track_gain &&
			item1.m_album_peak == item2.m_album_peak &&
			item1.m_track_peak == item2.m_track_peak;
}

bool file_info::are_meta_fields_identical(t_size p_index1,t_size p_index2) const
{
	const t_size count = meta_enum_value_count(p_index1);
	if (count != meta_enum_value_count(p_index2)) return false;
	t_size n;
	for(n=0;n<count;n++)
	{
		if (strcmp(meta_enum_value(p_index1,n),meta_enum_value(p_index2,n))) return false;
	}
	return true;
}


void file_info::meta_format_entry(t_size index, pfc::string_base & out, const char * separator) const {
	out.reset();
	t_size val, count = meta_enum_value_count(index);
	PFC_ASSERT( count > 0);
	for(val=0;val<count;val++)
	{
		if (val > 0) out += separator;
		out += meta_enum_value(index,val);
	}
}

bool file_info::meta_format(const char * p_name,pfc::string_base & p_out, const char * separator) const {
	p_out.reset();
	t_size index = meta_find(p_name);
	if (index == pfc_infinite) return false;
	meta_format_entry(index, p_out, separator);
	return true;
}

void file_info::info_calculate_bitrate(t_filesize p_filesize,double p_length)
{
	if (p_filesize > 0 && p_length > 0) info_set_bitrate((unsigned)floor((double)p_filesize * 8 / (p_length * 1000) + 0.5));
}

bool file_info::is_encoding_lossy() const {
	const char * encoding = info_get("encoding");
	if (encoding != NULL) {
		if (pfc::stricmp_ascii(encoding,"lossy") == 0 /*|| pfc::stricmp_ascii(encoding,"hybrid") == 0*/) return true;
	} else {
		//the old way
		//disabled: don't whine if we're not sure what we're dealing with - might be a file with info not-yet-loaded in oddball cases or a mod file
		//if (info_get("bitspersample") == NULL) return true;
	}
	return false;
}

bool file_info::g_is_meta_equal(const file_info & p_item1,const file_info & p_item2) {
	const t_size count = p_item1.meta_get_count();
	if (count != p_item2.meta_get_count()) {
		//uDebugLog() << "meta count mismatch";
		return false;
	}
	pfc::map_t<const char*,t_size,field_name_comparator> item2_meta_map;
	for(t_size n=0; n<count; n++) {
		item2_meta_map.set(p_item2.meta_enum_name(n),n);
	}
	for(t_size n1=0; n1<count; n1++) {
		t_size n2;
		if (!item2_meta_map.query(p_item1.meta_enum_name(n1),n2)) {
			//uDebugLog() << "item2 doesn't have " << p_item1.meta_enum_name(n1);
			return false;
		}
		t_size value_count = p_item1.meta_enum_value_count(n1);
		if (value_count != p_item2.meta_enum_value_count(n2)) {
			//uDebugLog() << "meta value count mismatch: " << p_item1.meta_enum_name(n1) << " : " << value_count << " vs " << p_item2.meta_enum_value_count(n2);
			return false;
		}
		for(t_size v = 0; v < value_count; v++) {
			if (strcmp(p_item1.meta_enum_value(n1,v),p_item2.meta_enum_value(n2,v)) != 0) {
				//uDebugLog() << "meta mismatch: " << p_item1.meta_enum_name(n1) << " : " << p_item1.meta_enum_value(n1,v) << " vs " << p_item2.meta_enum_value(n2,v);
				return false;
			}
		}
	}
	return true;
}

bool file_info::g_is_meta_equal_debug(const file_info & p_item1,const file_info & p_item2) {
	const t_size count = p_item1.meta_get_count();
	if (count != p_item2.meta_get_count()) {
		FB2K_DebugLog() << "meta count mismatch";
		return false;
	}
	pfc::map_t<const char*,t_size,field_name_comparator> item2_meta_map;
	for(t_size n=0; n<count; n++) {
		item2_meta_map.set(p_item2.meta_enum_name(n),n);
	}
	for(t_size n1=0; n1<count; n1++) {
		t_size n2;
		if (!item2_meta_map.query(p_item1.meta_enum_name(n1),n2)) {
			FB2K_DebugLog() << "item2 doesn't have " << p_item1.meta_enum_name(n1);
			return false;
		}
		t_size value_count = p_item1.meta_enum_value_count(n1);
		if (value_count != p_item2.meta_enum_value_count(n2)) {
			FB2K_DebugLog() << "meta value count mismatch: " << p_item1.meta_enum_name(n1) << " : " << (uint32_t)value_count << " vs " << (uint32_t)p_item2.meta_enum_value_count(n2);
			return false;
		}
		for(t_size v = 0; v < value_count; v++) {
			if (strcmp(p_item1.meta_enum_value(n1,v),p_item2.meta_enum_value(n2,v)) != 0) {
				FB2K_DebugLog() << "meta mismatch: " << p_item1.meta_enum_name(n1) << " : " << p_item1.meta_enum_value(n1,v) << " vs " << p_item2.meta_enum_value(n2,v);
				return false;
			}
		}
	}
	return true;
}

bool file_info::g_is_info_equal(const file_info & p_item1,const file_info & p_item2) {
	t_size count = p_item1.info_get_count();
	if (count != p_item2.info_get_count()) {
		//uDebugLog() << "info count mismatch";
		return false;
	}
	for(t_size n1=0; n1<count; n1++) {
		t_size n2 = p_item2.info_find(p_item1.info_enum_name(n1));
		if (n2 == pfc_infinite) {
			//uDebugLog() << "item2 does not have " << p_item1.info_enum_name(n1);
			return false;
		}
		if (strcmp(p_item1.info_enum_value(n1),p_item2.info_enum_value(n2)) != 0) {
			//uDebugLog() << "value mismatch: " << p_item1.info_enum_name(n1);
			return false;
		}
	}
	return true;
}

static bool is_valid_field_name_char(char p_char) {
	return p_char >= 32 && p_char < 127 && p_char != '=' && p_char != '%' && p_char != '<' && p_char != '>';
}

bool file_info::g_is_valid_field_name(const char * p_name,t_size p_length) {
	t_size walk;
	for(walk = 0; walk < p_length && p_name[walk] != 0; walk++) {
		if (!is_valid_field_name_char(p_name[walk])) return false;
	}
	return walk > 0;
}

void file_info::to_formatter(pfc::string_formatter& out) const {
	out << "File info dump:\n";
	if (get_length() > 0) out<< "Duration: " << pfc::format_time_ex(get_length(), 6) << "\n";
	pfc::string_formatter temp;
	for(t_size metaWalk = 0; metaWalk < meta_get_count(); ++metaWalk) {
		meta_format_entry(metaWalk, temp);
		out << "Meta: " << meta_enum_name(metaWalk) << " = " << temp << "\n";
	}
	for(t_size infoWalk = 0; infoWalk < info_get_count(); ++infoWalk) {
		out << "Info: " << info_enum_name(infoWalk) << " = " << info_enum_value(infoWalk) << "\n";
	}
}

void file_info::to_console() const {
	FB2K_console_formatter() << "File info dump:";
	if (get_length() > 0) FB2K_console_formatter() << "Duration: " << pfc::format_time_ex(get_length(), 6);
	pfc::string_formatter temp;
	for(t_size metaWalk = 0; metaWalk < meta_get_count(); ++metaWalk) {
		meta_format_entry(metaWalk, temp);
		FB2K_console_formatter() << "Meta: " << meta_enum_name(metaWalk) << " = " << temp;
	}
	for(t_size infoWalk = 0; infoWalk < info_get_count(); ++infoWalk) {
		FB2K_console_formatter() << "Info: " << info_enum_name(infoWalk) << " = " << info_enum_value(infoWalk);
	}
}

void file_info::info_set_wfx_chanMask(uint32_t val) {
	switch(val) {
	case 0:
	case 4:
	case 3:
		break;
	default:
		info_set ("WAVEFORMATEXTENSIBLE_CHANNEL_MASK", PFC_string_formatter() << "0x" << pfc::format_hex(val) );
		break;
	}
}

uint32_t file_info::info_get_wfx_chanMask() const {
	const char * str = this->info_get("WAVEFORMATEXTENSIBLE_CHANNEL_MASK");
	if (str == NULL) return 0;
	if (pfc::strcmp_partial( str, "0x") != 0) return 0;
	try {
		return pfc::atohex<uint32_t>( str + 2, strlen(str+2) );
	} catch(...) { return 0;}
}

bool file_info::field_is_person(const char * fieldName) {
	return field_name_equals(fieldName, "artist") ||
		field_name_equals(fieldName, "album artist") || 
		field_name_equals(fieldName, "composer") || 
		field_name_equals(fieldName, "performer") ||
		field_name_equals(fieldName, "conductor") ||
		field_name_equals(fieldName, "orchestra") ||
		field_name_equals(fieldName, "ensemble") ||
		field_name_equals(fieldName, "engineer");
}

bool file_info::field_is_title(const char * fieldName) {
	return field_name_equals(fieldName, "title") || field_name_equals(fieldName, "album");
}


void file_info::to_stream( stream_writer * stream, abort_callback & abort ) const {
	stream_writer_formatter<> out(* stream, abort );
	
	out << this->get_length();
	
	{
		const auto rg = this->get_replaygain();
		out << rg.m_track_gain << rg.m_album_gain << rg.m_track_peak << rg.m_album_peak;
	}

	
	{
		const uint32_t metaCount = pfc::downcast_guarded<uint32_t>( this->meta_get_count() );
		for(uint32_t metaWalk = 0; metaWalk < metaCount; ++metaWalk) {
			const char * name = this->meta_enum_name( metaWalk );
			if (*name) {
				out.write_string_nullterm( this->meta_enum_name( metaWalk ) );
				const size_t valCount = this->meta_enum_value_count( metaWalk );
				for(size_t valWalk = 0; valWalk < valCount; ++valWalk) {
					const char * value = this->meta_enum_value( metaWalk, valWalk );
					if (*value) {
						out.write_string_nullterm( value );
					}
				}
				out.write_int<char>(0);
			}
		}
		out.write_int<char>(0);
	}

	{
		const uint32_t infoCount = pfc::downcast_guarded<uint32_t>( this->info_get_count() );
		for(uint32_t infoWalk = 0; infoWalk < infoCount; ++infoWalk) {
			const char * name = this->info_enum_name( infoWalk );
			const char * value = this->info_enum_value( infoWalk );
			if (*name && *value) {
				out.write_string_nullterm(name); out.write_string_nullterm(value);
			}
		}
		out.write_int<char>(0);
	}
}

void file_info::from_stream( stream_reader * stream, abort_callback & abort ) {
	stream_reader_formatter<> in( *stream, abort );
	pfc::string_formatter tempName, tempValue;
	{
		double len; in >> len; this->set_length( len );
	}
	{
		replaygain_info rg;
		in >> rg.m_track_gain >> rg.m_album_gain >> rg.m_track_peak >> rg.m_album_peak;
	}

	{
		this->meta_remove_all();
		for(;;) {
			in.read_string_nullterm( tempName );
			if (tempName.length() == 0) break;
			size_t metaIndex = pfc_infinite;
			for(;;) {
				in.read_string_nullterm( tempValue );
				if (tempValue.length() == 0) break;
				if (metaIndex == pfc_infinite) metaIndex = this->meta_add( tempName, tempValue );
				else this->meta_add_value( metaIndex, tempValue );
			}
		}
	}
	{
		this->info_remove_all();
		for(;;) {
			in.read_string_nullterm( tempName );
			if (tempName.length() == 0) break;
			in.read_string_nullterm( tempValue );
			this->info_set( tempName, tempValue );
		}
	}
}

static const char * _readString( const uint8_t * & ptr, size_t & remaining ) {
	const char * rv = (const char*)ptr;
	for(;;) {
		if (remaining == 0) throw exception_io_data();
		uint8_t byte = *ptr++; --remaining;
		if (byte == 0) break;
	}
	return rv;
}

template<typename int_t> void _readInt( int_t & out, const uint8_t * &ptr, size_t & remaining) {
	if (remaining < sizeof(out)) throw exception_io_data();
	pfc::decode_little_endian( out, ptr ); ptr += sizeof(out); remaining -= sizeof(out);
}

template<typename float_t> static void _readFloat(float_t & out, const uint8_t * &ptr, size_t & remaining) {
	union {
		typename pfc::sized_int_t<sizeof(float_t)>::t_unsigned i;
		float_t f;
	} u;
	_readInt(u.i, ptr, remaining);
	out = u.f;
}

void file_info::from_mem( const void * memPtr, size_t memSize ) {
	size_t remaining = memSize;
	const uint8_t * walk = (const uint8_t*) memPtr;

	{
		double len; _readFloat(len, walk, remaining);
		this->set_length( len );
	}

	{
		replaygain_info rg;
		_readFloat(rg.m_track_gain, walk, remaining ); 
		_readFloat(rg.m_album_gain, walk, remaining );
		_readFloat(rg.m_track_peak, walk, remaining );
		_readFloat(rg.m_album_peak, walk, remaining );
		this->set_replaygain( rg );
	}

	{
		this->meta_remove_all();
		for(;;) {
			const char * metaName = _readString( walk, remaining );
			if (*metaName == 0) break;
			size_t metaIndex = pfc_infinite;
			for(;;) {
				const char * metaValue = _readString( walk, remaining );
				if (*metaValue == 0) break;
				if (metaIndex == pfc_infinite) metaIndex = this->meta_add( metaName, metaValue );
				else this->meta_add_value( metaIndex, metaName );
			}
		}
	}
	{
		this->info_remove_all();
		for(;;) {
			const char * infoName = _readString( walk, remaining );
			if (*infoName == 0) break;
			const char * infoValue = _readString( walk, remaining );
			this->info_set( infoName, infoValue );
		}
	}
}

audio_chunk::spec_t file_info::audio_chunk_spec() const 
{
	audio_chunk::spec_t rv = {};
	rv.sampleRate = (uint32_t)this->info_get_int("samplerate");
	rv.chanCount = (uint32_t)this->info_get_int("channels");
	rv.chanMask = (uint32_t)this->info_get_wfx_chanMask();
	if (audio_chunk::g_count_channels( rv.chanMask ) != rv.chanCount ) {
		rv.chanMask = audio_chunk::g_guess_channel_config( rv.chanCount );
	}
	return rv;
}