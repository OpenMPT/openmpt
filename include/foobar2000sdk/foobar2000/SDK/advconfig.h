//! Entrypoint class for adding items to Advanced Preferences page. \n
//! Implementations must derive from one of subclasses: advconfig_branch, advconfig_entry_checkbox, advconfig_entry_string. \n
//! Implementations are typically registered using static service_factory_single_t<myclass>, or using provided helper classes in case of standard implementations declared in this header.
class NOVTABLE advconfig_entry : public service_base {
public:
	virtual void get_name(pfc::string_base & p_out) = 0;
	virtual GUID get_guid() = 0;
	virtual GUID get_parent() = 0;
	virtual void reset() = 0;
	virtual double get_sort_priority() = 0;

	t_uint32 get_preferences_flags_();

	static bool g_find(service_ptr_t<advconfig_entry>& out, const GUID & id) {
		service_enum_t<advconfig_entry> e; service_ptr_t<advconfig_entry> ptr; while(e.next(ptr)) { if (ptr->get_guid() == id) {out = ptr; return true;} } return false;
	}

	template<typename outptr> static bool g_find_t(outptr & out, const GUID & id) {
		service_ptr_t<advconfig_entry> temp;
		if (!g_find(temp, id)) return false;
		return temp->service_query_t(out);
	}

	static const GUID guid_root;
	static const GUID guid_branch_tagging,guid_branch_decoding,guid_branch_tools,guid_branch_playback,guid_branch_display,guid_branch_debug;

	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(advconfig_entry);
};

//! Creates a new branch in Advanced Preferences. \n
//! Implementation: see advconfig_branch_impl / advconfig_branch_factory.
class NOVTABLE advconfig_branch : public advconfig_entry {
public:
	FB2K_MAKE_SERVICE_INTERFACE(advconfig_branch,advconfig_entry);
};

//! Creates a checkbox/radiocheckbox entry in Advanced Preferences. \n
//! The difference between checkboxes and radiocheckboxes is different icon (obviously) and that checking a radiocheckbox unchecks all other radiocheckboxes in the same branch. \n
//! Implementation: see advconfig_entry_checkbox_impl / advconfig_checkbox_factory_t.
class NOVTABLE advconfig_entry_checkbox : public advconfig_entry {
public:
	virtual bool get_state() = 0;
	virtual void set_state(bool p_state) = 0;
	virtual bool is_radio() = 0;

	bool get_default_state_();

	FB2K_MAKE_SERVICE_INTERFACE(advconfig_entry_checkbox,advconfig_entry);
};

class NOVTABLE advconfig_entry_checkbox_v2 : public advconfig_entry_checkbox {
	FB2K_MAKE_SERVICE_INTERFACE(advconfig_entry_checkbox_v2, advconfig_entry_checkbox)
public:
	virtual bool get_default_state() = 0;
	virtual t_uint32 get_preferences_flags() {return 0;} //signals whether changing this setting should trigger playback restart or app restart; see: preferences_state::* constants
};

//! Creates a string/integer editbox entry in Advanced Preferences.\n
//! Implementation: see advconfig_entry_string_impl / advconfig_string_factory.
class NOVTABLE advconfig_entry_string : public advconfig_entry {
public:
	virtual void get_state(pfc::string_base & p_out) = 0;
	virtual void set_state(const char * p_string,t_size p_length = ~0) = 0;
	virtual t_uint32 get_flags() = 0;

	void get_default_state_(pfc::string_base & out);

	enum {
		flag_is_integer		= 1 << 0, 
		flag_is_signed		= 1 << 1,
	};

	FB2K_MAKE_SERVICE_INTERFACE(advconfig_entry_string,advconfig_entry);
};

class NOVTABLE advconfig_entry_string_v2 : public advconfig_entry_string {
	FB2K_MAKE_SERVICE_INTERFACE(advconfig_entry_string_v2, advconfig_entry_string)
public:
	virtual void get_default_state(pfc::string_base & out) = 0;
	virtual void validate(pfc::string_base & val) {}
	virtual t_uint32 get_preferences_flags() {return 0;} //signals whether changing this setting should trigger playback restart or app restart; see: preferences_state::* constants
};


