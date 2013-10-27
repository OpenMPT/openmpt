//! Basic callback startup/shutdown callback, on_init is called after the main window has been created, on_quit is called before the main window is destroyed. \n
//! To register: static initquit_factory_t<myclass> myclass_factory; \n
//! Note that you should be careful with calling other components during on_init/on_quit or \n
//! initializing services that are possibly used by other components by on_init/on_quit - \n
//! initialization order of components is undefined.
//! If some other service that you publish is not properly functional before you receive an on_init() call, \n
//! someone else might call this service before >your< on_init is invoked.
class NOVTABLE initquit : public service_base {
public:
	virtual void on_init() {}
	virtual void on_quit() {}

	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(initquit);
};

template<typename T>
class initquit_factory_t : public service_factory_single_t<T> {};


//! \since 1.1
namespace init_stages {
	enum {
		before_config_read = 10,
		after_config_read = 20,
		before_library_init = 30,
		after_library_init = 40,
		before_ui_init = 50,
		after_ui_init = 60,
	};
};

//! \since 1.1
class NOVTABLE init_stage_callback : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(init_stage_callback)
public:
	virtual void on_init_stage(t_uint32 stage) = 0;

	static void dispatch(t_uint32 stage) {FB2K_FOR_EACH_SERVICE(init_stage_callback, on_init_stage(stage));}
};
