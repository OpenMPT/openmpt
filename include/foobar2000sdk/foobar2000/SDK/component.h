#include "foobar2000.h"

// This is a helper class that gets reliably static-instantiated in all components so any special code that must be executed on startup can be shoved into its constructor.
class foobar2000_component_globals {
public:
	foobar2000_component_globals() {
#ifdef _MSC_VER
#ifndef _DEBUG
		::OverrideCrtAbort();
#endif
#endif
	}
};

class NOVTABLE foobar2000_client
{
public:
	typedef service_factory_base* pservice_factory_base;

	enum {
		FOOBAR2000_CLIENT_VERSION_COMPATIBLE = 72,
		FOOBAR2000_CLIENT_VERSION = FOOBAR2000_TARGET_VERSION,
	};
	virtual t_uint32 get_version() = 0;
	virtual pservice_factory_base get_service_list() = 0;

	virtual void get_config(stream_writer * p_stream,abort_callback & p_abort) = 0;
	virtual void set_config(stream_reader * p_stream,abort_callback & p_abort) = 0;
	virtual void set_library_path(const char * path,const char * name) = 0;
	virtual void services_init(bool val) = 0;
	virtual bool is_debug() = 0;
protected:
	foobar2000_client() {}
	~foobar2000_client() {}
};

class NOVTABLE foobar2000_api {
public:
	virtual service_class_ref service_enum_find_class(const GUID & p_guid) = 0;
	virtual bool service_enum_create(service_ptr_t<service_base> & p_out,service_class_ref p_class,t_size p_index) = 0;
	virtual t_size service_enum_get_count(service_class_ref p_class) = 0;
	virtual HWND get_main_window()=0;
	virtual bool assert_main_thread()=0;
	virtual bool is_main_thread()=0;
	virtual bool is_shutting_down()=0;
	virtual const char * get_profile_path()=0;
	virtual bool is_initializing() = 0;

	//New in 0.9.6
	virtual bool is_portable_mode_enabled() = 0;
	virtual bool is_quiet_mode_enabled() = 0;
protected:
	foobar2000_api() {}
	~foobar2000_api() {}
};

extern foobar2000_api * g_foobar2000_api;