//! Standard implementation of advconfig_branch. \n
//! Usage: no need to use this class directly - use advconfig_branch_factory instead.
class advconfig_branch_impl : public advconfig_branch {
public:
	advconfig_branch_impl(const char * p_name,const GUID & p_guid,const GUID & p_parent,double p_priority) : m_name(p_name), m_guid(p_guid), m_parent(p_parent), m_priority(p_priority) {}
	void get_name(pfc::string_base & p_out) {p_out = m_name;}
	GUID get_guid() {return m_guid;}
	GUID get_parent() {return m_parent;}
	void reset() {}
	double get_sort_priority() {return m_priority;}
private:
	pfc::string8 m_name;
	GUID m_guid,m_parent;
	const double m_priority;
};

//! Standard implementation of advconfig_entry_checkbox. \n
//! p_is_radio parameter controls whether we're implementing a checkbox or a radiocheckbox (see advconfig_entry_checkbox description for more details).
template<bool p_is_radio = false, uint32_t prefFlags = 0>
class advconfig_entry_checkbox_impl : public advconfig_entry_checkbox_v2 {
public:
	advconfig_entry_checkbox_impl(const char * p_name,const GUID & p_guid,const GUID & p_parent,double p_priority,bool p_initialstate)
		: m_name(p_name), m_initialstate(p_initialstate), m_state(p_guid,p_initialstate), m_parent(p_parent), m_priority(p_priority) {}
	
	void get_name(pfc::string_base & p_out) {p_out = m_name;}
	GUID get_guid() {return m_state.get_guid();}
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
	cfg_bool m_state;
	GUID m_parent;
	const double m_priority;
};

//! Service factory helper around standard advconfig_branch implementation. Use this class to register your own Advanced Preferences branches. \n
//! Usage: static advconfig_branch_factory mybranch(name, branchID, parentBranchID, priority);
class advconfig_branch_factory : public service_factory_single_t<advconfig_branch_impl> {
public:
	advconfig_branch_factory(const char * p_name,const GUID & p_guid,const GUID & p_parent,double p_priority)
		: service_factory_single_t<advconfig_branch_impl>(p_name,p_guid,p_parent,p_priority) {}
};

//! Service factory helper around standard advconfig_entry_checkbox implementation. Use this class to register your own Advanced Preferences checkbox/radiocheckbox entries. \n
//! Usage: static advconfig_entry_checkbox<isRadioBox> mybox(name, itemID, parentID, priority, initialstate);
template<bool p_is_radio, uint32_t prefFlags = 0>
class advconfig_checkbox_factory_t : public service_factory_single_t<advconfig_entry_checkbox_impl<p_is_radio, prefFlags> > {
public:
	advconfig_checkbox_factory_t(const char * p_name,const GUID & p_guid,const GUID & p_parent,double p_priority,bool p_initialstate) 
		: service_factory_single_t<advconfig_entry_checkbox_impl<p_is_radio, prefFlags> >(p_name,p_guid,p_parent,p_priority,p_initialstate) {}

	bool get() const {return this->get_static_instance().get_state_();}
	void set(bool val) {this->get_static_instance().set_state(val);}
	operator bool() const {return get();}
	bool operator=(bool val) {set(val); return val;}
};

//! Service factory helper around standard advconfig_entry_checkbox implementation, specialized for checkboxes (rather than radiocheckboxes). See advconfig_checkbox_factory_t<> for more details.
typedef advconfig_checkbox_factory_t<false> advconfig_checkbox_factory;
//! Service factory helper around standard advconfig_entry_checkbox implementation, specialized for radiocheckboxes (rather than standard checkboxes). See advconfig_checkbox_factory_t<> for more details.
typedef advconfig_checkbox_factory_t<true> advconfig_radio_factory;


//! Standard advconfig_entry_string implementation. Use advconfig_string_factory to register your own string entries in Advanced Preferences instead of using this class directly.
class advconfig_entry_string_impl : public advconfig_entry_string_v2 {
public:
	advconfig_entry_string_impl(const char * p_name,const GUID & p_guid,const GUID & p_parent,double p_priority,const char * p_initialstate, t_uint32 p_prefFlags)
		: m_name(p_name), m_parent(p_parent), m_priority(p_priority), m_initialstate(p_initialstate), m_state(p_guid,p_initialstate), m_prefFlags(p_prefFlags) {}
	void get_name(pfc::string_base & p_out) {p_out = m_name;}
	GUID get_guid() {return m_state.get_guid();}
	GUID get_parent() {return m_parent;}
	void reset() {core_api::ensure_main_thread();m_state = m_initialstate;}
	double get_sort_priority() {return m_priority;}
	void get_state(pfc::string_base & p_out) {core_api::ensure_main_thread();p_out = m_state;}
	void set_state(const char * p_string,t_size p_length = ~0) {core_api::ensure_main_thread();m_state.set_string(p_string,p_length);}
	t_uint32 get_flags() {return 0;}
	void get_default_state(pfc::string_base & out) {out = m_initialstate;}
	t_uint32 get_preferences_flags() {return m_prefFlags;}
private:
	const pfc::string8 m_initialstate, m_name;
	cfg_string m_state;
	const double m_priority;
	const GUID m_parent;
	const t_uint32 m_prefFlags;
};

