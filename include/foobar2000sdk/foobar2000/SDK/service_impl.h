#pragma once

// service_impl.h
// This header provides functionality for spawning your own service objects.
// Various service_impl_* classes implement service_base methods (reference counting, query for interface) on top of your class.
// service_impl_* are top level ("sealed" in C# terms) classes; they derive from your classes but you should never derive from them.

#include <utility>

namespace service_impl_helper {
	//! Helper function to defer destruction of a service object. 
	//! Enqueues a main_thread_callback to release the object at a later time, escaping the current scope.
    void release_object_delayed(service_ptr obj);
};

//! Multi inheritance helper. \n
//! Please note that use of multi inheritance is not recommended. Most components will never need this. \n
//! This class handles multi inherited service_query() for you. \n
//! Usage: class myclass : public service_multi_inherit<interface1, interface2> {...}; \n
//! It's also legal to chain it: service_multi_inherit<interface1, service_multi_inherit<interface2, interface3> > and so on.
template<typename class1_t, typename class2_t>
class service_multi_inherit : public class1_t, public class2_t {
	typedef service_multi_inherit<class1_t, class2_t> self_t;
public:
	static bool handle_service_query(service_ptr & out, const GUID & guid, self_t * in) {
		return service_base::handle_service_query(out, guid, (class1_t*) in) || service_base::handle_service_query(out, guid, (class2_t*) in);
	}

	service_base * as_service_base() { return class1_t::as_service_base(); }

	// Obscure service_base methods from both so calling myclass->service_query() works like it should
	virtual int service_release() throw() = 0;
	virtual int service_add_ref() throw() = 0;
	virtual bool service_query(service_ptr & p_out, const GUID & p_guid) = 0;
};

//! Template implementing service_query walking the inheritance chain. \n
//! Do not use directly. Each service_impl_* template utilizes it implicitly.
template<typename class_t> class implement_service_query : public class_t 
{
	typedef class_t base_t;
public:
	template<typename ... arg_t> implement_service_query( arg_t && ... arg ) : base_t( std::forward<arg_t>(arg) ... ) {}

	bool service_query(service_ptr_t<service_base> & p_out, const GUID & p_guid) {
		return this->handle_service_query( p_out, p_guid, this );
	}
};

//! Template implementing reference-counting features of service_base. Intended for dynamic instantiation: "new service_impl_t<someclass>(param1,param2);"; should not be instantiated otherwise (no local/static/member objects) because it does a "delete this;" when reference count reaches zero. \n
//! Note that there's no more need to use this direclty, see fb2k::service_new<>().
template<typename class_t>
class service_impl_t : public implement_service_query<class_t>
{
	typedef implement_service_query<class_t> base_t;
public:
	int service_release() throw() {
		int ret = (int) --m_counter;
		if (ret == 0) {
            if (!this->serviceRequiresMainThreadDestructor() || core_api::is_main_thread()) {
				PFC_ASSERT_NO_EXCEPTION( delete this );
            } else {
                service_impl_helper::release_object_delayed(this->as_service_base());
            }
		}
		return ret;
	}
	int service_add_ref() throw() {return (int) ++m_counter;}

	template<typename ... arg_t> service_impl_t( arg_t && ... arg ) : base_t( std::forward<arg_t>(arg) ... ) {}
private:
	pfc::refcounter m_counter;
};

//! Alternate version of service_impl_t<> - calls this->service_shutdown() instead of delete this. \n
//! For special cases where selfdestruct on zero refcount is undesired.
template<typename class_t>
class service_impl_explicitshutdown_t : public implement_service_query<class_t> 
{
	typedef implement_service_query<class_t> base_t;
public:
	int service_release() throw() {
		int ret = --m_counter;
		if (ret == 0) {
			this->service_shutdown();
		} else {
			return ret;
		}
	}
	int service_add_ref() throw() {return ++m_counter;}

	template<typename ... arg_t> service_impl_explicitshutdown_t(arg_t && ... arg) : base_t(std::forward<arg_t>(arg) ...) {}
private:
	pfc::refcounter m_counter;
};

//! Template implementing dummy version of reference-counting features of service_base. Intended for static/local/member instantiation: "static service_impl_single_t<someclass> myvar(params);". Because reference counting features are disabled (dummy reference counter), code instantiating it is responsible for deleting it as well as ensuring that no references are active when the object gets deleted.\n
template<typename class_t>
class service_impl_single_t : public implement_service_query<class_t>
{
	typedef implement_service_query<class_t> base_t;
public:
	int service_release() throw() {return 1;}
	int service_add_ref() throw() {return 1;}

	template<typename ... arg_t> service_impl_single_t(arg_t && ... arg) : base_t(std::forward<arg_t>(arg) ...) {}
};

namespace fb2k {
	//! The new recommended way of spawning service objects, automatically implementing reference counting and service_query on top of your class. \n
	//! Usage: auto myObj = fb2k::service_new<myClass>(args); \n
	//! Returned type is a service_ptr_t<myClass>
	template<typename obj_t, typename ... arg_t> 
	service_ptr_t<obj_t> service_new(arg_t && ... arg) {
		return new service_impl_t< obj_t > ( std::forward<arg_t> (arg) ... );
	}
}