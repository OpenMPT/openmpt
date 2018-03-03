#pragma once

template<typename what>
static bool service_by_guid_fallback(service_ptr_t<what> & out, const GUID & id) {
	service_enum_t<what> e;
	service_ptr_t<what> ptr;
	while (e.next(ptr)) {
		if (ptr->get_guid() == id) { out = ptr; return true; }
	}
	return false;
}

template<typename what>
class service_by_guid_data {
public:
	service_by_guid_data() : m_servClass(), m_inited() {}

	bool ready() const { return m_inited; }

	// Caller must ensure initialize call before create() as well as thread safety of initialize() calls. The rest of this class is thread safe (only reads member data).
	void initialize() {
		if (m_inited) return;
		pfc::assert_same_type< what, typename what::t_interface_entrypoint >();
		m_servClass = service_factory_base::enum_find_class(what::class_guid);
		const t_size servCount = service_factory_base::enum_get_count(m_servClass);
		for (t_size walk = 0; walk < servCount; ++walk) {
			service_ptr_t<what> temp;
			if (_service_instantiate_helper(temp, m_servClass, walk)) {
				m_order.set(temp->get_guid(), walk);
			}
		}
		m_inited = true;
	}

	bool create(service_ptr_t<what> & out, const GUID & theID) const {
		PFC_ASSERT(m_inited);
		t_size index;
		if (!m_order.query(theID, index)) return false;
		return _service_instantiate_helper(out, m_servClass, index);
	}
	service_ptr_t<what> create(const GUID & theID) const {
		service_ptr_t<what> temp; if (!crete(temp, theID)) throw exception_service_not_found(); return temp;
	}

private:
	volatile bool m_inited;
	pfc::map_t<GUID, t_size> m_order;
	service_class_ref m_servClass;
};

template<typename what>
class _service_by_guid_data_container {
public:
	static service_by_guid_data<what> data;
};
template<typename what> service_by_guid_data<what> _service_by_guid_data_container<what>::data;


template<typename what>
static void service_by_guid_init() {
	service_by_guid_data<what> & data = _service_by_guid_data_container<what>::data;
	data.initialize();
}
template<typename what>
static bool service_by_guid(service_ptr_t<what> & out, const GUID & theID) {
	pfc::assert_same_type< what, typename what::t_interface_entrypoint >();
	service_by_guid_data<what> & data = _service_by_guid_data_container<what>::data;
	if (data.ready()) {
		//fall-thru
	} else if (core_api::is_main_thread()) {
		data.initialize();
	} else {
#ifdef _DEBUG
		FB2K_DebugLog() << "Warning: service_by_guid() used in non-main thread without initialization, using fallback";
#endif
		return service_by_guid_fallback(out, theID);
	}
	return data.create(out, theID);
}
template<typename what>
static service_ptr_t<what> service_by_guid(const GUID & theID) {
	service_ptr_t<what> temp;
	if (!service_by_guid(temp, theID)) throw exception_service_not_found();
	return temp;
}

#define FB2K_FOR_EACH_SERVICE(type, call) {service_enum_t<typename type::t_interface_entrypoint> e; service_ptr_t<type> ptr; while(e.next(ptr)) {ptr->call;} }



class comparator_service_guid {
public:
	template<typename what> static int compare(const what & v1, const what & v2) { return pfc::compare_t(v1->get_guid(), v2->get_guid()); }
};