//! Service factory helper around standard advconfig_entry_string implementation. Use this class to register your own string entries in Advanced Preferences. \n
//! Usage: static advconfig_string_factory mystring(name, itemID, branchID, priority, initialValue);
class advconfig_string_factory : public service_factory_single_t<advconfig_entry_string_impl> {
public:
	advconfig_string_factory(const char * p_name,const GUID & p_guid,const GUID & p_parent,double p_priority,const char * p_initialstate, t_uint32 p_prefFlags = 0) 
		: service_factory_single_t<advconfig_entry_string_impl>(p_name,p_guid,p_parent,p_priority,p_initialstate, p_prefFlags) {}

	void get(pfc::string_base & out) {get_static_instance().get_state(out);}
	void set(const char * in) {get_static_instance().set_state(in);}
};


//! Special advconfig_entry_string implementation - implements integer entries. Use advconfig_integer_factory to register your own integer entries in Advanced Preferences instead of using this class directly.
template<typename int_t_>
class advconfig_entry_integer_impl_ : public advconfig_entry_string_v2 {
public:
	typedef int_t_ int_t;
	advconfig_entry_integer_impl_(const char * p_name,const GUID & p_guid,const GUID & p_parent,double p_priority,int_t p_initialstate,int_t p_min,int_t p_max, t_uint32 p_prefFlags)
		: m_name(p_name), m_parent(p_parent), m_priority(p_priority), m_initval(p_initialstate), m_min(p_min), m_max(p_max), m_state(p_guid,p_initialstate), m_prefFlags(p_prefFlags) {
			PFC_ASSERT( p_min < p_max );
	}
	void get_name(pfc::string_base & p_out) {p_out = m_name;}
	GUID get_guid() {return m_state.get_guid();}
	GUID get_parent() {return m_parent;}
	void reset() {m_state = m_initval;}
	double get_sort_priority() {return m_priority;}
	void get_state(pfc::string_base & p_out) {format(p_out, m_state.get_value());}
	void set_state(const char * p_string,t_size p_length) {set_state_int(myATOI(p_string,p_length));}
	t_uint32 get_flags() {return advconfig_entry_string::flag_is_integer | (is_signed() ? flag_is_signed : 0);}

	int_t get_state_int() const {return m_state;}
	void set_state_int(int_t val) {m_state = pfc::clip_t<int_t>(val,m_min,m_max);}

	void get_default_state(pfc::string_base & out) {
		format(out, m_initval);
	}
	void validate(pfc::string_base & val) {
		format(val, pfc::clip_t<int_t>(myATOI(val,~0), m_min, m_max) );
	}
	t_uint32 get_preferences_flags() {return m_prefFlags;}
private:
	static void format( pfc::string_base & out, int_t v ) {
		if (is_signed()) out = pfc::format_int( v ).get_ptr();
		else out = pfc::format_uint( v ).get_ptr();
	}
	static int_t myATOI( const char * s, size_t l ) {
		if (is_signed()) return pfc::atoi64_ex(s,l);
		else return pfc::atoui64_ex(s,l);
	}
	static bool is_signed() {
		return ((int_t)-1) < ((int_t)0);
	}
	cfg_int_t<int_t> m_state;
	const double m_priority;
	const int_t m_initval, m_min, m_max;
	const GUID m_parent;
	const pfc::string8 m_name;
	const t_uint32 m_prefFlags;
};

typedef advconfig_entry_integer_impl_<uint64_t> advconfig_entry_integer_impl;

