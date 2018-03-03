#pragma once

class NOVTABLE service_factory_base {
public:
	inline const GUID & get_class_guid() const { return m_guid; }

	static service_class_ref enum_find_class(const GUID & p_guid);
	static bool enum_create(service_ptr_t<service_base> & p_out, service_class_ref p_class, t_size p_index);
	static t_size enum_get_count(service_class_ref p_class);

	inline static bool is_service_present(const GUID & g) { return enum_get_count(enum_find_class(g))>0; }

	//! Throws std::bad_alloc or another exception on failure.
	virtual void instance_create(service_ptr_t<service_base> & p_out) = 0;

	//! FOR INTERNAL USE ONLY
	static service_factory_base *__internal__list;
	//! FOR INTERNAL USE ONLY
	service_factory_base * __internal__next;
private:
	const GUID & m_guid;

protected:
	inline service_factory_base(const GUID & p_guid, service_factory_base * & factoryList = __internal__list) : m_guid(p_guid) { PFC_ASSERT(!core_api::are_services_available()); __internal__next = factoryList; factoryList = this; }
	inline ~service_factory_base() { PFC_ASSERT(!core_api::are_services_available()); }

};

template<typename B>
class service_factory_traits {
public:
	static service_factory_base * & factory_list() { return service_factory_base::__internal__list; }
};

template<typename B>
class service_factory_base_t : public service_factory_base {
public:
	service_factory_base_t() : service_factory_base(B::class_guid, service_factory_traits<B>::factory_list()) {
		pfc::assert_same_type<B, typename B::t_interface_entrypoint>();
	}
};

template<typename T>
class service_factory_t : public service_factory_base_t<typename T::t_interface_entrypoint> {
public:
	void instance_create(service_ptr_t<service_base> & p_out) {
		p_out = pfc::implicit_cast<service_base*>(pfc::implicit_cast<typename T::t_interface_entrypoint*>(pfc::implicit_cast<T*>(new service_impl_t<T>)));
	}
};


template<typename T>
class service_factory_single_t : public service_factory_base_t<typename T::t_interface_entrypoint> {
	service_impl_single_t<T> g_instance;
public:
	template<typename ... arg_t> service_factory_single_t(arg_t && ... arg) : g_instance(std::forward<arg_t>(arg) ...) {}

	void instance_create(service_ptr_t<service_base> & p_out) {
		p_out = pfc::implicit_cast<service_base*>(pfc::implicit_cast<typename T::t_interface_entrypoint*>(pfc::implicit_cast<T*>(&g_instance)));
	}

	inline T& get_static_instance() { return g_instance; }
	inline const T& get_static_instance() const { return g_instance; }
};

//! Alternate service_factory_single, shared instance created on first access and never deallocated. \n
//! Addresses the problem of dangling references to our object getting invoked or plainly de-refcounted during late shutdown.
template<typename T>
class service_factory_single_v2_t : public service_factory_base_t<typename T::t_interface_entrypoint> {
public:
	T * get() {
		static T * g_instance = new service_impl_single_t<T>;
		return g_instance;
	}
	void instance_create(service_ptr_t<service_base> & p_out) {
		p_out = pfc::implicit_cast<service_base*>(pfc::implicit_cast<typename T::t_interface_entrypoint*>(get()));
	}
};

template<typename T>
class service_factory_single_ref_t : public service_factory_base_t<typename T::t_interface_entrypoint>
{
private:
	T & instance;
public:
	service_factory_single_ref_t(T& param) : instance(param) {}

	void instance_create(service_ptr_t<service_base> & p_out) {
		p_out = pfc::implicit_cast<service_base*>(pfc::implicit_cast<typename T::t_interface_entrypoint*>(pfc::implicit_cast<T*>(&instance)));
	}

	inline T& get_static_instance() { return instance; }
};


template<typename T>
class service_factory_single_transparent_t : public service_factory_base_t<typename T::t_interface_entrypoint>, public service_impl_single_t<T>
{
public:
	template<typename ... arg_t> service_factory_single_transparent_t(arg_t && ... arg) : service_impl_single_t<T>(std::forward<arg_t>(arg) ...) {}

	void instance_create(service_ptr_t<service_base> & p_out) {
		p_out = pfc::implicit_cast<service_base*>(pfc::implicit_cast<typename T::t_interface_entrypoint*>(pfc::implicit_cast<T*>(this)));
	}

	inline T& get_static_instance() { return *(T*)this; }
};

