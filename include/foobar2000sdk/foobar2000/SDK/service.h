#pragma once

#include <utility> // std::forward

typedef const void* service_class_ref;

PFC_DECLARE_EXCEPTION(exception_service_not_found,pfc::exception,"Service not found");
PFC_DECLARE_EXCEPTION(exception_service_extension_not_found,pfc::exception,"Service extension not found");
PFC_DECLARE_EXCEPTION(exception_service_duplicated,pfc::exception,"Service duplicated");

#ifdef _MSC_VER
#define FOOGUIDDECL __declspec(selectany)
#else
#define FOOGUIDDECL
#endif


#define DECLARE_GUID(NAME,A,S,D,F,G,H,J,K,L,Z,X) FOOGUIDDECL const GUID NAME = {A,S,D,{F,G,H,J,K,L,Z,X}};
#define DECLARE_CLASS_GUID(NAME,A,S,D,F,G,H,J,K,L,Z,X) FOOGUIDDECL const GUID NAME::class_guid = {A,S,D,{F,G,H,J,K,L,Z,X}};

//! Special hack to ensure errors when someone tries to ->service_add_ref()/->service_release() on a service_ptr_t
template<typename T> class service_obscure_refcounting : public T {
private:
	int service_add_ref() throw();
	int service_release() throw();
};

//! Converts a service interface pointer to a pointer that obscures service counter functionality.
template<typename T> static inline service_obscure_refcounting<T>* service_obscure_refcounting_cast(T * p_source) throw() {return static_cast<service_obscure_refcounting<T>*>(p_source);}

//Must be templated instead of taking service_base* because of multiple inheritance issues.
template<typename T> static void service_release_safe(T * p_ptr) throw() {
	if (p_ptr != NULL) PFC_ASSERT_NO_EXCEPTION( p_ptr->service_release() );
}

//Must be templated instead of taking service_base* because of multiple inheritance issues.
template<typename T> static void service_add_ref_safe(T * p_ptr) throw() {
	if (p_ptr != NULL) PFC_ASSERT_NO_EXCEPTION( p_ptr->service_add_ref() );
}

class service_base;

template<typename T>
class service_ptr_base_t {
public:
	inline T* get_ptr() const throw() {return m_ptr;}
protected:
	T * m_ptr;
};

// forward declaration
template<typename T> class service_nnptr_t;

//! Autopointer class to be used with all services. Manages reference counter calls behind-the-scenes.
template<typename T>
class service_ptr_t : public service_ptr_base_t<T> {
private:
	typedef service_ptr_t<T> t_self;

	template<typename t_source> void _init(t_source * in) throw() {
		this->m_ptr = in;
		if (this->m_ptr) this->m_ptr->service_add_ref();
	}
	template<typename t_source> void _init(t_source && in) throw() {
		this->m_ptr = in.detach();
	}
public:
	service_ptr_t() throw() {this->m_ptr = NULL;}
	service_ptr_t(T * p_ptr) throw() {_init(p_ptr);}
	service_ptr_t(const t_self & p_source) throw() {_init(p_source.get_ptr());}
	service_ptr_t(t_self && p_source) throw() {_init(std::move(p_source));}
	template<typename t_source> service_ptr_t(t_source * p_ptr) throw() {_init(p_ptr);}
	template<typename t_source> service_ptr_t(const service_ptr_base_t<t_source> & p_source) throw() {_init(p_source.get_ptr());}
	template<typename t_source> service_ptr_t(const service_nnptr_t<t_source> & p_source) throw() { this->m_ptr = p_source.get_ptr(); this->m_ptr->service_add_ref(); }
	template<typename t_source> service_ptr_t(service_ptr_t<t_source> && p_source) throw() { _init(std::move(p_source)); }
	
	~service_ptr_t() throw() {service_release_safe(this->m_ptr);}
	
	template<typename t_source>
	void copy(t_source * p_ptr) throw() {
		service_add_ref_safe(p_ptr);
		service_release_safe(this->m_ptr);
		this->m_ptr = pfc::safe_ptr_cast<T>(p_ptr);
	}

	template<typename t_source>
	void copy(const service_ptr_base_t<t_source> & p_source) throw() {copy(p_source.get_ptr());}

	template<typename t_source>
	void copy(service_ptr_t<t_source> && p_source) throw() {attach(p_source.detach());}


