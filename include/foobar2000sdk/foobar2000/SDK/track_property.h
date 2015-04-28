//! Callback interface for track_property_provider::enumerate_properties().
class NOVTABLE track_property_callback {
public:
	//! Sets a property list entry to display. Called by track_property_provider::enumerate_properties() implementation.
	//! @param p_group Name of group to put the entry in, case-sensitive. Note that non-standard groups are sorted alphabetically.
	//! @param p_sortpriority Sort priority of the property inside its group (smaller value means earlier in the list), pass 0 if you don't care (alphabetic order by name used when more than one item has same priority).
	//! @param p_name Name of the property.
	//! @param p_value Value of the property.
	virtual void set_property(const char * p_group,double p_sortpriority,const char * p_name,const char * p_value) = 0;
protected:
	track_property_callback const & operator=(track_property_callback const &) {return *this;}
	~track_property_callback() {}
};

class NOVTABLE track_property_callback_v2 : public track_property_callback {
public:
	virtual bool is_group_wanted(const char * p_group) = 0;
protected:
	~track_property_callback_v2() {}
};

//! Service for adding custom entries in "Properties" tab of the properties dialog.
class NOVTABLE track_property_provider : public service_base {
public:
	//! Enumerates properties of specified track list.
	//! @param p_tracks List of tracks to enumerate properties on.
	//! @param p_out Callback interface receiving enumerated properties.
	virtual void enumerate_properties(metadb_handle_list_cref p_tracks, track_property_callback & p_out) = 0;
	//! Returns whether specified tech info filed is processed by our service and should not be displayed among unknown fields.
	//! @param p_name Name of tech info field being queried.
	//! @returns True if the field is among fields processed by this track_property_provider implementation and should not be displayed among unknown fields, false otherwise.
	virtual bool is_our_tech_info(const char * p_name) = 0;

	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(track_property_provider);
};

class NOVTABLE track_property_provider_v2 : public track_property_provider {
	FB2K_MAKE_SERVICE_INTERFACE(track_property_provider_v2,track_property_provider)
public:
	virtual void enumerate_properties_v2(metadb_handle_list_cref p_tracks, track_property_callback_v2 & p_out) = 0;
};

class NOVTABLE track_property_provider_v3_info_source {
public:
	virtual metadb_info_container::ptr get_info(size_t index) = 0;
};

class track_property_provider_v3_info_source_impl : public track_property_provider_v3_info_source {
public:
	track_property_provider_v3_info_source_impl(metadb_handle_list_cref items) : m_items(items) {}
	metadb_info_container::ptr get_info(size_t index) {return m_items[index]->get_info_ref();}
private:
	metadb_handle_list_cref m_items;
};

class track_property_callback_v2_proxy : public track_property_callback_v2 {
public:
	track_property_callback_v2_proxy(track_property_callback & callback) : m_callback(callback) {}
	void set_property(const char * p_group,double p_sortpriority,const char * p_name,const char * p_value) {m_callback.set_property(p_group, p_sortpriority, p_name, p_value);}
	bool is_group_wanted(const char*) {return true;}

private:
	track_property_callback & m_callback;
};

//! \since 1.3
class NOVTABLE track_property_provider_v3 : public track_property_provider_v2 {
	FB2K_MAKE_SERVICE_INTERFACE(track_property_provider_v3,track_property_provider_v2)
public:
	virtual void enumerate_properties_v3(metadb_handle_list_cref items, track_property_provider_v3_info_source & info, track_property_callback_v2 & callback) = 0;	

	void enumerate_properties(metadb_handle_list_cref p_tracks, track_property_callback & p_out) {track_property_provider_v3_info_source_impl src(p_tracks); track_property_callback_v2_proxy cb(p_out); enumerate_properties_v3(p_tracks, src, cb);}
	void enumerate_properties_v2(metadb_handle_list_cref p_tracks, track_property_callback_v2 & p_out) {track_property_provider_v3_info_source_impl src(p_tracks); enumerate_properties_v3(p_tracks, src, p_out);}
};

template<typename T>
class track_property_provider_factory_t : public service_factory_single_t<T> {};
