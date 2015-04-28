//! Callback object class for main_thread_callback_manager service.
class NOVTABLE main_thread_callback : public service_base {
public:
	//! Gets called from main app thread. See main_thread_callback_manager description for more info.
	virtual void callback_run() = 0;
    
    void callback_enqueue(); // helper

	FB2K_MAKE_SERVICE_INTERFACE(main_thread_callback,service_base);
};

/*!
Allows you to queue a callback object to be called from main app thread. This is commonly used to trigger main-thread-only API calls from worker threads.\n
This can be also used from main app thread, to avoid race conditions when trying to use APIs that dispatch global callbacks from inside some other global callback, or using message loops / modal dialogs inside global callbacks.
*/
class NOVTABLE main_thread_callback_manager : public service_base {
public:
	//! Queues a callback object. This can be called from any thread, implementation ensures multithread safety. Implementation will call p_callback->callback_run() once later. To get it called repeatedly, you would need to add your callback again.
	virtual void add_callback(service_ptr_t<main_thread_callback> p_callback) = 0;

	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(main_thread_callback_manager);
};


void main_thread_callback_add(main_thread_callback::ptr ptr);

template<typename t_class> static void main_thread_callback_spawn() {
	main_thread_callback_add(new service_impl_t<t_class>);
}
template<typename t_class, typename t_param1> static void main_thread_callback_spawn(const t_param1 & p1) {
	main_thread_callback_add(new service_impl_t<t_class>(p1));
}
template<typename t_class, typename t_param1, typename t_param2> static void main_thread_callback_spawn(const t_param1 & p1, const t_param2 & p2) {
	main_thread_callback_add(new service_impl_t<t_class>(p1, p2));
}

// Proxy class - friend this to allow callInMainThread to access your private methods
class callInMainThread {
public:
	template<typename host_t, typename param_t>
	static void callThis(host_t * host, param_t & param) {
		host->inMainThread(param);
	}
    template<typename host_t>
    static void callThis( host_t * host ) {
        host->inMainThread();
    }
};

// Internal class, do not use.
template<typename service_t, typename param_t> 
class _callInMainThreadSvc_t : public main_thread_callback {
public:
	_callInMainThreadSvc_t(service_t * host, param_t const & param) : m_host(host), m_param(param) {}
	void callback_run() {
		callInMainThread::callThis(m_host.get_ptr(), m_param);
	}
private:
	service_ptr_t<service_t> m_host;
	param_t m_param;
};


// Main thread callback helper. You can use this to easily invoke inMainThread(someparam) on your class without writing any wrapper code.
// Requires myservice_t to be a fb2k service class with reference counting.
template<typename myservice_t, typename param_t> 
static void callInMainThreadSvc(myservice_t * host, param_t const & param) {
	typedef _callInMainThreadSvc_t<myservice_t, param_t> impl_t;
	service_ptr_t<impl_t> obj = new service_impl_t<impl_t>(host, param);
	static_api_ptr_t<main_thread_callback_manager>()->add_callback( obj );
}




//! Helper class to call methods of your class (host class) in main thread with convenience. \n
//! Deals with the otherwise ugly scenario of your class becoming invalid while a method is queued. \n
//! Have this as a member of your class, then use m_mthelper.add( this, somearg ) ; to defer a call to this->inMainThread(somearg). \n
//! If your class becomes invalid before inMainThread is executed, the pending callback is discarded. \n
//! You can optionally call shutdown() to invalidate all pending callbacks early (in a destructor of your class - without waiting for callInMainThreadHelper destructor to do the job. \n
//! In order to let callInMainThreadHelper access your private methods, declare friend class callInMainThread.
class callInMainThreadHelper {
public:
    
    typedef pfc::rcptr_t< bool > killswitch_t;
    
    template<typename host_t, typename arg_t>
    class entry : public main_thread_callback {
    public:
        entry( host_t * host, arg_t const & arg, killswitch_t ks ) : m_ks(ks), m_host(host), m_arg(arg) {}
        void callback_run() {
            if (!*m_ks) callInMainThread::callThis( m_host, m_arg );
        }
    private:
        killswitch_t m_ks;
        host_t * m_host;
        arg_t m_arg;
    };
    template<typename host_t>
    class entryVoid : public main_thread_callback {
    public:
        entryVoid( host_t * host, killswitch_t ks ) : m_ks(ks), m_host(host) {}
        void callback_run() {
            if (!*m_ks) callInMainThread::callThis( m_host );
        }
    private:
        killswitch_t m_ks;
        host_t * m_host;
    };
    
    template<typename host_t, typename arg_t>
    void add( host_t * host, arg_t const & arg) {
        add_( new service_impl_t< entry<host_t, arg_t> >( host, arg, m_ks ) );
    }
    template<typename host_t>
    void add( host_t * host ) {
        add_( new service_impl_t< entryVoid<host_t> >( host, m_ks ) );
    }
    void add_( main_thread_callback::ptr cb ) {
        main_thread_callback_add( cb );
    }
    
    callInMainThreadHelper() {
        m_ks.new_t();
        * m_ks = false;
    }
    void shutdown() {
        PFC_ASSERT( core_api::is_main_thread() );
        * m_ks = true;
    }
    ~callInMainThreadHelper() {
        shutdown();
    }
    
private:
    killswitch_t m_ks;
    
};