	inline const t_self & operator=(const t_self & p_source) throw() {copy(p_source); return *this;}
	inline const t_self & operator=(t_self && p_source) throw() {copy(std::move(p_source)); return *this;}
	inline const t_self & operator=(T * p_ptr) throw() {copy(p_ptr); return *this;}

	template<typename t_source> inline t_self & operator=(const service_ptr_base_t<t_source> & p_source) throw() {copy(p_source); return *this;}
	template<typename t_source> inline t_self & operator=(service_ptr_t<t_source> && p_source) throw() {copy(std::move(p_source)); return *this;}
	template<typename t_source> inline t_self & operator=(t_source * p_ptr) throw() {copy(p_ptr); return *this;}

	template<typename t_source> inline t_self & operator=(const service_nnptr_t<t_source> & p_ptr) throw() {
		service_release_safe(this->m_ptr);
		t_source * ptr = p_ptr.get_ptr();
		ptr->service_add_ref();
		this->m_ptr = ptr;
		return *this;
	}

	
	inline void release() throw() {
		service_release_safe(this->m_ptr);
		this->m_ptr = NULL;
	}


	inline service_obscure_refcounting<T>* operator->() const throw() {PFC_ASSERT(this->m_ptr != NULL);return service_obscure_refcounting_cast(this->m_ptr);}

	inline T* get_ptr() const throw() {return this->m_ptr;}
	
	inline bool is_valid() const throw() {return this->m_ptr != NULL;}
	inline bool is_empty() const throw() {return this->m_ptr == NULL;}

	inline bool operator==(const service_ptr_base_t<T> & p_item) const throw() {return this->m_ptr == p_item.get_ptr();}
	inline bool operator!=(const service_ptr_base_t<T> & p_item) const throw() {return this->m_ptr != p_item.get_ptr();}

	inline bool operator>(const service_ptr_base_t<T> & p_item) const throw() {return this->m_ptr > p_item.get_ptr();}
	inline bool operator<(const service_ptr_base_t<T> & p_item) const throw() {return this->m_ptr < p_item.get_ptr();}

	inline bool operator==(T * p_item) const throw() {return this->m_ptr == p_item;}
	inline bool operator!=(T * p_item) const throw() {return this->m_ptr != p_item;}

	inline bool operator>(T * p_item) const throw() {return this->m_ptr > p_item;}
	inline bool operator<(T * p_item) const throw() {return this->m_ptr < p_item;}

	template<typename t_other>
	inline t_self & operator<<(service_ptr_t<t_other> & p_source) throw() {attach(p_source.detach());return *this;}
	template<typename t_other>
	inline t_self & operator>>(service_ptr_t<t_other> & p_dest) throw() {p_dest.attach(detach());return *this;}


	inline T* _duplicate_ptr() const throw() {//should not be used ! temporary !
		service_add_ref_safe(this->m_ptr);
		return this->m_ptr;
	}

	inline T* detach() throw() {
		return pfc::replace_null_t(this->m_ptr);
	}

	template<typename t_source>
	inline void attach(t_source * p_ptr) throw() {
		service_release_safe(this->m_ptr);
		this->m_ptr = pfc::safe_ptr_cast<T>(p_ptr);
	}

	T & operator*() const throw() {return *this->m_ptr;}

	service_ptr_t<service_base> & _as_base_ptr() {
		PFC_ASSERT( _as_base_ptr_check() );
		return *reinterpret_cast<service_ptr_t<service_base>*>(this);
	}
	static bool _as_base_ptr_check() {
		return static_cast<service_base*>((T*)NULL) == reinterpret_cast<service_base*>((T*)NULL);
	}
        
    //! Forced cast operator - obtains a valid service pointer to the expected class or crashes the app if such pointer cannot be obtained.
    template<typename otherPtr_t>
    void operator ^= ( otherPtr_t other ) {
        if (other.is_empty()) release(); // allow null ptr, get upset only on type mismatch
        else if (!other->cast(*this)) uBugCheck();
    }
    template<typename otherObj_t>
    void operator ^= ( otherObj_t * other ) {
        if (other == nullptr) release();
        else forcedCastFrom( other );
    }

	//! Conditional cast operator - attempts to obtain a vaild service pointer to the expected class; returns true on success, false on failure.
    template<typename otherPtr_t>
    bool operator &= ( otherPtr_t other ) {
        PFC_ASSERT( other.is_valid() );
        return other->cast(*this);
    }
    template<typename otherObj_t>
    bool operator &= ( otherObj_t * other ) {
        if (other == nullptr) return false;
        return other->cast( *this );
    }
 
};