//! Service factory helper around integer-specialized advconfig_entry_string implementation. Use this class to register your own integer entries in Advanced Preferences. \n
//! Usage: static advconfig_integer_factory myint(name, itemID, parentID, priority, initialValue, minValue, maxValue);
template<typename int_t_>
class advconfig_integer_factory_ : public service_factory_single_t<advconfig_entry_integer_impl_<int_t_> > {
public:
	typedef int_t_ int_t;
	advconfig_integer_factory_(const char * p_name,const GUID & p_guid,const GUID & p_parent,double p_priority,t_uint64 p_initialstate,t_uint64 p_min,t_uint64 p_max, t_uint32 p_prefFlags = 0) 
		: service_factory_single_t<advconfig_entry_integer_impl_<int_t_> >(p_name,p_guid,p_parent,p_priority,p_initialstate,p_min,p_max,p_prefFlags) {}

	int_t get() const {return get_static_instance().get_state_int();}
	void set(int_t val) {get_static_instance().set_state_int(val);}

	operator int_t() const {return get();}
	int_t operator=(int_t val) {set(val); return val;}
};

typedef advconfig_integer_factory_<uint64_t> advconfig_integer_factory;
typedef advconfig_integer_factory_<int64_t> advconfig_signed_integer_factory;

//! Not currently used, reserved for future use.
class NOVTABLE advconfig_entry_enum : public advconfig_entry {
public:
	virtual t_size get_value_count() = 0;
	virtual void enum_value(pfc::string_base & p_out,t_size p_index) = 0;
	virtual t_size get_state() = 0;
	virtual void set_state(t_size p_value) = 0;
	
	FB2K_MAKE_SERVICE_INTERFACE(advconfig_entry_enum,advconfig_entry);
};




//! Special version if advconfig_entry_string_impl that allows the value to be retrieved from worker threads.
class advconfig_entry_string_impl_MT : public advconfig_entry_string_v2 {
public:
	advconfig_entry_string_impl_MT(const char * p_name,const GUID & p_guid,const GUID & p_parent,double p_priority,const char * p_initialstate, t_uint32 p_prefFlags)
		: m_name(p_name), m_parent(p_parent), m_priority(p_priority), m_initialstate(p_initialstate), m_state(p_guid,p_initialstate), m_prefFlags(p_prefFlags) {}
	void get_name(pfc::string_base & p_out) {p_out = m_name;}
	GUID get_guid() {return m_state.get_guid();}
	GUID get_parent() {return m_parent;}
	void reset() {
		insync(m_sync);
		m_state = m_initialstate;
	}
	double get_sort_priority() {return m_priority;}
	void get_state(pfc::string_base & p_out) {
		insync(m_sync);
		p_out = m_state;
	}
	void set_state(const char * p_string,t_size p_length = ~0) {
		insync(m_sync);
		m_state.set_string(p_string,p_length);
	}
	t_uint32 get_flags() {return 0;}
	void get_default_state(pfc::string_base & out) {out = m_initialstate;}
	t_uint32 get_preferences_flags() {return m_prefFlags;}
private:
	const pfc::string8 m_initialstate, m_name;
	cfg_string m_state;
	critical_section m_sync;
	const double m_priority;
	const GUID m_parent;
	const t_uint32 m_prefFlags;
};

//! Special version if advconfig_string_factory that allows the value to be retrieved from worker threads.
class advconfig_string_factory_MT : public service_factory_single_t<advconfig_entry_string_impl_MT> {
public:
	advconfig_string_factory_MT(const char * p_name,const GUID & p_guid,const GUID & p_parent,double p_priority,const char * p_initialstate, t_uint32 p_prefFlags = 0) 
		: service_factory_single_t<advconfig_entry_string_impl_MT>(p_name,p_guid,p_parent,p_priority,p_initialstate, p_prefFlags) {}

	void get(pfc::string_base & out) {get_static_instance().get_state(out);}
	void set(const char * in) {get_static_instance().set_state(in);}
};




/* 
  Advanced Preferences variable declaration examples 
	
	static advconfig_string_factory mystring("name goes here",myguid,parentguid,0,"asdf");
	to retrieve state: pfc::string8 val; mystring.get(val);

	static advconfig_checkbox_factory mycheckbox("name goes here",myguid,parentguid,0,false);
	to retrieve state: mycheckbox.get();

	static advconfig_integer_factory myint("name goes here",myguid,parentguid,0,initialValue,minimumValue,maximumValue);
	to retrieve state: myint.get();
*/
