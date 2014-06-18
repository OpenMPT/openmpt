#include "stdafx.h"

bool t_cuesheet_index_list::is_valid() const {
	if (m_positions[1] < m_positions[0]) return false;
	for(t_size n = 2; n < count && m_positions[n] > 0; n++) {
		if (m_positions[n] < m_positions[n-1]) return false;
	}
	return true;
}

void t_cuesheet_index_list::to_infos(file_info & p_out) const
{
	double base = m_positions[1];

	if (base > 0) {
		p_out.info_set("referenced_offset",cuesheet_format_index_time(base));
	}
	
	if (m_positions[0] < base)
		p_out.info_set("pregap",cuesheet_format_index_time(base - m_positions[0]));
	else
		p_out.info_remove("pregap");

	p_out.info_remove("index 00");	
	p_out.info_remove("index 01");
	
	for(unsigned n=2;n<count;n++)
	{
		char namebuffer[16];
		sprintf_s(namebuffer,"index %02u",n);
		double position = m_positions[n] - base;
		if (position > 0)
			p_out.info_set(namebuffer,cuesheet_format_index_time(position));
		else
			p_out.info_remove(namebuffer);
	}
}

static bool parse_value(const char * p_name,double & p_out)
{
	if (p_name == NULL) return false;
	try {
		p_out = cuesheet_parse_index_time_e(p_name,strlen(p_name));
	} catch(std::exception const &) {return false;}
	return true;
}

bool t_cuesheet_index_list::from_infos(file_info const & p_in,double p_base)
{
	double pregap;
	bool found = false;
	if (!parse_value(p_in.info_get("pregap"),pregap)) pregap = 0;
	else found = true;
	m_positions[0] = p_base - pregap;
	m_positions[1] = p_base;
	for(unsigned n=2;n<count;n++)
	{
		char namebuffer[16];
		sprintf_s(namebuffer,"index %02u",n);
		double temp;
		if (parse_value(p_in.info_get(namebuffer),temp)) {
			m_positions[n] = temp + p_base; found = true;
		} else {
			m_positions[n] = 0;
		}
	}
	return found;	
}

bool t_cuesheet_index_list::is_empty() const {
	for(unsigned n=0;n<count;n++) if (m_positions[n] != m_positions[1]) return false;
	return true;
}