//! Autopointer class to be used with all services. Manages reference counter calls behind-the-scenes. \n
//! This assumes that the pointers are valid all the time (can't point to null). Mainly intended to be used for scenarios where null pointers are not valid and relevant code should crash ASAP if somebody passes invalid pointers around. \n
//! You want to use service_ptr_t<> rather than service_nnptr_t<> most of the time.
template<typename T>
class service_nnptr_t : public service_ptr_base_t<T> {
private:
	typedef service_nnptr_t<T> t_self;

	template<typename t_source> void _init(t_source * in) {
		this->m_ptr = in;
		this->m_ptr->service_add_ref();
	}
	service_nnptr_t() throw() {pfc::crash();}
public:
	service_nnptr_t(T * p_ptr) throw() {_init(p_ptr);}
	service_nnptr_t(const t_self & p_source) throw() {_init(p_source.get_ptr());}
	template<typename t_source> service_nnptr_t(t_source * p_ptr) throw() {_init(p_ptr);}
	template<typename t_source> service_nnptr_t(const service_ptr_base_t<t_source> & p_source) throw() {_init(p_source.get_ptr());}

	template<typename t_source> service_nnptr_t(service_ptr_t<t_source> && p_source) throw() {this->m_ptr = p_source.detach();}

	~service_nnptr_t() throw() {this->m_ptr->service_release();}
	
	template<typename t_source>
	void copy(t_source * p_ptr) throw() {
		p_ptr->service_add_ref();
		this->m_ptr->service_release();
		this->m_ptr = pfc::safe_ptr_cast<T>(p_ptr);
	}

	template<typename t_source>
	void copy(const service_ptr_base_t<t_source> & p_source) throw() {copy(p_source.get_ptr());}


	inline const t_self & operator=(const t_self & p_source) throw() {copy(p_source); return *this;}
	inline const t_self & operator=(T * p_ptr) throw() {copy(p_ptr); return *this;}

	template<typename t_source> inline t_self & operator=(const service_ptr_base_t<t_source> & p_source) throw() {copy(p_source); return *this;}
	template<typename t_source> inline t_self & operator=(t_source * p_ptr) throw() {copy(p_ptr); return *this;}
	template<typename t_source> inline t_self & operator=(service_ptr_t<t_source> && p_source) throw() {this->m_ptr->service_release(); this->m_ptr = p_source.detach();}


	inline service_obscure_refcounting<T>* operator->() const throw() {PFC_ASSERT(this->m_ptr != NULL);return service_obscure_refcounting_cast(this->m_ptr);}

	inline T* get_ptr() const throw() {return this->m_ptr;}
	
	inline bool is_valid() const throw() {return true;}
	inline bool is_empty() const throw() {return false;}

	inline bool operator==(const service_ptr_base_t<T> & p_item) const throw() {return this->m_ptr == p_item.get_ptr();}
	inline bool operator!=(const service_ptr_base_t<T> & p_item) const throw() {return this->m_ptr != p_item.get_ptr();}

	inline bool operator>(const service_ptr_base_t<T> & p_item) const throw() {return this->m_ptr > p_item.get_ptr();}
	inline bool operator<(const service_ptr_base_t<T> & p_item) const throw() {return this->m_ptr < p_item.get_ptr();}

	inline bool operator==(T * p_item) const throw() {return this->m_ptr == p_item;}
	inline bool operator!=(T * p_item) const throw() {return this->m_ptr != p_item;}

	inline bool operator>(T * p_item) const throw() {return this->m_ptr > p_item;}
	inline bool operator<(T * p_item) const throw() {return this->m_ptr < p_item;}

	inline T* _duplicate_ptr() const throw() {//should not be used ! temporary !
		service_add_ref_safe(this->m_ptr);
		return this->m_ptr;
	}

	T & operator*() const throw() {return *this->m_ptr;}

	service_ptr_t<service_base> & _as_base_ptr() {
		PFC_ASSERT( _as_base_ptr_check() );
		return *reinterpret_cast<service_ptr_t<service_base>*>(this);
	}
	static bool _as_base_ptr_check() {
		return static_cast<service_base*>((T*)NULL) == reinterpret_cast<service_base*>((T*)NULL);
	}
};

