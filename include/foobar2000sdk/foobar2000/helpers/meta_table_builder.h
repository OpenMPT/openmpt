class _meta_table_enum_wrapper {
public:
	_meta_table_enum_wrapper(file_info & p_info) : m_info(p_info) {}
	template<typename t_values>
	void operator() (const char * p_name,const t_values & p_values) {
		t_size index = ~0;
		for(typename t_values::const_iterator iter = p_values.first(); iter.is_valid(); ++iter) {
			if (index == ~0) index = m_info.__meta_add_unsafe(p_name,*iter);
			else m_info.meta_add_value(index,*iter);
		}
	}
private:
	file_info & m_info;
};

class _meta_table_enum_wrapper_RG {
public:
	_meta_table_enum_wrapper_RG(file_info & p_info) : m_info(p_info) {}
	template<typename t_values>
	void operator() (const char * p_name,const t_values & p_values) {
		if (p_values.get_count() > 0) {
			if (!m_info.info_set_replaygain(p_name, *p_values.first())) {
				t_size index = ~0;
				for(typename t_values::const_iterator iter = p_values.first(); iter.is_valid(); ++iter) {
					if (index == ~0) index = m_info.__meta_add_unsafe(p_name,*iter);
					else m_info.meta_add_value(index,*iter);
				}
			}
		}
	}
private:
	file_info & m_info;
};

//! Purpose: building a file_info metadata table from loose input without search-for-existing-entry bottleneck
class meta_table_builder {
public:
	typedef pfc::chain_list_v2_t<pfc::string8> t_entry;
	typedef pfc::map_t<pfc::string8,t_entry,file_info::field_name_comparator> t_content;

	t_content & content() {return m_data;}
	t_content const & content() const {return m_data;}

	void add(const char * p_name,const char * p_value,t_size p_value_len = ~0) {
		if (file_info::g_is_valid_field_name(p_name)) {
			_add(p_name).insert_last()->set_string(p_value,p_value_len);
		}
	}

	void remove(const char * p_name) {
		m_data.remove(p_name);
	}
	void set(const char * p_name,const char * p_value,t_size p_value_len = ~0) {
		if (file_info::g_is_valid_field_name(p_name)) {
			t_entry & entry = _add(p_name);
			entry.remove_all();
			entry.insert_last()->set_string(p_value,p_value_len);
		}
	}
	t_entry & add(const char * p_name) {
		if (!file_info::g_is_valid_field_name(p_name)) uBugCheck();//we return a reference, nothing smarter to do
		return _add(p_name);
	}
	void deduplicate(const char * name) {
		t_entry * e;
		if (!m_data.query_ptr(name, e)) return;
		pfc::avltree_t<const char*, pfc::comparator_strcmp> found;
		for(t_entry::iterator iter = e->first(); iter.is_valid(); ) {
			t_entry::iterator next = iter; ++next;
			const char * v = *iter;
			if (!found.add_item_check(v)) e->remove(iter);
			iter = next;
		}
	}
	void keep_one(const char * name) {
		t_entry * e;
		if (!m_data.query_ptr(name, e)) return;
		while(e->get_count() > 1) e->remove(e->last());
	}
	void tidy_VorbisComment() {
		deduplicate("album artist");
		deduplicate("publisher");
		keep_one("totaltracks");
		keep_one("totaldiscs");
	}
	void finalize(file_info & p_info) const {
		p_info.meta_remove_all();
        _meta_table_enum_wrapper e(p_info);
		m_data.enumerate(e);
	}
	void finalize_withRG(file_info & p_info) const {
		p_info.meta_remove_all(); p_info.set_replaygain(replaygain_info_invalid);
        _meta_table_enum_wrapper_RG e(p_info);
		m_data.enumerate(e);
	}

	void from_info(const file_info & p_info) {
		m_data.remove_all();
		from_info_overwrite(p_info);
	}
	void from_info_withRG(const file_info & p_info) {
		m_data.remove_all();
		from_info_overwrite(p_info);
		from_RG_overwrite(p_info.get_replaygain());
	}
	void from_RG_overwrite(replaygain_info info) {
		replaygain_info::t_text_buffer buffer;
		if (info.format_album_gain(buffer)) set("replaygain_album_gain", buffer);
		if (info.format_track_gain(buffer)) set("replaygain_track_gain", buffer);
		if (info.format_album_peak(buffer)) set("replaygain_album_peak", buffer);
		if (info.format_track_peak(buffer)) set("replaygain_track_peak", buffer);
	}
	void from_info_overwrite(const file_info & p_info) {
		for(t_size metawalk = 0, metacount = p_info.meta_get_count(); metawalk < metacount; ++metawalk ) {
			const t_size valuecount = p_info.meta_enum_value_count(metawalk);
			if (valuecount > 0) {
				t_entry & entry = add(p_info.meta_enum_name(metawalk));
				entry.remove_all();
				for(t_size valuewalk = 0; valuewalk < valuecount; ++valuewalk) {
					entry.insert_last(p_info.meta_enum_value(metawalk,valuewalk));
				}
			}
		}
	}
	void reset() {m_data.remove_all();}

	void fix_itunes_compilation() {
		static const char cmp[] = "itunescompilation";
		if (m_data.have_item(cmp)) {
			// m_data.remove(cmp);
			if (!m_data.have_item("album artist")) add("album artist", "Various Artists");
		}
	}
private:

	t_entry & _add(const char * p_name) {
		return m_data.find_or_add(p_name);
	}

	t_content m_data;
};
