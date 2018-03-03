#pragma once

namespace pfc {

	typedef list_base_const_t<const char*> string_list_const;

	class string_list_impl : public string_list_const
	{
	public:
		t_size get_count() const {return m_data.get_size();}
		void get_item_ex(const char* & p_out, t_size n) const {p_out = m_data[n];}

		const char * operator[] (t_size n) const {return m_data[n];}
		void add_item(const char * p_string) {pfc::append_t(m_data, p_string);}

		template<typename t_what> void add_items(const t_what & p_source) {_append(p_source);}

		void remove_all() {m_data.set_size(0);}

		string_list_impl() {}
		template<typename t_what> string_list_impl(const t_what & p_source) {_copy(p_source);}
		template<typename t_what> string_list_impl & operator=(const t_what & p_source) {_copy(p_source); return *this;}
		template<typename t_what> string_list_impl & operator|=(const string_list_impl & p_source) {_append(p_source); return *this;}
		template<typename t_what> string_list_impl & operator+=(const t_what & p_source) {pfc::append_t(m_data, p_source); return *this;}

	private:
		template<typename t_what> void _append(const t_what & p_source) {
			const t_size toadd = p_source.get_size(), base = m_data.get_size();
			m_data.set_size(base+toadd);
			for(t_size n=0;n<toadd;n++) m_data[base+n] = p_source[n];
		}

		template<typename t_what> void _copy(const t_what & p_source) {
			const t_size newcount = p_source.get_size();
			m_data.set_size(newcount);
			for(t_size n=0;n<newcount;n++) m_data[n] = p_source[n];
		}

		pfc::array_t<pfc::string8,pfc::alloc_fast> m_data;
	};
}