namespace pfc {
	class traits_service_ptr : public traits_default {
	public:
		enum { realloc_safe = true, constructor_may_fail = false};
	};

	template<typename T> class traits_t<service_ptr_t<T> > : public traits_service_ptr {};
	template<typename T> class traits_t<service_nnptr_t<T> > : public traits_service_ptr {};
}


template<typename T, template<typename> class t_alloc = pfc::alloc_fast>
class service_list_t : public pfc::list_t<service_ptr_t<T>, t_alloc >
{
};

//! For internal use, see FB2K_MAKE_SERVICE_INTERFACE
#define FB2K_MAKE_SERVICE_INTERFACE_EX(THISCLASS,PARENTCLASS,IS_CORE_API) \
	public:	\
		typedef THISCLASS t_interface;	\
		typedef PARENTCLASS t_interface_parent;	\
			\
		static const GUID class_guid;	\
			\
		typedef service_ptr_t<t_interface> ptr;	\
		typedef service_nnptr_t<t_interface> nnptr;	\
		typedef ptr ref; \
		typedef nnptr nnref; \
		enum { _is_core_api = IS_CORE_API }; \
	protected:	\
		THISCLASS() {}	\
		~THISCLASS() {}	\
	private:	\
		const THISCLASS & operator=(const THISCLASS &) = delete;	\
		THISCLASS(const THISCLASS &) = delete;	\
	private:	\
		void _private_service_declaration_selftest() {	\
			static_assert( pfc::is_same_type<PARENTCLASS,PARENTCLASS::t_interface>::value, "t_interface sanity" ); \
			static_assert( ! pfc::is_same_type<PARENTCLASS, THISCLASS>::value, "parent class sanity"); \
			static_assert( pfc::is_same_type<PARENTCLASS, service_base>::value || IS_CORE_API == PARENTCLASS::_is_core_api, "is_core_api sanity" ); \
			_validate_service_class_helper<THISCLASS>(); /*service_base must be reachable by walking t_interface_parent*/	\
			pfc::implicit_cast<service_base*>(this); /*this class must derive from service_base, directly or indirectly, and be implictly castable to it*/ \
		}

#define FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT_EX(THISCLASS, IS_CORE_API)	\
	public:	\
		typedef THISCLASS t_interface_entrypoint;	\
	FB2K_MAKE_SERVICE_INTERFACE_EX(THISCLASS,service_base, IS_CORE_API)

//! Helper macro for use when defining a service class. Generates standard features of a service, without ability to register using service_factory / enumerate using service_enum_t. \n
//! This is used for declaring services that are meant to be instantiated by means other than service_enum_t (or non-entrypoint services), or extensions of services (including extension of entrypoint services).	\n
//! Sample non-entrypoint declaration: class myclass : public service_base {...; FB2K_MAKE_SERVICE_INTERFACE(myclass, service_base); };	\n
//! Sample extension declaration: class myclass : public myotherclass {...; FB2K_MAKE_SERVICE_INTERFACE(myclass, myotherclass); };	\n
//! This macro is intended for use ONLY WITH INTERFACE CLASSES, not with implementation classes.
#define FB2K_MAKE_SERVICE_INTERFACE(THISCLASS, PARENTCLASS) FB2K_MAKE_SERVICE_INTERFACE_EX(THISCLASS, PARENTCLASS, false)

//! Helper macro for use when defining an entrypoint service class. Generates standard features of a service, including ability to register using service_factory and enumerate using service_enum.	\n
//! Sample declaration: class myclass : public service_base {...; FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(myclass); };	\n
//! Note that entrypoint service classes must directly derive from service_base, and not from another service class.
//! This macro is intended for use ONLY WITH INTERFACE CLASSES, not with implementation classes.
#define FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(THISCLASS) FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT_EX(THISCLASS, false)


#define FB2K_MAKE_SERVICE_COREAPI(THISCLASS) \
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT_EX( THISCLASS, true ) \
	public: static ptr get() { return fb2k::std_api_get<THISCLASS>(); } static bool tryGet(ptr & out) { return fb2k::std_api_try_get(out); }

