#pragma once

// Alternate advconfig var implementations that discard their state across app restarts - use for debug options that should not stick

template<bool p_is_radio = false, uint32_t prefFlags = 0>
class advconfig_entry_checkbox_runtime_impl : public advconfig_entry_checkbox_v2 {
public:
	advconfig_entry_checkbox_runtime_impl(const char * p_name,const GUID & p_guid,const GUID & p_parent,double p_priority,bool p_initialstate)
		: m_name(p_name), m_initialstate(p_initialstate), m_state(p_initialstate), m_parent(p_parent), m_priority(p_priority), m_guid(p_guid) {}
	
	void get_name(pfc::string_base & p_out) {p_out = m_name;}
	GUID get_guid() {return m_guid;}
	GUID get_parent() {return m_parent;}
	void reset() {m_state = m_initialstate;}
	bool get_state() {return m_state;}
	void set_state(bool p_state) {m_state = p_state;}
	bool is_radio() {return p_is_radio;}
	double get_sort_priority() {return m_priority;}
	bool get_state_() const {return m_state;}
	bool get_default_state() {return m_initialstate;}
	bool get_default_state_() const {return m_initialstate;}
	t_uint32 get_preferences_flags() {return prefFlags;}
private:
	pfc::string8 m_name;
	const bool m_initialstate;
	bool m_state;
	const GUID m_parent;
	const GUID m_guid;
	const double m_priority;
};

template<bool p_is_radio, uint32_t prefFlags = 0>
class advconfig_checkbox_factory_runtime_t : public service_factory_single_t<advconfig_entry_checkbox_runtime_impl<p_is_radio, prefFlags> > {
public:
	advconfig_checkbox_factory_runtime_t(const char * p_name,const GUID & p_guid,const GUID & p_parent,double p_priority,bool p_initialstate) 
		: service_factory_single_t<advconfig_entry_checkbox_runtime_impl<p_is_radio, prefFlags> >(p_name,p_guid,p_parent,p_priority,p_initialstate) {}

	bool get() const {return this->get_static_instance().get_state_();}
	void set(bool val) {this->get_static_instance().set_state(val);}
	operator bool() const {return get();}
	bool operator=(bool val) {set(val); return val;}
};

typedef advconfig_checkbox_factory_runtime_t<false> advconfig_checkbox_factory_runtime;
typedef advconfig_checkbox_factory_runtime_t<true> advconfig_radio_factory_runtime;