#define FB2K_MAKE_SERVICE_COREAPI_EXTENSION(THISCLASS, BASECLASS) \
	FB2K_MAKE_SERVICE_INTERFACE_EX( THISCLASS, BASECLASS, true ) \
	public: static ptr get() { return fb2k::std_api_get<THISCLASS>(); } static bool tryGet(ptr & out) { return fb2k::std_api_try_get(out); }



//! Alternate way of declaring services, begin/end macros wrapping the whole class declaration
#define FB2K_DECLARE_SERVICE_BEGIN(THISCLASS,BASECLASS) \
	class NOVTABLE THISCLASS : public BASECLASS	{	\
		FB2K_MAKE_SERVICE_INTERFACE(THISCLASS,BASECLASS);	\
	public:

//! Alternate way of declaring services, begin/end macros wrapping the whole class declaration
#define FB2K_DECLARE_SERVICE_END() \
	};

//! Alternate way of declaring services, begin/end macros wrapping the whole class declaration
#define FB2K_DECLARE_SERVICE_ENTRYPOINT_BEGIN(THISCLASS) \
	class NOVTABLE THISCLASS : public service_base {	\
		FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(THISCLASS)	\
	public:

class service_base;
typedef service_ptr_t<service_base> service_ptr;
typedef service_nnptr_t<service_base> service_nnptr;
		

//! Base class for all service classes.\n
//! Provides interfaces for reference counter and querying for different interfaces supported by the object.\n
class NOVTABLE service_base
{
public:	
	//! Decrements reference count; deletes the object if reference count reaches zero. This is normally not called directly but managed by service_ptr_t<> template. \n
	//! Implemented by service_impl_* classes.
	//! @returns New reference count. For debug purposes only, in certain conditions return values may be unreliable.
	virtual int service_release() throw() = 0;
	//! Increments reference count. This is normally not called directly but managed by service_ptr_t<> template. \n
	//! Implemented by service_impl_* classes.
	//! @returns New reference count. For debug purposes only, in certain conditions return values may be unreliable.
	virtual int service_add_ref() throw() = 0;
	//! Queries whether the object supports specific interface and retrieves a pointer to that interface. This is normally not called directly but managed by service_query_t<> function template. \n
	//! Checks the parameter against GUIDs of interfaces supported by this object, if the GUID is one of supported interfaces, p_out is set to service_base pointer that can be static_cast<>'ed to queried interface and the method returns true; otherwise the method returns false. \n
	//! Implemented by service_impl_* classes. \n
	//! Note that service_query() implementation semantics (but not usage semantics) changed in SDK for foobar2000 1.4; they used to be auto-implemented by each service interface (via FB2K_MAKE_SERVICE_INTERFACE macro); they're now implemented in service_impl_* instead. See SDK readme for more details. \n
	virtual bool service_query(service_ptr & p_out,const GUID & p_guid) = 0;

	//! Queries whether the object supports specific interface and retrieves a pointer to that interface.
	//! @param p_out Receives pointer to queried interface on success.
	//! returns true on success, false on failure (interface not supported by the object).
	template<class T>
	bool service_query_t(service_ptr_t<T> & p_out)
	{
		pfc::assert_same_type<T,typename T::t_interface>();
		return service_query( *reinterpret_cast<service_ptr_t<service_base>*>(&p_out),T::class_guid);
	}
	//! New shortened version, same as service_query_t.
	template<typename outPtr_t> 
	bool cast( outPtr_t & outPtr ) { return service_query_t( outPtr ); }

	typedef service_base t_interface;
	enum { _is_core_api = false };

	static bool serviceRequiresMainThreadDestructor() { return false; }

	service_base * as_service_base() { return this; }
protected:
	service_base() {}
	~service_base() {}

	static bool service_query_walk(service_ptr &, const GUID &, service_base *) {
		return false;
	}

	template<typename interface_t> static bool service_query_walk(service_ptr & out, const GUID & guid, interface_t * in) {
		if (guid == interface_t::class_guid) {
			out = in; return true;
		}
		typename interface_t::t_interface_parent * chain = in;
		return service_query_walk(out, guid, chain);
	}
	template<typename class_t> static bool handle_service_query(service_ptr & out, const GUID & guid, class_t * in) {
		typename class_t::t_interface * in2 = in;
		return service_query_walk( out, guid, in2 );
	}
private:
	service_base(const service_base&) = delete;
	const service_base & operator=(const service_base&) = delete;
};


template<typename T>
inline void _validate_service_class_helper() {
	_validate_service_class_helper<typename T::t_interface_parent>();
}

template<>
inline void _validate_service_class_helper<service_base>() {}


template<typename T> static void _validate_service_ptr(service_ptr_t<T> const & ptr) {
	PFC_ASSERT( ptr.is_valid() );
	service_ptr_t<T> test;
	PFC_ASSERT( ptr->service_query_t(test) );
}

#ifdef _DEBUG
#define FB2K_ASSERT_VALID_SERVICE_PTR(ptr) _validate_service_ptr(ptr)
#else
#define FB2K_ASSERT_VALID_SERVICE_PTR(ptr)
#endif

template<class T> static bool service_enum_create_t(service_ptr_t<T> & p_out,t_size p_index) {
	pfc::assert_same_type<T,typename T::t_interface_entrypoint>();
	service_ptr_t<service_base> ptr;
	if (service_factory_base::enum_create(ptr,service_factory_base::enum_find_class(T::class_guid),p_index)) {
		p_out = static_cast<T*>(ptr.get_ptr());
		return true;
	} else {
		p_out.release();
		return false;
	}
}

template<typename T> static service_class_ref _service_find_class() {
	pfc::assert_same_type<T,typename T::t_interface_entrypoint>();
	return service_factory_base::enum_find_class(T::class_guid);
}

template<typename what>
static bool _service_instantiate_helper(service_ptr_t<what> & out, service_class_ref servClass, t_size index) {
	/*if (out._as_base_ptr_check()) {
		const bool state = service_factory_base::enum_create(out._as_base_ptr(), servClass, index);
		if (state) { FB2K_ASSERT_VALID_SERVICE_PTR(out); }
		return state;
	} else */{
		service_ptr temp;
		const bool state = service_factory_base::enum_create(temp, servClass, index);
		if (state) {
			out.attach( static_cast<what*>( temp.detach() ) );
			FB2K_ASSERT_VALID_SERVICE_PTR( out );
		}
		return state;
	}
}

template<typename T> class service_class_helper_t {
public:
	service_class_helper_t() : m_class(service_factory_base::enum_find_class(T::class_guid)) {
		pfc::assert_same_type<T,typename T::t_interface_entrypoint>();
	}
	t_size get_count() const {
		return service_factory_base::enum_get_count(m_class);
	}

	bool create(service_ptr_t<T> & p_out,t_size p_index) const {
		return _service_instantiate_helper(p_out, m_class, p_index);
	}

	service_ptr_t<T> create(t_size p_index) const {
		service_ptr_t<T> temp;
		if (!create(temp,p_index)) uBugCheck();
		return temp;
	}
	service_class_ref get_class() const {return m_class;}
private:
	service_class_ref m_class;
};

void _standard_api_create_internal(service_ptr & out, const GUID & classID);
void _standard_api_get_internal(service_ptr & out, const GUID & classID);
bool _standard_api_try_get_internal(service_ptr & out, const GUID & classID);

template<typename T> inline void standard_api_create_t(service_ptr_t<T> & p_out) {
	if (pfc::is_same_type<T,typename T::t_interface_entrypoint>::value) {
		_standard_api_create_internal(p_out._as_base_ptr(), T::class_guid);
		FB2K_ASSERT_VALID_SERVICE_PTR(p_out);
	} else {
		service_ptr_t<typename T::t_interface_entrypoint> temp;
		standard_api_create_t(temp);
		if (!temp->service_query_t(p_out)) throw exception_service_extension_not_found();
	}
}

template<typename T> inline void standard_api_create_t(T* & p_out) {
	p_out = NULL;
	standard_api_create_t( *reinterpret_cast< service_ptr_t<T> * >( & p_out ) );
}

template<typename T> inline service_ptr_t<T> standard_api_create_t() {
	service_ptr_t<T> temp;
	standard_api_create_t(temp);
	return temp;
}

template<typename T>
inline bool static_api_test_t() {
	typedef typename T::t_interface_entrypoint EP;
	service_class_helper_t<EP> helper;
	if (helper.get_count() != 1) return false;
	if (!pfc::is_same_type<T,EP>::value) {
		service_ptr_t<T> t;
		if (!helper.create(0)->service_query_t(t)) return false;
	}
	return true;
}

#define FB2K_API_AVAILABLE(API) static_api_test_t<API>()

//! Helper template used to easily access core services. \n
//! Usage: static_api_ptr_t<myclass> api; api->dosomething(); \n
//! Can be used at any point of code, WITH EXCEPTION of static objects that are initialized during the DLL loading process before the service system is initialized; such as static static_api_ptr_t objects or having static_api_ptr_t instances as members of statically created objects. \n
//! Throws exception_service_not_found if service could not be reached (which can be ignored for core APIs that are always present unless there is some kind of bug in the code). \n
//! This class is provided for backwards compatibility. The recommended way to do this stuff is now someclass::get() / someclass::tryGet().
template<typename t_interface>
class static_api_ptr_t {
private:
	typedef static_api_ptr_t<t_interface> t_self;
public:
	static_api_ptr_t() {
		standard_api_create_t(m_ptr);
	}
	service_obscure_refcounting<t_interface>* operator->() const {return service_obscure_refcounting_cast(m_ptr);}
	t_interface * get_ptr() const {return m_ptr;}
	~static_api_ptr_t() {m_ptr->service_release();}
	
	static_api_ptr_t(const t_self & in) {
		m_ptr = in.m_ptr; m_ptr->service_add_ref();
	}
	const t_self & operator=(const t_self & in) {return *this;} //obsolete, each instance should carry the same pointer
private:
	t_interface * m_ptr;
};

//! Helper; simulates array with instance of each available implementation of given service class.
template<typename T> class service_instance_array_t {
public:
	typedef service_ptr_t<T> t_ptr;
	service_instance_array_t() {
		service_class_helper_t<T> helper;
		const t_size count = helper.get_count();
		m_data.set_size(count);
		for(t_size n=0;n<count;n++) m_data[n] = helper.create(n);
	}

	t_size get_size() const {return m_data.get_size();}
	const t_ptr & operator[](t_size p_index) const {return m_data[p_index];}

	//nonconst version to allow sorting/bsearching; do not abuse
	t_ptr & operator[](t_size p_index) {return m_data[p_index];}
private:
	pfc::array_t<t_ptr> m_data;
};

template<typename t_interface>
class service_enum_t {
public:
	service_enum_t() : m_index(0) {
		pfc::assert_same_type<t_interface,typename t_interface::t_interface_entrypoint>();
	}
	void reset() {m_index = 0;}

	template<typename t_query>
	bool first(service_ptr_t<t_query> & p_out) {
		reset();
		return next(p_out);
	}
	
	template<typename t_query>
	bool next(service_ptr_t<t_query> & p_out) {
		pfc::assert_same_type<typename t_query::t_interface_entrypoint,t_interface>();
		if (pfc::is_same_type<t_query,t_interface>::value) {
			return _next(reinterpret_cast<service_ptr_t<t_interface>&>(p_out));
		} else {
			service_ptr_t<t_interface> temp;
			while(_next(temp)) {
				if (temp->service_query_t(p_out)) return true;
			}
			return false;
		}
	}

private:
	bool _next(service_ptr_t<t_interface> & p_out) {
		return m_helper.create(p_out,m_index++);
	}
	unsigned m_index;
	service_class_helper_t<t_interface> m_helper;
};


namespace fb2k {
	//! Modern get-std-api helper. \n
	//! Does not throw exceptions, crashes on failure. \n
	//! If failure is possible, use std_api_try_get() instead and handle false return value.
	template<typename api_t>
	service_ptr_t<api_t> std_api_get() {
		typedef typename api_t::t_interface_entrypoint entrypoint_t;
		service_ptr_t<api_t> ret;
		if (pfc::is_same_type<api_t, entrypoint_t>::value) {
			_standard_api_get_internal(ret._as_base_ptr(), api_t::class_guid);
		} else {
			ret ^= std_api_get<entrypoint_t>();
		}
		return ret;
	}

	//! Modern get-std-api helper. \n
	//! Returns true on scucess (ret ptr is valid), false on failure (API not found).
	template<typename api_t>
	bool std_api_try_get( service_ptr_t<api_t> & ret ) {
		typedef typename api_t::t_interface_entrypoint entrypoint_t;
		if (pfc::is_same_type<api_t, entrypoint_t>::value) {
			return _standard_api_try_get_internal(ret._as_base_ptr(), api_t::class_guid);
		} else {
			service_ptr_t<entrypoint_t> temp;
			if (! std_api_try_get( temp ) ) return false;
			return ret &= temp;
		}
	}
}
